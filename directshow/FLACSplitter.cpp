#include "StdAfx.h"
#include "FLACSplitter.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

// CFLACSplitterFilter

CFLACSplitterFilter::CFLACSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CFLACSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_file(NULL)
{
}

CFLACSplitterFilter::~CFLACSplitterFilter()
{
	delete m_file;
}

STDMETHODIMP CFLACSplitterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CFLACSplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr;

	delete m_file;

	m_file = new CBaseSplitterFile(pAsyncReader, hr);

	memset(&m_frame, 0, sizeof(m_frame));

	m_index.clear();
	
	m_file->Seek(0);

	if(m_file->BitRead(32) != 'fLaC')
	{
		return E_FAIL;
	}

	while(m_file->GetRemaining() > 0)
	{
		bool last = m_file->BitRead(1) != 0;

		BYTE type = (BYTE)m_file->BitRead(7);
		DWORD size = (DWORD)m_file->BitRead(24);

		__int64 next = m_file->GetPos() + size;

		std::string s;

		switch(type)
		{
		case 0: 
			
			if(!m_file->Read(m_stream)) return E_FAIL; 
			
			break;

		case 3: 

			m_index.resize(size / 18);

			for(size_t i = 0; i < m_index.size(); i++)
			{
				m_index[i].pos = m_file->BitRead(64);
				m_index[i].offset = m_file->BitRead(64);
				m_file->BitRead(16); // samples
				
				if(m_index[i].pos == 0xffffffffffffffffui64)
				{
					m_index.resize(i);

					break;
				}
			}

			break;

		case 4:

			m_file->ByteRead((BYTE*)&size, 4);
			if(m_file->GetPos() + size > next) break;
			m_file->ReadString(s, size);
				
			m_file->ByteRead((BYTE*)&size, 4);

			for(DWORD i = size; i > 0; i--)
			{
				m_file->ByteRead((BYTE*)&size, 4);
				if(m_file->GetPos() + size > next) break;
				m_file->ReadString(s, size);

				SetProperty(L"APPL", Util::UTF8To16(s.c_str()).c_str());

				std::string::size_type j = s.find('=');

				if(j != std::string::npos && j < s.size() - 1)
				{
					std::string k = s.substr(0, j);
					std::wstring v = Util::UTF8To16(&s[j + 1]);

					if(k == "TITLE") SetProperty(L"TITL", v.c_str());
					else if(k == "ARTIST") SetProperty(L"AUTH", v.c_str());
					else if(k == "COMMENT") SetProperty(L"DESC", v.c_str());
				}
			}

			break;

		case 6:

			if(m_file->BitRead(32) == 3)
			{
				std::wstring mime, desc;
				std::vector<BYTE> buff;

				size = (DWORD)m_file->BitRead(32);
				if(m_file->GetPos() + size > next) break;
				m_file->ReadString(s, size);
				mime = Util::UTF8To16(s.c_str());

				size = (DWORD)m_file->BitRead(32);
				if(m_file->GetPos() + size > next) break;
				m_file->ReadString(s, size);
				desc = Util::UTF8To16(s.c_str());

				m_file->Seek(m_file->GetPos() + 16); // w, h, bpp, pal

				size = (DWORD)m_file->BitRead(32);
				if(m_file->GetPos() + size > next) break;
				buff.resize(size);
				m_file->ByteRead(buff.data(), buff.size());

				ResAppend(L"Cover (front)", desc.c_str(), mime.c_str(), buff.data(), buff.size());
			}

			break;
		}

		m_file->Seek(next);

		if(last) break;
	}

	if(m_stream.spb_min < 16 || m_stream.spb_max < 16 
	|| m_stream.spb_min > m_stream.spb_max
	|| m_stream.fs_min > m_stream.fs_max
	|| m_stream.freq == 0 
	|| m_stream.bits < 4 || m_stream.bits > 32 
	|| m_stream.channels < 1 || m_stream.channels > 8)
	{
		return E_FAIL;
	}

	int size = (int)m_file->GetPos();

	CMediaType mt;

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_FLAC_FRAMED;
	mt.formattype = FORMAT_WaveFormatEx; 

	WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + size);

	memset(wfe, 0, sizeof(WAVEFORMATEX));

	wfe->wFormatTag = WAVE_FORMAT_FLAC;
	wfe->nSamplesPerSec = m_stream.freq;
	wfe->wBitsPerSample = m_stream.bits;
	wfe->nChannels = m_stream.channels;
	wfe->cbSize = (WORD)size;

	m_file->Seek(0);

	m_file->ByteRead((BYTE*)(wfe + 1), size);

	std::vector<CMediaType> mts;
	
	mts.push_back(mt);

	AddOutputPin(0, new CBaseSplitterOutputPin(mts, L"Output", this, &hr));

	if(!m_file->Read(m_frame, m_stream))
	{
		return E_FAIL;
	}

	if(m_stream.spb_min == m_stream.spb_max)
	{
		UINT64 samples = 0;

		m_file->Seek(m_file->GetLength() - m_stream.fs_max);

		CBaseSplitterFile::FLACFrameHeader h;

		while(m_file->Read(h, m_stream))
		{
			samples = h.pos - m_frame.pos + h.samples;
		}

		if(samples > 0 && (samples < m_stream.samples || samples > m_stream.samples + m_stream.spb_min))
		{
			m_stream.samples = samples;
		}
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtDuration = m_rtNewStop = m_rtStop = 10000000i64 * m_stream.samples / m_stream.freq;

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

bool CFLACSplitterFilter::DemuxInit()
{
	return true;
}

void CFLACSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	m_file->Seek(m_frame.start);

	if(!m_index.empty())
	{
		UINT64 sample = rt * m_stream.freq / 10000000i64;

		for(auto i = m_index.rbegin(); i != m_index.rend(); i++) // TODO: bsearch
		{
			if(i->pos <= sample)
			{
				m_file->Seek(m_frame.start + i->offset);

				break;
			}
		}
	}
	else
	{
		// TODO
	}
}

bool CFLACSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_file->GetRemaining() >= m_stream.fs_min)
	{
		CBaseSplitterFile::FLACFrameHeader h[2];

		if(m_file->Read(h[0], m_stream))
		{
			m_file->Seek(h[0].start + m_stream.fs_min);

			if(m_file->Read(h[1], m_stream))
			{
				m_file->Seek(h[0].start);

				int size = (int)(h[1].start - h[0].start);

				CPacket* p = new CPacket(size);

				m_file->ByteRead(&p->buff[0], size);

				DWORD freq = h[0].freq ? h[0].freq : m_stream.freq;

				REFERENCE_TIME start = 10000000i64 * (h[0].pos - m_frame.pos) / freq;
				REFERENCE_TIME dur = 10000000i64 * h[0].samples / freq;

				p->id = 0;
				p->start = start;
				p->stop = start + dur;
				p->flags |= CPacket::SyncPoint | CPacket::TimeValid;

				hr = DeliverPacket(p);
			}
		}
	}

	return true;
}

// CFLACSourceFilter

CFLACSourceFilter::CFLACSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CFLACSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);

	delete m_pInput;

	m_pInput = NULL;
}
