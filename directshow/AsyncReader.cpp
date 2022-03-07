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


#include "StdAfx.h"
#include "AsyncReader.h"
#include "DirectShow.h"

// CAsyncFileReader

CAsyncFileReader::CAsyncFileReader(LPCWSTR fn, HRESULT& hr) 
	: CUnknown(NAME("CAsyncFileReader"), NULL, &hr)
	, m_fn(fn)
	, m_len(0)
	, m_hBreakEvent(NULL)
	, m_lOsError(0)
{
	m_file = CreateFile(fn, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(m_file == INVALID_HANDLE_VALUE)
	{
		hr = E_FAIL;

		return;
	}

	GetFileSizeEx(m_file, (LARGE_INTEGER*)&m_len);

	hr = S_OK;
}

CAsyncFileReader::~CAsyncFileReader()
{
	if(m_file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_file);
	}
}

STDMETHODIMP CAsyncFileReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IAsyncReader)
		QI(ISyncReader)
		QI(IFileHandle)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAsyncReader

STDMETHODIMP CAsyncFileReader::SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	LONGLONG total, available;

	if(FAILED(Length(&total, &available)))
	{
		return E_FAIL;
	}

	if(llPosition + lLength > available) 
	{
		return E_FAIL;
	}

	LARGE_INTEGER pos, newpos;

	pos.QuadPart = llPosition;

	newpos.QuadPart = -1;

	if(SetFilePointerEx(m_file, pos, &newpos, FILE_BEGIN) && llPosition == newpos.QuadPart)
	{
		DWORD read = 0;

		if(ReadFile(m_file, pBuffer, lLength, &read, NULL) && read == lLength)
		{
			m_lOsError = 0;

			return S_OK;
		}
	}

	m_lOsError = GetLastError();

	if(m_lOsError == ERROR_BAD_NETPATH)
	{
		wprintf(L"Network path lost, trying to reopen '%s'\n", m_fn.c_str());
		
		while(m_hBreakEvent != NULL && WaitForSingleObject(m_hBreakEvent, 500) == WAIT_TIMEOUT)
		{
			HANDLE file = CreateFile(m_fn.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

			if(file != INVALID_HANDLE_VALUE)
			{
				if(m_file != INVALID_HANDLE_VALUE)
				{
					CloseHandle(m_file);
				}

				m_file = file;

				return SyncRead(llPosition, lLength, pBuffer);
			}
		}
	}

	return E_FAIL;
}

STDMETHODIMP CAsyncFileReader::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	LARGE_INTEGER size;

	size.QuadPart = m_len;

	if(m_len < 0)
	{
		GetFileSizeEx(m_file, (LARGE_INTEGER*)&size);
	}

	if(pTotal)
	{
		*pTotal = size.QuadPart;
	}

	if(pAvailable)
	{
		*pAvailable = size.QuadPart;
	}

	return S_OK;
}

// IFileHandle

STDMETHODIMP_(HANDLE) CAsyncFileReader::GetFileHandle()
{
	return m_file;
}

// CAsyncMemReader

CAsyncMemReader::CAsyncMemReader(const BYTE* buff, int size)
	: CUnknown(_T(""), NULL)
	, m_buff(buff)
	, m_size(size)
{
}

STDMETHODIMP CAsyncMemReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(IAsyncReader)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAsyncReader

STDMETHODIMP CAsyncMemReader::RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator** ppActual) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CAsyncMemReader::Request(IMediaSample* pSample, DWORD_PTR dwUser) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CAsyncMemReader::WaitForNext(DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR* pdwUser) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CAsyncMemReader::SyncReadAligned(IMediaSample* pSample) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CAsyncMemReader::BeginFlush() 
{
	return S_OK;
}

STDMETHODIMP CAsyncMemReader::EndFlush() 
{
	return S_OK;
}

STDMETHODIMP CAsyncMemReader::SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	if(llPosition >= 0 && llPosition + lLength <= m_size)
	{
		memcpy(pBuffer, &m_buff[llPosition], lLength);

		return S_OK;
	}

	return E_INVALIDARG;
}
    
STDMETHODIMP CAsyncMemReader::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	*pTotal = m_size;
	*pAvailable = m_size;

	return S_OK;
}

// CAsyncHttpStreamReader

CAsyncHttpStreamReader::CAsyncHttpStreamReader(CHttpStream* stream)
	: CUnknown(_T("CAsyncHttpStreamReader"), NULL)
	, m_stream(stream)
{
}

CAsyncHttpStreamReader::~CAsyncHttpStreamReader()
{
	delete m_stream;
}

STDMETHODIMP CAsyncHttpStreamReader::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(IAsyncReader)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IAsyncReader

STDMETHODIMP CAsyncHttpStreamReader::RequestAllocator(IMemAllocator* pPreferred, ALLOCATOR_PROPERTIES* pProps, IMemAllocator** ppActual) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CAsyncHttpStreamReader::Request(IMediaSample* pSample, DWORD_PTR dwUser) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CAsyncHttpStreamReader::WaitForNext(DWORD dwTimeout, IMediaSample** ppSample, DWORD_PTR* pdwUser) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CAsyncHttpStreamReader::SyncReadAligned(IMediaSample* pSample) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CAsyncHttpStreamReader::BeginFlush() 
{
	return S_OK;
}

STDMETHODIMP CAsyncHttpStreamReader::EndFlush() 
{
	return S_OK;
}

STDMETHODIMP CAsyncHttpStreamReader::SyncRead(LONGLONG llPosition, LONG lLength, BYTE* pBuffer)
{
	DWORD bytes = 0;

	m_stream->SetPointer(llPosition);
	m_stream->Read(pBuffer, lLength, 1, &bytes);

	return lLength == bytes ? S_OK : E_FAIL;
}
    
STDMETHODIMP CAsyncHttpStreamReader::Length(LONGLONG* pTotal, LONGLONG* pAvailable)
{
	*pTotal = m_stream->Size(pAvailable);

	return S_OK;
}
