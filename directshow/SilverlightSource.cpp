#include "stdafx.h"
#include "SilverlightSource.h"
#include "DirectShow.h"
#include "AsyncReader.h"
#include "MediaTypeEx.h"
#include <initguid.h>
#include "moreuuids.h"

// CSilverlightSourceFilter

CSilverlightSourceFilter::CSilverlightSourceFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseFilter(NAME("CSilverlightSourceFilter"), lpunk, this, __uuidof(this))
	, m_current(0)
{
}

CSilverlightSourceFilter::~CSilverlightSourceFilter()
{
	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		delete *i;
	}
}

STDMETHODIMP CSilverlightSourceFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(IFileSourceFilter)
		QI(IMediaSeeking)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

int CSilverlightSourceFilter::GetPinCount()
{
	return m_outputs.size();
}

CBasePin* CSilverlightSourceFilter::GetPin(int n)
{
	return n >= 0 && n < m_outputs.size() ? m_outputs[n] : NULL;
}

STDMETHODIMP CSilverlightSourceFilter::Pause()
{
	FILTER_STATE state = m_State;

	HRESULT hr = __super::Pause();

	if(state == State_Stopped)
	{
		CAMThread::Create();

		CAMThread::CallWorker(CMD_START);
	}

	return hr;
}

STDMETHODIMP CSilverlightSourceFilter::Stop()
{
	if(m_State != State_Stopped)
	{
		for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
		{
			(*i)->DeliverBeginFlush();
		}

		CAMThread::CallWorker(CMD_EXIT);

		CAMThread::Close();

		for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
		{
			(*i)->DeliverEndFlush();
		}
	}

	return __super::Stop();
}

DWORD CSilverlightSourceFilter::ThreadProc()
{
	std::list<CSilverlightSourceOutputPin*> pins;

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

			for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
			{
				(*i)->DeliverEndFlush();
			}

			Reply(S_OK);

			break;

		case CMD_START:

			Reply(S_OK);

			for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
			{
				(*i)->DeliverNewSegment(m_current, m_media.m_duration, 1.0f);
			}

			pins.clear();
			pins.insert(pins.end(), m_outputs.begin(), m_outputs.end());

			while(!CheckRequest(NULL) && !pins.empty())
			{
				CSilverlightSourceOutputPin* pin = NULL;

				REFERENCE_TIME current = _I64_MAX;
				REFERENCE_TIME next;

				for(auto i = pins.begin(); i != pins.end(); )
				{
					auto j = i++;

					if((*j)->GetNextSampleStart(next))
					{
						if(next < current)
						{
							pin = *j;
							current = next;
						}
					}
					else
					{
						pins.erase(j);
					}
				}

				m_current = current;

				if(pin != NULL)
				{
					pin->QueueNextSample();
				}
			}

			for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
			{
				(*i)->DeliverEndOfStream();
			}

			break;
		}
	}

	return 0;
}

HRESULT CSilverlightSourceFilter::Load(Util::Socket& s, LPCSTR url)
{
	std::map<std::string, std::string> headers;

	std::list<std::string> types;

	types.push_back("text/xml");

	std::string str;

	if(!s.HttpGetXML(url, headers, types, str, 1, 0))
	{
		return E_FAIL;
	}

	TiXmlDocument doc;

	doc.Parse(str.c_str(), NULL, TIXML_ENCODING_UTF8);

	if(!m_media.Parse(doc.FirstChildElement("SmoothStreamingMedia"))
	|| !m_media.m_protection.id.empty()
	)
	{
		return E_FAIL;
	}

	return S_OK;
}

// IFileSourceFilter

