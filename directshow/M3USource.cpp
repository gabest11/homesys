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
#include "M3USource.h"
#include "DirectShow.h"
#include "AsyncReader.h"
#include "MediaTypeEx.h"
#include <initguid.h>
#include "moreuuids.h"

// CM3USourceFilter

CM3USourceFilter::CM3USourceFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseFilter(NAME("CM3USourceFilter"), lpunk, this, __uuidof(this))
	, m_pOutput(NULL)
{
	m_pOutput = new CM3USourceOutputPin(this, phr);
}

CM3USourceFilter::~CM3USourceFilter()
{
	delete m_pOutput;
}

STDMETHODIMP CM3USourceFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(IFileSourceFilter)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

int CM3USourceFilter::GetPinCount()
{
	return m_pOutput != NULL ? 1 : 0;
}

CBasePin* CM3USourceFilter::GetPin(int n)
{
	return n == 0 ? m_pOutput : NULL;
}

STDMETHODIMP CM3USourceFilter::Run(REFERENCE_TIME tStart)
{
	m_pOutput->DeliverBeginFlush();

	CAMThread::CallWorker(CMD_FLUSH);

	CAMThread::CallWorker(CMD_START);

	return __super::Run(tStart);
}

STDMETHODIMP CM3USourceFilter::Pause()
{
	if(m_State == State_Stopped)
	{
		CAMThread::Create();

		CAMThread::CallWorker(CMD_START);
	}

	return __super::Pause();
}

STDMETHODIMP CM3USourceFilter::Stop()
{
	if(m_State != State_Stopped)
	{
		m_pOutput->DeliverBeginFlush();

		CAMThread::CallWorker(CMD_EXIT);

		CAMThread::Close();

		m_pOutput->DeliverEndFlush();
	}

	return __super::Stop();
}

DWORD CM3USourceFilter::ThreadProc()
{
	Util::Socket::Startup();

	Util::Socket s;

	HRESULT hr;

	int meta = 1;

	hr = Load(s, std::string(m_url.begin(), m_url.end()).c_str(), meta);

	if(FAILED(hr))
	{
		NotifyEvent(EC_ERRORABORT, 0, 0);
	}

	int size = meta > 0 ? meta : 2048;

	BYTE* info = new BYTE[255 * 16];

	while(1)
	{
		DWORD cmd = GetRequest();

		if(cmd == CMD_EXIT)
		{
			Reply(S_OK);

			break;
		}

		switch(cmd)
		{
		case CMD_FLUSH:

			m_pOutput->DeliverEndFlush();

			Reply(S_OK);

			break;

		case CMD_START:

			Reply(S_OK);

			m_pOutput->DeliverNewSegment(0, _I64_MAX, 1.0f);

			while(!CheckRequest(NULL))
			{
				std::vector<BYTE>* buff = new std::vector<BYTE>(size);

				if(s.ReadAll(buff->data(), size))
				{
					m_pOutput->Deliver(buff);
				}
				else
				{
					delete buff;

					break;
				}

				if(meta > 0)
				{
					BYTE count = 0;

					if(!s.ReadAll(&count, 1))
					{
						break;
					}

					if(count > 0)
					{
						if(!s.ReadAll(info, count * 16))
						{
							break;
						}

						std::string str((LPCSTR)info, count * 16);

						std::string::size_type i = str.find("StreamTitle='");

						if(i != std::string::npos)
						{
							std::string::size_type j = i + 13;

							for(; j < str.size(); j++)
							{
								if(str[j] == '\\')
								{
									continue;
								}
								else if(str[j] == '\'')
								{
									str = str.substr(i, j - i);

									SetProperty(L"TITL", std::wstring(str.begin(), str.end()).c_str());

									break;
								}
							}
						}
					}
				}
			}

			break;
		}
	}

	delete [] info;

	Util::Socket::Cleanup();

	return 0;
}

