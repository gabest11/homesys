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
#include "SubInputPin.h"
#include "DirectShow.h"
#include "TextSubProvider.h"
#include "VobSubProvider.h"
#include "moreuuids.h"

// our first format id

#define __GAB1__ "GAB1"

// same as __GAB1__, but the size is (uint) and only __GAB1_LANGUAGE_UNICODE__ is valid

#define __GAB2__ "GAB2"

// our tags for __GAB1__ (ushort) + size (ushort)

enum
{
	__GAB1_LANGUAGE__  = 0, // "lang" + '0'
	__GAB1_ENTRY__ = 1, // (int)start + (int)stop + (char*)line + '0'
	__GAB1_LANGUAGE_UNICODE__ = 2, // L"lang" + '0'
	__GAB1_ENTRY_UNICODE__ = 3, // (int)start + (int)stop + (WCHAR*)line + '0'
	__GAB1_RAWTEXTSUBTITLE__ = 4, // (BYTE*)
};

CSubInputPin::CSubInputPin(CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr)
	: CBaseInputPin(NAME("CSubInputPin"), pFilter, pLock, phr, L"Input")
{
	m_bCanReconnectWhenActive = TRUE;
}

HRESULT CSubInputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Text && (pmt->subtype == MEDIASUBTYPE_NULL || pmt->subtype == FOURCCMap((DWORD)0))
		|| pmt->majortype == MEDIATYPE_Subtitle && pmt->subtype == MEDIASUBTYPE_UTF8
		|| pmt->majortype == MEDIATYPE_Subtitle && (pmt->subtype == MEDIASUBTYPE_SSA || pmt->subtype == MEDIASUBTYPE_ASS || pmt->subtype == MEDIASUBTYPE_ASS2)
		|| pmt->majortype == MEDIATYPE_Subtitle && pmt->subtype == MEDIASUBTYPE_SSF
		|| pmt->majortype == MEDIATYPE_Subtitle && (pmt->subtype == MEDIASUBTYPE_VOBSUB)
		? S_OK 
		: E_FAIL;
}

HRESULT CSubInputPin::CompleteConnect(IPin* pReceivePin)
{
	m_name.clear();
	m_init.clear();

	if(m_mt.majortype == MEDIATYPE_Text)
	{
		m_name = DirectShow::GetName(pReceivePin) + L" (embeded)";

		CComPtr<ITextSubStream> ss = new CTextSubProvider();

		if(SUCCEEDED(ss->OpenText(m_name.c_str())))
		{
			m_ss = ss;
		}
	}
	else if(m_mt.majortype == MEDIATYPE_Subtitle)
	{
		SUBTITLEINFO* psi = (SUBTITLEINFO*)m_mt.pbFormat;

		std::string lang = psi->IsoLang;

		m_name = Util::ISO6392ToLanguage(std::wstring(lang.begin(), lang.end()).c_str());

		if(m_name.empty())
		{
			m_name = L"English";
		}
		
		if(wcslen(psi->TrackName) > 0) 
		{
			m_name += L" (" + std::wstring(psi->TrackName) + L")";
		}

		DWORD dwOffset = psi->dwOffset;

		std::string s((char*)m_mt.pbFormat + dwOffset, m_mt.cbFormat - dwOffset);

		m_init = Util::UTF8To16(s.c_str());

		if(m_mt.subtype == MEDIASUBTYPE_UTF8)
		{
			CComPtr<ITextSubStream> ss = new CTextSubProvider();

			if(SUCCEEDED(ss->OpenText(m_name.c_str())))
			{
				m_ss = ss;
			}
		}
		else if(m_mt.subtype == MEDIASUBTYPE_SSA || m_mt.subtype == MEDIASUBTYPE_ASS || m_mt.subtype == MEDIASUBTYPE_ASS2)
		{
			CComPtr<ITextSubStream> ss = new CTextSubProvider();

			if(SUCCEEDED(ss->OpenSSA(m_init.c_str(), m_name.c_str())))
			{
				m_ss = ss;
			}
		}
		else if(m_mt.subtype == MEDIASUBTYPE_SSF)
		{
			CComPtr<ITextSubStream> ss = new CTextSubProvider();

			if(SUCCEEDED(ss->OpenSSF(m_init.c_str(), m_name.c_str())))
			{
				m_ss = ss;
			}
		}
		else if(m_mt.subtype == MEDIASUBTYPE_VOBSUB)
		{
			CComPtr<IVobSubStream> vs = new CVobSubProvider();

			if(SUCCEEDED(vs->OpenIndex(m_init.c_str(), m_name.c_str())))
			{
				m_ss = vs;
			}
		}
	}

	if(m_ss != NULL)
	{
		AddSubStream(m_ss);
	}
	else
	{
		return E_FAIL;
	}

    return __super::CompleteConnect(pReceivePin);
}