STDMETHODIMP CSilverlightSourceFilter::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cAutoLock(this);

	if(m_State != State_Stopped)
	{
		return VFW_E_NOT_STOPPED;
	}

	m_url.clear();

	for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
	{
		delete *i;
	}

	m_outputs.clear();

	std::wstring fn = pszFileName;

	Util::Replace(fn, L"silverlight://", L"http://");

	if(fn.find(L"?") == std::wstring::npos) // find .ism too?
	{
		std::list<std::wstring> fns;

		fns.push_back(fn);

		// add mirrors for rtlmost.hu

		std::wstring s1 = L"http://smooth.edge";

		std::wstring::size_type j = fn.find('.', s1.size());

		if(fn.find(s1) == 0 && j != std::wstring::npos)
		{
			std::wstring s2 = fn.substr(j);

			for(char j = 'a'; j <= 'd'; j++)
			{
				for(int i = 7; i >= 1; i--)
				{
					std::wstring s3 = s1 + Util::Format(L"%d%c", i, j) + s2;
				
					if(wcsicmp(fn.c_str(), s3.c_str()) != 0) 
					{
						fns.push_back(s3);
					}
				}
			}
		}

		//

		HRESULT hr;

		std::string url;

		Util::Socket::Startup();

		for(auto i = fns.begin(); i != fns.end(); i++)
		{
			Util::Socket s;

			url = std::string(i->begin(), i->end());

			hr = Load(s, (url + "/Manifest").c_str());

			if(SUCCEEDED(hr))
			{
				m_url = *i;

				break;
			}
		}

		Util::Socket::Cleanup();

		for(auto i = m_media.m_streams.begin(); i != m_media.m_streams.end(); i++)
		{
			CSilverlightSourceOutputPin* pin = new CSilverlightSourceOutputPin(this, url, &*i, &hr);

			if(SUCCEEDED(hr))
			{
				m_outputs.push_back(pin);
			}
			else
			{
				delete pin;
			}
		}
	}

	return !m_outputs.empty() ? S_OK : E_FAIL;
}

STDMETHODIMP CSilverlightSourceFilter::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);
	
	if(!(*ppszFileName = (LPOLESTR)CoTaskMemAlloc((m_url.size() + 1) * sizeof(WCHAR))))
	{
		return E_OUTOFMEMORY;
	}

	wcscpy(*ppszFileName, m_url.c_str());

	return S_OK;
}

// IMediaSeeking

STDMETHODIMP CSilverlightSourceFilter::GetCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL)
	{
		return E_POINTER;
	}
	
	*pCapabilities = 
		AM_SEEKING_CanGetStopPos | 
		AM_SEEKING_CanGetDuration | 
		AM_SEEKING_CanSeekAbsolute |
		AM_SEEKING_CanSeekForwards |
		AM_SEEKING_CanSeekBackwards;

	return S_OK;
}

STDMETHODIMP CSilverlightSourceFilter::CheckCapabilities(DWORD* pCapabilities)
{
	if(pCapabilities == NULL) return E_POINTER;

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;

	GetCapabilities(&caps);

	caps &= *pCapabilities;

	return caps == 0 ? E_FAIL : caps == *pCapabilities ? S_OK : S_FALSE;
}

STDMETHODIMP CSilverlightSourceFilter::IsFormatSupported(const GUID* pFormat)
{
	if(pFormat == NULL) return E_POINTER;

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CSilverlightSourceFilter::QueryPreferredFormat(GUID* pFormat)
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CSilverlightSourceFilter::GetTimeFormat(GUID* pFormat)
{
	if(pFormat == NULL) return E_POINTER;

	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP CSilverlightSourceFilter::IsUsingTimeFormat(const GUID* pFormat)
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CSilverlightSourceFilter::SetTimeFormat(const GUID* pFormat)
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CSilverlightSourceFilter::GetDuration(LONGLONG* pDuration)
{
	if(pDuration == NULL)
	{
		return E_POINTER;
	}
	
	*pDuration = m_media.m_duration;

	return S_OK;
}

STDMETHODIMP CSilverlightSourceFilter::GetStopPosition(LONGLONG* pStop)
{
	if(pStop == NULL)
	{
		return E_POINTER;
	}

	*pStop = m_media.m_duration;

	return S_OK;
}

STDMETHODIMP CSilverlightSourceFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSilverlightSourceFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSilverlightSourceFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	if(ThreadExists())
	{
		for(auto i = m_outputs.begin(); i != m_outputs.end(); i++)
		{
			(*i)->DeliverBeginFlush();
		}

		CallWorker(CMD_FLUSH);

		m_current = *pCurrent;

		CallWorker(CMD_START);
	}

	return S_OK;
}

STDMETHODIMP CSilverlightSourceFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	if(pCurrent == NULL || pStop == NULL)
	{
		return E_POINTER;
	}

	*pCurrent = m_current;
	*pStop = m_media.m_duration;

	return S_OK;
}

STDMETHODIMP CSilverlightSourceFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	if(pEarliest == NULL || pLatest == NULL)
	{
		return E_POINTER;
	}

	*pEarliest = 0;
	*pLatest = m_media.m_duration;

	return S_OK;
}