HRESULT CM3USourceFilter::Load(Util::Socket& s, LPCSTR url, int& meta)
{
	std::map<std::string, std::string> headers;

	if(meta > 0)
	{
		headers["Icy-MetaData"] = "1";

		meta = 0;
	}

	if(!s.HttpGet(url, headers))
	{
		return E_FAIL;
	}
	
	if(s.GetContentType() == "audio/x-mpegurl" || s.GetContentType() == "audio/x-scpls")
	{
		int len = (int)s.GetContentLength();

		if(len > 65536)
		{
			return E_FAIL;
		}
		else if(len == 0)
		{
			len = 65536;
		}

		HRESULT hr = E_FAIL;

		std::vector<BYTE> buff;

		if(s.ReadToEnd(buff, len))
		{
			std::list<std::string> tokens;

			Util::Explode(std::string((const char*)buff.data(), buff.size()), tokens, "\n");

			for(auto i = tokens.begin(); i != tokens.end(); i++)
			{
				std::string file;

				if(s.GetContentType() == "audio/x-scpls")
				{
					std::list<std::string> sl;

					if(Util::Explode(*i, sl, '=', 2).find("File") == 0)
					{
						file = sl.back();
					}
				}
				else if(i->find("://") != std::string::npos)
				{
					file = *i;
				}

				file = Util::Trim(file);

				if(!file.empty())
				{
					hr = Load(s, file.c_str(), meta);

					if(SUCCEEDED(hr))
					{
						break;
					}
				}
			}
		}

		return hr;
	}

	if(s.GetContentLength() != 0)
	{
		return E_FAIL; // live streaming only
	}

	for(auto i = headers.begin(); i != headers.end(); i++)
	{
		std::wstring value = Util::UTF8To16(i->second.c_str()); // TODO: not utf8, non-ascii is escaped somehow (\somecharactercodenum)

		if(stricmp(i->first.c_str(), "icy-name") == 0)
		{
			SetProperty(L"TITL", value.c_str());
		}
		else if(stricmp(i->first.c_str(), "icy-description") == 0)
		{
			SetProperty(L"DESC", value.c_str());
		}
		else if(stricmp(i->first.c_str(), "icy-metaint") == 0)
		{
			meta = atoi(i->second.c_str());
		}
	}

	CMediaType& mt = m_pOutput->CurrentMediaType();

	if(mt.IsValid())
	{
		return S_OK;
	}

	std::vector<BYTE> buff(2048);

	if(s.ReadAll(&buff[0], buff.size()))
	{
		CBaseSplitterFile f(&buff[0], buff.size());

		while(f.GetRemaining() > 0)
		{
			if(s.GetContentType() == "audio/mpeg")
			{
				CBaseSplitterFile::MpegAudioHeader h;

				if(f.Read(h, f.GetRemaining(), false, &mt))
				{
					f.Seek(f.GetPos() + h.bytes);

					if(f.BitRead(12, true) == 0xfff)
					{
						break;
					}
				}
			}
			else if(s.GetContentType() == "audio/aac" || s.GetContentType() == "audio/aacp")
			{
				CBaseSplitterFile::AACHeader h;

				if(f.Read(h, f.GetRemaining(), &mt))
				{
					f.Seek(f.GetPos() + h.bytes);

					if(f.BitRead(12, true) == 0xfff)
					{
						break;
					}
				}
			}
			else
			{
				break;
			}

			mt.InitMediaType();
		}
	}

	return mt.IsValid() ? S_OK : E_FAIL;
}

// IFileSourceFilter

STDMETHODIMP CM3USourceFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped)
	{
		return VFW_E_NOT_STOPPED;
	}

	m_url.clear();

	m_pOutput->CurrentMediaType().InitMediaType();

	std::wstring fn = pszFileName;

	Util::Socket::Startup();

	Util::Socket s;

	HRESULT hr;

	int meta = 0;

	hr = Load(s, std::string(fn.begin(), fn.end()).c_str(), meta);

	if(SUCCEEDED(hr))
	{
		m_url = fn;
	}

	s.Close();

	Util::Socket::Cleanup();

	return hr;
}

STDMETHODIMP CM3USourceFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);
	
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_url.size() + 1) * sizeof(WCHAR))))
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*ppszFileName, m_url.c_str());

	return S_OK;
}

// CM3USourceOutputPin

CM3USourceFilter::CM3USourceOutputPin::CM3USourceOutputPin(CM3USourceFilter* pFilter, HRESULT* phr)
	: CBaseOutputPin(NAME("CM3USourceOutputPin"), pFilter, pFilter, phr, L"Output")
	, m_queue(10)
	, m_flushing(false)
	, m_size(0)
	, m_start(0)
{
}

CM3USourceFilter::CM3USourceOutputPin::~CM3USourceOutputPin()
{
}

HRESULT CM3USourceFilter::CM3USourceOutputPin::Active()
{
	m_size = 0;
	m_start = 0;

	CAMThread::Create();

	return __super::Active();
}

HRESULT CM3USourceFilter::CM3USourceOutputPin::Inactive()
{
	m_exit.Set();

	CAMThread::Close();

	return __super::Inactive();
}

