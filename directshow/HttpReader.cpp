/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "HttpReader.h"
#include "DirectShow.h"
#include <winioctl.h>
#include <initguid.h>
#include "moreuuids.h"

#define CHUNK_WIDTH 14 
#define CHUNK_SIZE (1 << CHUNK_WIDTH)
#define CHUNK_MASK (CHUNK_SIZE - 1) 

// CHttpReader

CHttpReader::CHttpReader(IUnknown* pUnk, HRESULT* phr)
	: CAsyncReader(NAME("CHttpReader"), pUnk, &m_stream, phr, __uuidof(this))
{
	if(phr) *phr = S_OK;

	m_mt.majortype = MEDIATYPE_Stream;
}

CHttpReader::~CHttpReader()
{
}

STDMETHODIMP CHttpReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(IFileSourceFilter)
		QI(IHttpReader)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CHttpReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt) 
{
	std::string s = Util::UTF16To8(pszFileName);

	if(s.find("apple.com") != std::wstring::npos)
	{
		m_stream.m_useragent = "QuickTime";
	}

	if(!m_stream.Load(s.c_str()))
	{
		return E_FAIL;
	}

	m_url = pszFileName;

	return S_OK;
}

STDMETHODIMP CHttpReader::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);
	
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_url.size() + 1) * sizeof(WCHAR))))
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*ppszFileName, m_url.c_str());

	return S_OK;
}

// IHttpReader

STDMETHODIMP CHttpReader::GetAvailable(std::list<HttpFilePart>& parts, __int64& len)
{
	parts.clear();

	len = 0;

	CAutoLock cAutoLock(&m_stream);

	__int64 end = (m_stream.m_length + CHUNK_MASK) / CHUNK_SIZE;

	if(m_stream.m_chunk.count >= end)
	{
		return S_OK;
	}

	end *= CHUNK_SIZE;
	
	len = m_stream.m_length;

	for(__int64 pos = 0, start = pos; pos <= end; pos += CHUNK_SIZE)
	{
		if(!m_stream.GetChunk(pos))
		{
			if(pos > start)
			{
				HttpFilePart part;

				part.pos = start;
				part.len = pos - start;

				parts.push_back(part);
			}

			start = pos + CHUNK_SIZE;
		}
	}

	return S_OK;
}

STDMETHODIMP CHttpReader::SetUserAgent(LPCSTR useragent)
{
	CAutoLock cAutoLock(&m_stream);

	m_stream.m_useragent = useragent;

	return S_OK;
}

// CHttpStream

CHttpStream::CHttpStream()
	: m_input(INVALID_HANDLE_VALUE)
	, m_output(INVALID_HANDLE_VALUE)
	, m_useragent("Homesys")
	, m_pos(0)
	, m_length(0)
	, m_seek(-1)
	, m_error(false)
{
	m_chunk.size = 1024 * 1024;
	m_chunk.count = 0;
	m_chunk.buff = new LONG[m_chunk.size >> 2];

	memset(m_chunk.buff, 0, m_chunk.size);
}

CHttpStream::~CHttpStream()
{
	Close();

	delete [] m_chunk.buff;
}