STDMETHODIMP CSilverlightSourceFilter::SetRate(double dRate)
{
	return dRate == 1.0 ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CSilverlightSourceFilter::GetRate(double* pdRate)
{
	if(pdRate == NULL)
	{
		return E_POINTER;
	}

	*pdRate = 1.0;

	return S_OK;
}

STDMETHODIMP CSilverlightSourceFilter::GetPreroll(LONGLONG* pllPreroll)
{
	if(pllPreroll == NULL)
	{
		return E_POINTER;
	}

	*pllPreroll = 0;

	return S_OK;
}

// CSilverlightSourceOutputPin

CSilverlightSourceFilter::CSilverlightSourceOutputPin::CSilverlightSourceOutputPin(CSilverlightSourceFilter* pFilter, const std::string& url, Silverlight::StreamIndex* stream, HRESULT* phr)
	: CBaseOutputPin(NAME("CSilverlightSourceOutputPin"), pFilter, pFilter, phr, L"Output")
	, m_queue(100)
	, m_flushing(false)
	, m_url(url)
	, m_stream(stream)
	, m_file(NULL)
	, m_mdat_pos(0)
{
	*phr = E_FAIL;

	if(m_stream->m_levels.empty())
	{
		return;
	}

	Silverlight::QualityLevel* level = &m_stream->m_levels.front();

	int bitrate = 0;

	for(auto i = m_stream->m_levels.begin(); i != m_stream->m_levels.end(); i++)
	{
		if(bitrate < i->m_bitrate)
		{
			bitrate = i->m_bitrate;

			level = &*i;
		}
	}

	m_url += "/";
	m_url += m_stream->m_url;

	Util::Replace(m_url, "{bitrate}", Util::Format("%d", level->m_bitrate).c_str());

	std::vector<BYTE> buff;

	Util::ToBin(std::wstring(level->m_private.begin(), level->m_private.end()).c_str(), buff);

	if(m_stream->m_type == "video")
	{
		m_mt.majortype = MEDIATYPE_Video;
		m_mt.subtype = FOURCCMap(level->GetFOURCC());
		m_mt.formattype = FORMAT_VideoInfo;

		if(m_mt.subtype == MEDIASUBTYPE_H264)
		{
			m_h264prefix.swap(buff);
		}

		int size = sizeof(VIDEOINFOHEADER) + buff.size();
	
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)m_mt.AllocFormatBuffer(size);
		
		memset(vih, 0, size);
		memcpy(vih + 1, buff.data(), buff.size());

		vih->bmiHeader.biSize = sizeof(vih->bmiHeader);
		vih->bmiHeader.biWidth = level->m_width;
		vih->bmiHeader.biHeight = level->m_heigth;
		vih->bmiHeader.biCompression = level->GetFOURCC();
		vih->dwBitRate = level->m_bitrate;

		*phr = S_OK;
	}
	else if(m_stream->m_type == "audio")
	{
		int tag = level->m_tag;

		if(level->m_fourcc == "AACL")
		{
			tag = WAVE_FORMAT_AAC; // FIXME

			if(buff.empty())
			{
				BYTE tmp[8];
				
				buff.resize(DirectShow::AACInitData(tmp, 0, level->m_freq, level->m_channels));

				memcpy(buff.data(), tmp, buff.size());
			}
		}

		m_mt.majortype = MEDIATYPE_Audio;
		m_mt.subtype = FOURCCMap(tag);
		m_mt.formattype = FORMAT_WaveFormatEx;

		int size = sizeof(WAVEFORMATEX) + buff.size();
	
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_mt.AllocFormatBuffer(size);
		
		memset(wfe, 0, size);
		memcpy(wfe + 1, buff.data(), buff.size());

		wfe->wFormatTag = tag;
		wfe->nAvgBytesPerSec = level->m_bitrate / 8;
		wfe->nBlockAlign = level->m_packetsize;
		wfe->nChannels = level->m_channels;
		wfe->nSamplesPerSec = level->m_freq;
		wfe->wBitsPerSample = level->m_bps;
		wfe->cbSize = buff.size();

		*phr = S_OK;
	}
}

CSilverlightSourceFilter::CSilverlightSourceOutputPin::~CSilverlightSourceOutputPin()
{
	delete m_file;
}

HRESULT CSilverlightSourceFilter::CSilverlightSourceOutputPin::Active()
{
	HRESULT hr;

	hr = __super::Active();

	m_chunks.clear();
	m_samples.clear();

	CAMThread::Create();

	return hr;
}