HRESULT CSubInputPin::BreakConnect()
{
	RemoveSubStream(m_ss);

	m_ss = NULL;
	m_name.clear();
	m_init.clear();

	ASSERT(IsStopped());

    return __super::BreakConnect();
}

STDMETHODIMP CSubInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	if(m_Connected)
	{
		RemoveSubStream(m_ss);

		m_ss = NULL;

        m_Connected->Release();
        m_Connected = NULL;
	}

	return __super::ReceiveConnection(pConnector, pmt);
}

STDMETHODIMP CSubInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	CAutoLock cAutoLock(&m_csReceive);

	if(m_mt.majortype == MEDIATYPE_Text || m_mt.majortype == MEDIATYPE_Subtitle && m_mt.subtype == MEDIASUBTYPE_UTF8)
	{
		CComQIPtr<ITextSubStream>(m_ss)->OpenText(m_name.c_str());
	}
	else if(m_mt.majortype == MEDIATYPE_Subtitle)
	{
		if(m_mt.subtype == MEDIASUBTYPE_SSA || m_mt.subtype == MEDIASUBTYPE_ASS || m_mt.subtype == MEDIASUBTYPE_ASS2)
		{
			CComQIPtr<ITextSubStream>(m_ss)->OpenSSA(m_init.c_str(), m_name.c_str());
		}
		else if(m_mt.majortype == MEDIATYPE_Subtitle && m_mt.subtype == MEDIASUBTYPE_SSF)
		{
			CComQIPtr<ITextSubStream>(m_ss)->OpenSSF(m_init.c_str(), m_name.c_str());
		}
		else if(m_mt.majortype == MEDIATYPE_Subtitle && m_mt.subtype == MEDIASUBTYPE_VOBSUB)
		{
			CComQIPtr<IVobSubStream>(m_ss)->RemoveAll();
		}
	}

	return __super::NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CSubInputPin::Receive(IMediaSample* pSample)
{
	HRESULT hr;

	hr = __super::Receive(pSample);

    if(FAILED(hr)) return hr;

	CAutoLock cAutoLock(&m_csReceive);

	REFERENCE_TIME start, stop;

    pSample->GetTime(&start, &stop);

	start += m_tStart; 
	stop += m_tStart;

	BYTE* src = NULL;

    hr = pSample->GetPointer(&src);

    if(FAILED(hr) || src == NULL) 
	{
		return hr;
	}

	int len = pSample->GetActualDataLength();

	bool invalidate = false;

	if(m_mt.majortype == MEDIATYPE_Text)
	{
		CComQIPtr<ITextSubStream> ss = m_ss;

		if(!strncmp((char*)src, __GAB1__, strlen(__GAB1__)))
		{
			char* ptr = (char*)&src[strlen(__GAB1__) + 1];
			char* end = (char*)&src[len];

			while(ptr < end)
			{
				WORD tag = *((WORD*)(ptr)); ptr += 2;
				WORD size = *((WORD*)(ptr)); ptr += 2;

				if(tag == __GAB1_LANGUAGE__)
				{
					m_name = Util::ConvertMBCS(ptr, ANSI_CHARSET);
				}
				else if(tag == __GAB1_LANGUAGE_UNICODE__)
				{
					m_name = (WCHAR*)ptr;
				}
				else if(tag == __GAB1_ENTRY__)
				{
					REFERENCE_TIME start = (REFERENCE_TIME)((int*)ptr)[0] * 10000;
					REFERENCE_TIME stop = (REFERENCE_TIME)((int*)ptr)[1] * 10000;

					ss->AppendText(start, stop, Util::ConvertMBCS(ptr + 8, ANSI_CHARSET).c_str());

					invalidate = true;
				}
				else if(tag == __GAB1_ENTRY_UNICODE__)
				{
					REFERENCE_TIME start = (REFERENCE_TIME)((int*)ptr)[0] * 10000;
					REFERENCE_TIME stop = (REFERENCE_TIME)((int*)ptr)[1] * 10000;

					ss->AppendText(start, stop, (LPCWSTR)(ptr + 8));

					invalidate = true;
				}

				ptr += size;
			}
		}
		else if(!strncmp((char*)src, __GAB2__, strlen(__GAB2__)))
		{
			char* ptr = (char*)&src[strlen(__GAB2__) + 1];
			char* end = (char*)&src[len];

			while(ptr < end)
			{
				WORD tag = *((WORD*)(ptr)); ptr += 2;
				DWORD size = *((DWORD*)(ptr)); ptr += 4;

				if(tag == __GAB1_LANGUAGE_UNICODE__)
				{
					m_name = (WCHAR*)ptr;
				}
				else if(tag == __GAB1_RAWTEXTSUBTITLE__)
				{
					ss->OpenMemory((BYTE*)ptr, size, m_name.c_str());

					invalidate = true;
				}

				ptr += size;
			}
		}
		else if(src != 0 && len > 1 && *src != 0)
		{
			std::string s((char*)src, len);

			Util::Replace(s, "\r\n", "\n");

			s = Util::Trim(s);

			if(!s.empty())
			{
				ss->AppendText(start, stop, Util::ConvertMBCS(s.c_str(), ANSI_CHARSET).c_str());

				invalidate = true;
			}
		}
	}
	else if(m_mt.majortype == MEDIATYPE_Subtitle)
	{
		if(m_mt.subtype == MEDIASUBTYPE_UTF8)
		{
			std::wstring s = Util::Trim(Util::UTF8To16(std::string((LPCSTR)src, len).c_str()));
			
			if(!s.empty())
			{
				CComQIPtr<ITextSubStream>(m_ss)->AppendText(start, stop, s.c_str());

				invalidate = true;
			}
		}
		else if(m_mt.subtype == MEDIASUBTYPE_SSA || m_mt.subtype == MEDIASUBTYPE_ASS || m_mt.subtype == MEDIASUBTYPE_ASS2)
		{
			std::wstring s = Util::Trim(Util::UTF8To16(std::string((LPCSTR)src, len).c_str()));
			
			if(!s.empty())
			{
				if(m_mt.subtype != MEDIASUBTYPE_ASS2)
				{
					std::list<std::wstring> sl;

					Util::Explode(s, sl, L",", 9);

					if(sl.size() == 9)
					{
						auto i = sl.begin();
						for(int j = 0; j < 6; i++, j++);
						sl.insert(i, *i);
					}

					s = Util::Implode(sl, L",");
				}

				CComQIPtr<ITextSubStream>(m_ss)->AppendSSA(start, stop, s.c_str());

				invalidate = true;
			}
		}
		else if(m_mt.subtype == MEDIASUBTYPE_SSF)
		{
			std::wstring s = Util::Trim(Util::UTF8To16(std::string((LPCSTR)src, len).c_str()));
			
			if(!s.empty())
			{
				CComQIPtr<ITextSubStream>(m_ss)->AppendSSF(start, stop, s.c_str());

				invalidate = true;
			}
		}
		else if(m_mt.subtype == MEDIASUBTYPE_VOBSUB)
		{
			CComQIPtr<IVobSubStream>(m_ss)->Append(start, stop, src, len);
		}
	}

	if(invalidate)
	{
		InvalidateSubtitle(start, m_ss);
	}

	hr = S_OK;

    return hr;
}