bool CHttpStream::Load(LPCSTR url)
{
	Close();

	m_url = url;

	DirectShow::GetTempFileName(m_temp, L"HTTPReader");

	m_output = CreateFile(m_temp.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	if(m_output == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD bytes = 0;
	
	DeviceIoControl(m_output, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, (LPDWORD) &bytes, NULL);

	m_input = CreateFile(m_temp.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if(m_input == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	CAMThread::Create();

	if(CAMThread::CallWorker(CMD_START) != S_OK || m_length == 0)
	{
		return false;
	}

	return true;
}

bool CHttpStream::Open(__int64 start, int retry_count)
{
	// wprintf(L"HttpReader: Open(%I64d, %d)\n", start, retry_count);

	std::map<std::string, std::string> headers;

	char range[64];

	sprintf(range, "bytes=%I64d-", start);

	headers["Range"] = range;
	headers["User-Agent"] = m_useragent;

	if(!m_socket.HttpGet(m_url.c_str(), headers))
	{
		return false;
	}

	if(start > 0)
	{
		auto i = headers.find("HTTP_RES");

		if(i == headers.end())
		{
			return false;
		}

		if(atoi(i->second.c_str()) != 206)
		{
			// wprintf(L"HttpReader: %s, no partial content (start = %lld)\n", i->second.c_str(), start);

			if(retry_count > 0)
			{
				Sleep(100);

				return Open(start, retry_count - 1);
			}

			return false;
		}
	}

	if(m_socket.GetContentType().find("/x-ms-") != std::string::npos) // ???
	{
		return false;
	}

	CAutoLock cAutoLock(this);

	m_length = m_socket.GetContentLength();

	return true;
}

void CHttpStream::Close()
{
	if(m_hThread != NULL)
	{
		CAMThread::CallWorker(CMD_EXIT);
		CAMThread::Close();
	}
	
	if(m_input != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_input);

		m_input = INVALID_HANDLE_VALUE;
	}

	if(m_output != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_output);

		m_output = INVALID_HANDLE_VALUE;
	}

	if(!m_temp.empty())
	{
		DeleteFile(m_temp.c_str());

		m_temp.clear();
	}

	memset(m_chunk.buff, 0, m_chunk.size);
	m_chunk.count = 0;

	m_pos = 0;
	m_length = 0;
	m_seek = -1;

	m_error = false;
}

HRESULT CHttpStream::SetPointer(LONGLONG llPos)
{
	CAutoLock cAutoLock(this);

	llPos = std::max<__int64>(0, std::min<__int64>(llPos, m_length));

	if(m_pos != llPos)
	{
		m_pos = llPos;
		m_seek = llPos;
	}

	return S_OK;
}

HRESULT CHttpStream::Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead)
{
	// wprintf(L"HttpReader: Read(%I64d, %d)\n", m_pos, dwBytesToRead);
	
	CAutoLock cAutoLock(&m_csReadLock);

	__int64 bytes = dwBytesToRead;

	{
		CAutoLock csAutoLock(this);

		bytes = std::min<__int64>(bytes, m_length - m_pos);
	}

	if(pdwBytesRead)
	{
		*pdwBytesRead = 0;
	}

	while(bytes > 0 && !m_error) // TODO
	{
		if(!GetChunk(m_pos))
		{
			Sleep(100);

			continue;
		}

		__int64 size = std::min<__int64>(bytes, CHUNK_SIZE - (m_pos & CHUNK_MASK));

		LARGE_INTEGER liDistanceToMove;
		LARGE_INTEGER liNewFilePointer;

		liDistanceToMove.QuadPart = m_pos;

		SetFilePointerEx(m_input, liDistanceToMove, &liNewFilePointer, FILE_BEGIN);

		DWORD read = 0;

		if(!ReadFile(m_input, pbBuffer, size, &read, NULL))
		{
			break;
		}

		m_pos += size;
		pbBuffer += size;
		bytes -= size;

		if(pdwBytesRead)
		{
			*pdwBytesRead += size;
		}
	}

	return bytes == 0 ? S_OK : E_FAIL;
}

LONGLONG CHttpStream::Size(LONGLONG* pSizeAvailable)
{
	CAutoLock cAutoLock(this);

	__int64 total = m_socket.CanSeek() ? m_length : 0;
	__int64 available = m_length;

	if(pSizeAvailable)
	{
		*pSizeAvailable = available;
	}

	return total;
}

DWORD CHttpStream::Alignment()
{
    return 1;
}

void CHttpStream::Lock()
{
    m_csReadLock.Lock();
}

void CHttpStream::Unlock()
{
    m_csReadLock.Unlock();
}

DWORD CHttpStream::ThreadProc()
{
	Util::Socket::Startup();

	Open();

	Reply(S_OK);

	FillBuffer();

	GetRequest();

	CAMThread::m_hThread = NULL;

	Reply(S_OK);

	Util::Socket::Cleanup();

	return 0;
}

void CHttpStream::FillBuffer()
{
	int count = (m_length + CHUNK_MASK) / CHUNK_SIZE;

	__int64 pos = 0;

	BYTE* buff = new BYTE[CHUNK_SIZE];

	DWORD param;
/*
bool paused = false;
__int64 seek_last = 0;
*/
	while(!m_error && m_chunk.count < count && !CheckRequest(&param))
	{
		bool reopen = false;

		if(m_seek >= 0)
		{
			CAutoLock cAutoLock(this);

			__int64 i = m_seek;

			if(i >= pos + (1 << 20)) //  CHUNK_SIZE * 8)
			{
				reopen = true;
			}
			else if(i < pos)
			{
				for(; i < pos; i += CHUNK_SIZE)
				{
					if(!GetChunk(i))
					{
						reopen = true;

						break;
					}
				}
			}

			if(reopen)
			{
				pos = i & ~CHUNK_MASK;

				while(pos < m_length && GetChunk(pos))
				{
					pos += CHUNK_SIZE;
				}
			}
/*
if(m_seek < seek_last || m_seek > seek_last + CHUNK_SIZE)
{
	printf("%I64d => %I64d\n", seek_last, m_seek);

	seek_last = m_seek;
}
*/
			m_seek = -1;
		}

		if(reopen)
		{
			if(!Open(pos, 5))
			{
				m_error = true;

				continue;
			}
		}

		LARGE_INTEGER liDistanceToMove;
		LARGE_INTEGER liNewFilePointer;

		liDistanceToMove.QuadPart = pos;

		SetFilePointerEx(m_output, liDistanceToMove, &liNewFilePointer, FILE_BEGIN);

		if(pos >= m_length)
		{
			pos = 0;

			while(pos < m_length && GetChunk(pos))
			{
				pos += CHUNK_SIZE;
			}

			if(!Open(pos, 5))
			{
				m_error = true;
			}

			continue;
		}

		__int64 size = std::min<__int64>(m_length - pos, (__int64)CHUNK_SIZE);

		if(m_socket.ReadAll(buff, size))
		{
			// wprintf(L"HttpReader: Write(%I64d, %I64d)\n", pos, size);

			pos += size;
		}
		else
		{
			m_error = true;

			continue;
		}

		DWORD written = 0;

		WriteFile(m_output, buff, size, &written, NULL);

		SetChunk(liDistanceToMove.QuadPart);
	}

	delete [] buff;
}

bool CHttpStream::GetChunk(__int64 pos)
{
	int chunk = (int)(pos >> CHUNK_WIDTH);

	return _bittest(&m_chunk.buff[chunk >> 5], chunk & 31) != 0;
}

void CHttpStream::SetChunk(__int64 pos)
{
	int chunk = (int)(pos >> CHUNK_WIDTH);

	if(_bittestandset(&m_chunk.buff[chunk >> 5], chunk & 31) == 0)
	{
		CAutoLock cAutoLock(this);

		m_chunk.count++;
	}
}