HRESULT CSilverlightSourceFilter::CSilverlightSourceOutputPin::Inactive()
{
	m_exit.Set();

	CAMThread::Close();

	delete m_file;

	m_file = NULL;

	return __super::Inactive();
}

HRESULT CSilverlightSourceFilter::CSilverlightSourceOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, float dRate)
{
	m_start = tStart;

	//REFERENCE_TIME rt = 0;

	m_chunks.clear();

	for(auto i = m_stream->m_chunks.begin(); i != m_stream->m_chunks.end(); i++)
	{
		if(*i >= tStart)
		{
			m_chunks.push_back(*i);
		}
		/*
		REFERENCE_TIME next = rt + *i;

		if(next > tStart)
		{
			m_chunks.push_back(rt);
		}

		rt = next;
		*/
	}

	//m_chunks.push_back(rt);

	m_samples.clear();

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

HRESULT CSilverlightSourceFilter::CSilverlightSourceOutputPin::Deliver(QueueItem* item)
{
	if(m_flushing) {delete item; return S_FALSE;}

	m_queue.Enqueue(item);

	return S_OK;
}

HRESULT CSilverlightSourceFilter::CSilverlightSourceOutputPin::DeliverBeginFlush()
{
	m_flushing = true;

	return __super::DeliverBeginFlush();
}

HRESULT CSilverlightSourceFilter::CSilverlightSourceOutputPin::DeliverEndFlush()
{
	m_queue.GetDequeueEvent().Wait();

	m_flushing = false;

	return __super::DeliverEndFlush();
}

HRESULT CSilverlightSourceFilter::CSilverlightSourceOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return m_mt == *pmt ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CSilverlightSourceFilter::CSilverlightSourceOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_mt;

	return S_OK;
}

HRESULT CSilverlightSourceFilter::CSilverlightSourceOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1 << 20;

	return S_OK;
}

bool CSilverlightSourceFilter::CSilverlightSourceOutputPin::GetNextSampleStart(REFERENCE_TIME& next)
{
	if(m_samples.empty())
	{
		if(m_chunks.empty())
		{
			return false;
		}

		REFERENCE_TIME start = m_chunks.front();

		delete m_file; m_file = NULL;

		std::string url = m_url;

		Util::Replace(url, "{start time}", Util::Format("%I64d", start + m_stream->m_start).c_str());

		CHttpStream* s = new CHttpStream();

		int i = 0;

		while(!s->Load(url.c_str()))
		{
			printf("cannot load fragment at %s (try %d)\n", url.c_str(), i);

			if(++i == 5)
			{
				delete s;

				return false;
			}

			Sleep(100);
		}

		HRESULT hr;

		m_file = new CBaseSplitterFile(new CAsyncHttpStreamReader(s), hr);

		ParseMOOF(start);

		m_chunks.pop_front();

		// remove elements from m_samples until the first syncpoint before m_start
		
		for(auto i = m_samples.begin(), j = m_samples.begin(); i != m_samples.end(); i++)
		{
			if(i->start >= m_start)
			{
				if(i != m_samples.begin() && j != m_samples.begin())
				{
					m_samples.erase(m_samples.begin(), j);
				}

				break;
			}

			if(i->syncpoint)
			{
				j = i;
			}
		}
	}

	if(m_samples.empty())
	{
		return false;
	}

	next = m_samples.front().start;

	return true;
}

#include <openssl/aes.h>