HRESULT CM3USourceFilter::CM3USourceOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, float dRate)
{
	m_start = tStart;

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CM3USourceFilter::CM3USourceOutputPin::Deliver(std::vector<BYTE>* buff)
{
	if(m_flushing) {delete buff; return S_FALSE;}

	m_queue.Enqueue(buff);

	return S_OK;
}

HRESULT CM3USourceFilter::CM3USourceOutputPin::DeliverBeginFlush()
{
	m_flushing = true;

	return __super::DeliverBeginFlush();
}

HRESULT CM3USourceFilter::CM3USourceOutputPin::DeliverEndFlush()
{
	m_queue.GetDequeueEvent().Wait();

	m_buff.clear();
	m_size = 0;

	m_flushing = false;

	return __super::DeliverEndFlush();
}

HRESULT CM3USourceFilter::CM3USourceOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return m_mt == *pmt ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CM3USourceFilter::CM3USourceOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_mt;

	return S_OK;
}

HRESULT CM3USourceFilter::CM3USourceOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 65536;

	return S_OK;
}

DWORD CM3USourceFilter::CM3USourceOutputPin::ThreadProc()
{
	HANDLE handles[] = {m_exit, m_queue.GetEnqueueEvent()};

	while(m_hThread != NULL)
	{
		switch(::WaitForMultipleObjects(countof(handles), handles, FALSE, INFINITE))
		{
		default:
		case WAIT_OBJECT_0 + 0: 

			return 0;

		case WAIT_OBJECT_0 + 1:

			while(m_queue.GetCount() > 0)
			{
				std::vector<BYTE>* buff = m_queue.Dequeue();

				if(!m_flushing)
				{
					m_buff.resize(m_size + buff->size());
					memcpy(&m_buff[m_size], buff->data(), buff->size());
					m_size += buff->size();

					CBaseSplitterFile f(&m_buff[0], m_size);

					if(m_mt.subtype == MEDIASUBTYPE_MPEG_AUDIO || m_mt.subtype == MEDIASUBTYPE_MP3)
					{
						CBaseSplitterFile::MpegAudioHeader h;

						while(f.GetRemaining() >= 4)
						{
							if(f.Read(h, (int)f.GetRemaining(), false))
							{
								f.Seek(h.start);

								if(f.GetRemaining() < h.FrameSize)
								{
									break;
								}

								DeliverSample(f, h.FrameSize, h.rtDuration);
							}
							else
							{
								f.Seek(f.GetLength());

								break;
							}
						}
					}
					else if(m_mt.subtype == MEDIASUBTYPE_AAC)
					{
						CBaseSplitterFile::AACHeader h;

						while(f.GetRemaining() >= 9)
						{
							if(f.Read(h, (int)f.GetRemaining(), false))
							{
								if(f.GetRemaining() < h.bytes)
								{
									f.Seek(h.start);

									break;
								}

								DeliverSample(f, h.bytes, h.rtDuration);
							}
							else
							{
								f.Seek(f.GetLength());

								break;
							}
						}
					}
					else
					{
						f.Seek(f.GetLength());
					}

					m_size = f.GetRemaining();

					f.ByteRead(&m_buff[0], m_size);
				}

				delete buff;
			}

			break;
		}
	}

	return 0;
}

void CM3USourceFilter::CM3USourceOutputPin::DeliverSample(CBaseSplitterFile& f, int len, REFERENCE_TIME dur)
{
	HRESULT hr;

	__int64 next = f.GetPos() + len;

	REFERENCE_TIME start = m_start;
	REFERENCE_TIME stop = start + dur;

	m_start = stop;

	CComPtr<IMediaSample> sample;

	hr = GetDeliveryBuffer(&sample, NULL, NULL, 0);

	if(SUCCEEDED(hr) && sample->GetSize() >= len)
	{
		BYTE* dst = NULL;
		
		hr = sample->GetPointer(&dst);

		if(SUCCEEDED(hr) && dst != NULL)
		{
			hr = f.ByteRead(dst, len);

			if(SUCCEEDED(hr))
			{
				sample->SetActualDataLength(len);

				sample->SetTime(&start, &stop);
				sample->SetMediaTime(NULL, NULL);

				sample->SetDiscontinuity(FALSE);
				sample->SetSyncPoint(TRUE);
				sample->SetPreroll(FALSE);

				__super::Deliver(sample);
			}
		}
	}

	f.Seek(next);
}
