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
#include "DSMSplitter.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

// CDSMSplitterFilter

CDSMSplitterFilter::CDSMSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CDSMSplitterFilter"), pUnk, phr, __uuidof(this))
	, m_file(NULL)
{
}

CDSMSplitterFilter::~CDSMSplitterFilter()
{
	delete m_file;
}

HRESULT CDSMSplitterFilter::CreateOutputPins(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	delete m_file;

	m_file = new CDSMSplitterFile(pAsyncReader, hr, *this, *this);

	if(FAILED(hr)) 
	{
		delete m_file;

		m_file = NULL;

		return hr;
	}

	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = m_file->m_time.dur;

	std::vector<BYTE> ids;

	for(auto i = m_file->m_mts.begin(); i != m_file->m_mts.end(); i++)
	{
		ids.push_back(i->first);
	}

	std::sort(ids.begin(), ids.end());

	for(auto i = ids.begin(); i != ids.end(); i++)
	{
		BYTE id = *i;

		//const 
		CMediaType& mt = m_file->m_mts[id];

		if(mt.lSampleSize == 1)
		{
			mt.lSampleSize = 1 << 20;
		}

		std::wstring name = Util::Format(L"Output %02d", id);

		if(mt.majortype == MEDIATYPE_VBI) name = L"~" + name;

		std::vector<CMediaType> mts(1, mt);

		CBaseSplitterOutputPin* pPinOut = new CBaseSplitterOutputPin(mts, name.c_str(), this, &hr);
	
		name.clear();
		std::wstring lang;

		for(auto i = m_file->m_sim[id].begin(); i != m_file->m_sim[id].end(); i++)
		{
			const std::wstring& key = i->first;
			const std::wstring& value = i->second;

			pPinOut->SetProperty(key.c_str(), value.c_str());

			if(key == L"NAME") 
			{
				name = value;
			}

			if(key == L"LANG") 
			{
				lang = Util::ISO6392ToLanguage(value.c_str());

				if(lang.empty())
				{
					lang = value;
				}
			}
		}

		if(!name.empty() || !lang.empty())
		{
			if(!name.empty()) {if(!lang.empty()) name += L" (" + lang + L")";}
			else if(!lang.empty()) name = lang;
			pPinOut->SetName(name.c_str());
		}

		EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(id, pPinOut)));
	}

	for(auto i = m_file->m_fim.begin(); i != m_file->m_fim.end(); i++)
	{
		SetProperty(i->first.c_str(), i->second.c_str());
	}

	for(auto i = m_resources.begin(); i != m_resources.end(); i++)
	{
		if(i->m_mime == L"application/x-truetype-font")
		{
			m_font.Install(i->m_data);
		}
	}

	return !m_pOutputs.empty() ? S_OK : E_FAIL;
}

bool CDSMSplitterFilter::DemuxInit()
{
	return true;
}

void CDSMSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	m_file->Seek(m_file->FindSyncPoint(rt));
}

bool CDSMSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_file->GetRemaining())
	{
		dsmp_t type;
		UINT64 len;

		if(!m_file->Sync(type, len))
		{
			continue;
		}

		__int64 pos = m_file->GetPos();

		if(type == DSMP_SAMPLE)
		{
			CPacket* p = new CPacket();

			if(m_file->Read(len, p))
			{
				if(p->flags & CPacket::TimeValid)
				{
					p->start -= m_file->m_time.first;
					p->stop -= m_file->m_time.first;
				}

				//printf("[%d] %lld %lld\n", p->id, p->start, p->stop);

				hr = DeliverPacket(p);
			}
			else
			{
				delete p;
			}
		}

		m_file->Seek(pos + len);
	}

	return true;
}

// IKeyFrameInfo

STDMETHODIMP CDSMSplitterFilter::GetKeyFrameCount(UINT& nKFs)
{
	CheckPointer(m_file, E_UNEXPECTED);

	nKFs = m_file->m_sps.size();

	return S_OK;
}

STDMETHODIMP CDSMSplitterFilter::GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs)
{
	CheckPointer(pFormat, E_POINTER);
	CheckPointer(pKFs, E_POINTER);
	CheckPointer(m_file, E_UNEXPECTED);

	if(*pFormat != TIME_FORMAT_MEDIA_TIME) 
	{
		return E_INVALIDARG;
	}

	// these aren't really the keyframes, but quicky accessable points in the stream

	nKFs = m_file->m_sps.size();

	for(int i = 0, j = m_file->m_sps.size(); i < j; i++)
	{
		pKFs[i] = m_file->m_sps[i].m_rt;
	}

	return S_OK;
}

// CDSMSourceFilter

CDSMSourceFilter::CDSMSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CDSMSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);

	delete m_pInput;

	m_pInput = NULL;
}