void CSilverlightSourceFilter::CSilverlightSourceOutputPin::QueueNextSample()
{
	if(!m_samples.empty())
	{
		QueueItem* item = new QueueItem();

		item->sample = m_samples.front();

		m_file->Seek(m_mdat_pos + item->sample.pos);

		CBaseSplitterFile* f = m_file;

		std::vector<BYTE> src, dst;

		if(m_playready.m_algorithm != Silverlight::PlayReady::ALGORITHM_NONE)
		{
			src.resize(item->sample.size);
			dst.resize(item->sample.size);

			f->ByteRead(src.data(), src.size());

			//

			BYTE secret[16] = {0}; // TODO

			AES_KEY key;
			
			memset(&key, 0, sizeof(key));

			AES_set_encrypt_key(secret, 128, &key);

			BYTE iv[16];
            BYTE ecount[16];
            unsigned int num;

			memcpy(iv, item->sample.iv, 16);
			memset(ecount, 0, sizeof(ecount));
			num = 0;

			//

			int pos = 0;

			for(auto i = item->sample.subsamples.begin(); i != item->sample.subsamples.end(); i++)
			{
				memcpy(&dst[pos], &src[pos], i->clear);

				pos += i->clear;

				AES_ctr128_encrypt(&src[pos], &dst[pos], i->encrypted, &key, iv, ecount, &num);

				pos += i->encrypted;
			}

			f = new CBaseSplitterFile(dst.data(), dst.size());
		}

		if(!m_h264prefix.empty())
		{
			item->buff.resize(m_h264prefix.size() + item->sample.size);

			BYTE* p = item->buff.data();

			memcpy(p, m_h264prefix.data(), m_h264prefix.size());

			p += m_h264prefix.size();

			for(DWORD i = 0, size; i < item->sample.size; i += 4 + size)
			{
				p[i + 0] = 0;
				p[i + 1] = 0;
				p[i + 2] = 0; 
				p[i + 3] = 1;
			
				size = (DWORD)f->BitRead(32);

				f->ByteRead(&p[i + 4], size);
			}

			item->sample.size += m_h264prefix.size();
		}
		else
		{
			item->buff.resize(item->sample.size);

			f->ByteRead(item->buff.data(), item->sample.size);
		}

		if(f != m_file) delete f;

		Deliver(item);

		m_samples.pop_front();
	}
}

void CSilverlightSourceFilter::CSilverlightSourceOutputPin::ParseMOOF(REFERENCE_TIME start)
{
	m_samples.clear();
	m_playready = ((CSilverlightSourceFilter*)m_pFilter)->m_media.m_playready;

	UINT64 base_data_offset = 0;
	DWORD sample_description_index = 0;
	DWORD default_sample_duration = 0;
	DWORD default_sample_size = 0;
	DWORD default_sample_flags = 0;

	int data_offset = 0;
	DWORD first_sample_flags = 0;

	CBaseSplitterFile::MP4Box h;

	while(m_file->Read(h))
	{
		if(h.tag == 'moof')
		{
			CBaseSplitterFile::MP4Box h2;

			while(m_file->Read(h2))
			{
				if(h2.tag == 'mfhd')
				{
				}
				else if(h2.tag == 'traf')
				{
					CBaseSplitterFile::MP4Box h3;

					while(m_file->Read(h3))
					{
						if(h3.tag == 'tfhd')
						{
							DWORD flags = (DWORD)m_file->BitRead(32) & 0xffffff;

							m_file->BitRead(32); // track id

							if(flags & 0x01) base_data_offset = m_file->BitRead(64);
							if(flags & 0x02) sample_description_index = (DWORD)m_file->BitRead(32);
							if(flags & 0x08) default_sample_duration = (DWORD)m_file->BitRead(32);
							if(flags & 0x10) default_sample_size = (DWORD)m_file->BitRead(32);
							if(flags & 0x20) default_sample_flags = (DWORD)m_file->BitRead(32);
						}
						else if(h3.tag == 'trun')
						{
							DWORD flags = (DWORD)m_file->BitRead(32) & 0xffffff;

							DWORD sample_count = (DWORD)m_file->BitRead(32);

							if(flags & 0x01) data_offset = (int)m_file->BitRead(32);
							if(flags & 0x04) first_sample_flags = (DWORD)m_file->BitRead(32);

							// m_mdat_offset = base_data_offset + data_offset;

							Sample s;
							
							s.start = start;
							s.pos = 0;

							UINT64 sample_offset = 0;

							for(DWORD i = 0; i < sample_count; i++)
							{
								DWORD sample_duration = (flags & 0x0100) ? (DWORD)m_file->BitRead(32) : default_sample_duration;
								DWORD sample_size = (flags & 0x0200) ? (DWORD)m_file->BitRead(32) : default_sample_size;
								DWORD sample_flags = (flags & 0x0400) ? (DWORD)m_file->BitRead(32) : default_sample_flags;
								DWORD sample_composition_time_offset = (flags & 0x0800) ? (DWORD)m_file->BitRead(32) : 0;

								if(i == 0 && (flags & 0x04))
								{
									sample_flags = first_sample_flags;
								}

								s.duration = sample_duration;
								s.size = sample_size;
								s.syncpoint = true;

								m_samples.push_back(s);

								s.start += s.duration;
								s.pos += s.size;
							}
						}
						else if(h3.tag == 'sdtp')
						{
							m_file->BitRead(32);

							for(auto i = m_samples.begin(); i != m_samples.end(); i++)
							{
								BYTE flags = (BYTE)m_file->BitRead(8);

								i->syncpoint = ((flags >> 4) & 3) == 2;
							}
						}
						else if(h3.tag == 'uuid')
						{
							GUID CLSID_SampleEncryptionBox;

							Util::ToCLSID(L"{A2394F52-5A9B-4f14-A244-6C427C648DF4}", CLSID_SampleEncryptionBox);
							
							if(h3.id == CLSID_SampleEncryptionBox)
							{
								DWORD flags = (DWORD)m_file->BitRead(32) & 0xffffff;

								if(flags & 1)
								{
									m_playready.m_algorithm = (int)m_file->BitRead(24);
									m_playready.m_iv_size = (int)m_file->BitRead(8);
									m_file->ByteRead(m_playready.m_kid, sizeof(16));

									ASSERT(0); // TODO
								}

								if((m_playready.m_algorithm == Silverlight::PlayReady::ALGORITHM_CTR || m_playready.m_algorithm == Silverlight::PlayReady::ALGORITHM_CBC)
								&& (m_playready.m_iv_size == 8 || m_playready.m_iv_size == 16))
								{
									if(m_playready.m_algorithm == Silverlight::PlayReady::ALGORITHM_CBC) 
									{
										ASSERT(m_playready.m_iv_size == 16);
									}

									DWORD sample_count = (DWORD)m_file->BitRead(32);

									ASSERT(sample_count == m_samples.size());

									auto s = m_samples.begin();

									for(DWORD i = 0; i < sample_count && s != m_samples.end(); i++, s++)
									{
										m_file->ByteRead(s->iv, m_playready.m_iv_size);

										if(m_playready.m_algorithm == Silverlight::PlayReady::ALGORITHM_CTR && m_playready.m_iv_size == 8)
										{
											memset(&s->iv[8], 0, 8);

											// TODO: increment counter? 
										}

										if(flags & 2)
										{
											WORD entries = (WORD)m_file->BitRead(16);

											s->subsamples.resize(entries);

											for(WORD j = 0; j < entries; j++)
											{
												s->subsamples[j].clear = (int)m_file->BitRead(16);
												s->subsamples[j].encrypted = (int)m_file->BitRead(32);
											}
										}
										else
										{
											s->subsamples.resize(1);

											s->subsamples[0].clear = 0;
											s->subsamples[0].encrypted = s->size;
										}
									}
								}
							}
						}
					}

					m_file->Seek(h3.next);
				}

				m_file->Seek(h2.next);
			}
		}
		else if(h.tag == 'mdat')
		{
			m_mdat_pos = m_file->GetPos();
		}

		m_file->Seek(h.next);
	}
}

//static FILE* s_fp = fopen("c:/1.txt", "w");

DWORD CSilverlightSourceFilter::CSilverlightSourceOutputPin::ThreadProc()
{
	HRESULT hr;
	
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
				QueueItem* item = m_queue.Dequeue();

				int size = item->buff.size();

				if(!m_flushing)
				{
					CComPtr<IMediaSample> sample;

					hr = GetDeliveryBuffer(&sample, NULL, NULL, 0);

					if(SUCCEEDED(hr) && sample->GetSize() >= size)
					{
						BYTE* dst = NULL;
						
						hr = sample->GetPointer(&dst);

						if(SUCCEEDED(hr) && dst != NULL)
						{
							memcpy(dst, item->buff.data(), size);

							sample->SetActualDataLength(size);
				
							REFERENCE_TIME start = item->sample.start - m_start;
							REFERENCE_TIME stop = item->sample.start + item->sample.duration - m_start;

							sample->SetTime(&start, &stop);
							sample->SetMediaTime(NULL, NULL);

							sample->SetDiscontinuity(FALSE);
							sample->SetSyncPoint(item->sample.syncpoint);
							sample->SetPreroll(stop < 0);
							/*
							fprintf(s_fp, "d [%p] %d %lld - %lld\n", this, item->sample.syncpoint ? 1 : 0, start / 10000, stop / 10000);
							fflush(s_fp);
							*/
							__super::Deliver(sample);
						}
					}
				}

				delete item;
			}

			break;
		}
	}

	return 0;
}
