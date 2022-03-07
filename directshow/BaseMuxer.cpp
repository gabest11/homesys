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
#include "BaseMuxer.h"
#include "DirectShow.h"
#include <mmreg.h>
#include <aviriff.h>
#include <initguid.h>
#include "moreuuids.h"

// CBaseMuxerFilter

CBaseMuxerFilter::CBaseMuxerFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid)
	: CBaseFilter(NAME("CBaseMuxerFilter"), pUnk, this, clsid)
	, m_current(0)
{
	if(phr) *phr = S_OK;

	m_pOutput = new CBaseMuxerOutputPin(L"Output", this, phr);

	AddInput();
}

CBaseMuxerFilter::~CBaseMuxerFilter()
{
	for(auto i = m_pInputs.begin(); i != m_pInputs.end(); i++)
	{
		delete *i;
	}

	for(auto i = m_pRawOutputs.begin(); i != m_pRawOutputs.end(); i++)
	{
		delete *i;
	}

	delete m_pOutput;
}

STDMETHODIMP CBaseMuxerFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return 
		QI(IMediaSeeking)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		QI(IDSMResourceBag)
		QI(IDSMChapterBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CBaseMuxerFilter::AddInput()
{
	for(auto i = m_pInputs.begin(); i != m_pInputs.end(); i++)
	{
		if(!(*i)->IsConnected()) 
		{
			return;
		}
	}

	std::wstring name = Util::Format(L"Input %d", m_pInputs.size() + 1);

	CBaseMuxerInputPin* pInputPin = NULL;

	if(FAILED(CreateInput(name.c_str(), &pInputPin)))
	{
		return;
	}

	name = Util::Format(L"~Output %d", m_pRawOutputs.size() + 1);

	CBaseMuxerRawOutputPin* pRawOutputPin = NULL;

	if(FAILED(CreateRawOutput(name.c_str(), &pRawOutputPin)))
	{
		delete pInputPin;

		return;
	}

	pInputPin->SetRelatedPin(pRawOutputPin);
	pRawOutputPin->SetRelatedPin(pInputPin);

	m_pInputs.push_back(pInputPin);
	m_pRawOutputs.push_back(pRawOutputPin);
}

HRESULT CBaseMuxerFilter::CreateInput(LPCWSTR name, CBaseMuxerInputPin** ppPin)
{
	CheckPointer(ppPin, E_POINTER);

	HRESULT hr = S_OK;
	
	*ppPin = new CBaseMuxerInputPin(name, this, &hr);

	return hr;
}

HRESULT CBaseMuxerFilter::CreateRawOutput(LPCWSTR name, CBaseMuxerRawOutputPin** ppPin)
{
	CheckPointer(ppPin, E_POINTER);

	HRESULT hr = S_OK;

	*ppPin = new CBaseMuxerRawOutputPin(name, this, &hr);

	return hr;
}

DWORD CBaseMuxerFilter::ThreadProc()
{
	SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

	while(1)
	{
		DWORD cmd = GetRequest();

		switch(cmd)
		{
		default:
		case CMD_EXIT:
			CAMThread::m_hThread = NULL;
			Reply(S_OK);
			return 0;

		case CMD_RUN:
			m_pActivePins.clear();
			m_pins.clear();

			for(auto i = m_pInputs.begin(); i != m_pInputs.end(); i++)
			{
				CBaseMuxerInputPin* pin = *i;

				if(pin->IsConnected())
				{
					m_pActivePins.push_back(pin);
					m_pins.push_back(pin);
				}
			}

			m_current = 0;

			Reply(S_OK);

			MuxInit();

			CMuxerPacket* p = NULL;

			try
			{
				MuxHeaderInternal();

				while(!CheckRequest(NULL) && !m_pActivePins.empty())
				{
					if(m_State == State_Paused) {Sleep(10); continue;}

					CMuxerPacket* p = GetPacket();

					if(p == NULL) {Sleep(1); continue;}

					if(p->IsTimeValid())
					{
						m_current = p->start;
					}

					if(p->IsEOS())
					{
						m_pActivePins.remove(p->pin);
					}
					
					MuxPacketInternal(p);

					delete p;

					p = NULL;
				}

				MuxFooterInternal();
			}
			catch(HRESULT hr)
			{
				CComQIPtr<IMediaEventSink>(m_pGraph)->Notify(EC_ERRORABORT, hr, 0);
			}

			delete p;

			m_pOutput->DeliverEndOfStream();

			for(auto i = m_pRawOutputs.begin(); i != m_pRawOutputs.end(); i++)
			{
				(*i)->DeliverEndOfStream();
			}

			m_pActivePins.clear();
			m_pins.clear();

			break;
		}
	}

	ASSERT(0); // this function should only return via CMD_EXIT

	CAMThread::m_hThread = NULL;
	return 0;
}

void CBaseMuxerFilter::MuxHeaderInternal()
{
	if(CComQIPtr<IBitStream> stream = m_pOutput->GetBitStream())
	{
		MuxHeader(stream);
	}

	MuxHeader();

	for(auto i = m_pins.begin(); i != m_pins.end(); i++)
	{
		CBaseMuxerInputPin* pin = *i;

		if(pin != NULL)
		{
			if(CBaseMuxerRawOutputPin* pOutput = dynamic_cast<CBaseMuxerRawOutputPin*>(pin->GetRelatedPin()))
			{
				pOutput->MuxHeader(pin->CurrentMediaType());
			}
		}
	}
}

void CBaseMuxerFilter::MuxPacketInternal(const CMuxerPacket* p)
{
	if(CComQIPtr<IBitStream> stream = m_pOutput->GetBitStream())
	{
		MuxPacket(stream, p);
	}

	MuxPacket(p);

	if(p->pin != NULL)
	{
		if(CBaseMuxerRawOutputPin* pOutput = dynamic_cast<CBaseMuxerRawOutputPin*>(p->pin->GetRelatedPin()))
		{
			pOutput->MuxPacket(p->pin->CurrentMediaType(), p);
		}
	}
}

void CBaseMuxerFilter::MuxFooterInternal()
{
	if(CComQIPtr<IBitStream> stream = m_pOutput->GetBitStream())
	{
		MuxFooter(stream);
	}

	MuxFooter();

	for(auto i = m_pins.begin(); i != m_pins.end(); i++)
	{
		CBaseMuxerInputPin* pin = *i;

		if(pin != NULL)
		{
			if(CBaseMuxerRawOutputPin* pOutput = dynamic_cast<CBaseMuxerRawOutputPin*>(pin->GetRelatedPin()))
			{
				pOutput->MuxFooter(pin->CurrentMediaType());
			}
		}
	}
}

CMuxerPacket* CBaseMuxerFilter::GetPacket()
{
	REFERENCE_TIME rtMin = _I64_MAX;

	CBaseMuxerInputPin* pPinMin = NULL;

	int n = m_pActivePins.size();

	for(auto i = m_pActivePins.begin(); i != m_pActivePins.end(); i++)
	{
		CBaseMuxerInputPin* pin = *i;

		if(pin->m_queue.GetCount() > 0)
		{
			CAutoLock cAutoLock(&pin->m_queue);

			CMuxerPacket* p = pin->m_queue.Peek();

			if(p->IsBogus() || !p->IsTimeValid() || p->IsEOS())
			{
				pPinMin = pin;			
				n = 0;
				break;
			}

			if(p->start < rtMin)
			{
				rtMin = p->start;
				pPinMin = pin;
			}

			n--;
		}
	}

	CMuxerPacket* p = NULL;

	if(pPinMin != NULL && n == 0)
	{
		p = pPinMin->m_queue.Dequeue();
	}

	return p;
}

int CBaseMuxerFilter::GetPinCount()
{
	return m_pInputs.size() + (m_pOutput != NULL ? 1 : 0) + m_pRawOutputs.size();
}

CBasePin* CBaseMuxerFilter::GetPin(int n)
{
    CAutoLock cAutoLock(this);

	if(n >= 0 && n < m_pInputs.size())
	{
		return m_pInputs[n];
	}

	n -= m_pInputs.size();

	if(n == 0 && m_pOutput != NULL)
	{
		return m_pOutput;
	}

	n--;

	if(n >= 0 && n < m_pRawOutputs.size())
	{
		return m_pRawOutputs[n];
	}

	return NULL;
}

STDMETHODIMP CBaseMuxerFilter::Stop()
{
	CAutoLock cAutoLock(this);

	HRESULT hr;
	
	hr = __super::Stop();

	if(FAILED(hr)) 
	{
		return hr;
	}

	CallWorker(CMD_EXIT);

	return hr;
}

STDMETHODIMP CBaseMuxerFilter::Pause()
{
	CAutoLock cAutoLock(this);

	FILTER_STATE fs = m_State;

	HRESULT hr;
	
	hr = __super::Pause();
	
	if(FAILED(hr))
	{
		return hr;
	}

	if(fs == State_Stopped && m_pOutput != NULL)
	{
		CAMThread::Create();

		CallWorker(CMD_RUN);
	}

	return hr;
}

// IMediaSeeking

STDMETHODIMP CBaseMuxerFilter::GetCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);

	*pCapabilities = AM_SEEKING_CanGetDuration | AM_SEEKING_CanGetCurrentPos;

	return S_OK;
}

STDMETHODIMP CBaseMuxerFilter::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);

	if(*pCapabilities == 0) return S_OK;

	DWORD caps;

	GetCapabilities(&caps);

	caps &= *pCapabilities;

	if(caps == 0)
	{
		return E_FAIL;
	}

	return caps == *pCapabilities ? S_OK : S_FALSE;
}

STDMETHODIMP CBaseMuxerFilter::IsFormatSupported(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

STDMETHODIMP CBaseMuxerFilter::QueryPreferredFormat(GUID* pFormat) 
{
	return GetTimeFormat(pFormat);
}

STDMETHODIMP CBaseMuxerFilter::GetTimeFormat(GUID* pFormat) 
{
	return pFormat ? *pFormat = TIME_FORMAT_MEDIA_TIME, S_OK : E_POINTER;
}

STDMETHODIMP CBaseMuxerFilter::IsUsingTimeFormat(const GUID* pFormat) 
{
	return IsFormatSupported(pFormat);
}

STDMETHODIMP CBaseMuxerFilter::SetTimeFormat(const GUID* pFormat)
{
	return S_OK == IsFormatSupported(pFormat) ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CBaseMuxerFilter::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);

	REFERENCE_TIME dur = 0;

	for(auto i = m_pInputs.begin(); i != m_pInputs.end(); i++)
	{
		REFERENCE_TIME rt = (*i)->GetDuration(); 
		
		if(rt > dur)
		{
			dur = rt;
		}
	}

	*pDuration = dur;

	return S_OK;
}

STDMETHODIMP CBaseMuxerFilter::GetStopPosition(LONGLONG* pStop)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::GetCurrentPosition(LONGLONG* pCurrent)
{
	CheckPointer(pCurrent, E_POINTER);

	*pCurrent = m_current;

	return S_OK;
}

STDMETHODIMP CBaseMuxerFilter::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	FILTER_STATE fs;

	if(SUCCEEDED(GetState(0, &fs)) && fs == State_Stopped)
	{
		for(auto i = m_pInputs.begin(); i != m_pInputs.end(); i++)
		{
			CBasePin* pin = *i;

			CComQIPtr<IMediaSeeking> pMS = pin->GetConnected();
			
			if(pMS == NULL)
			{
				pMS = DirectShow::GetFilter(pin->GetConnected());
			}

			if(pMS != NULL) 
			{
				pMS->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
			}
		}

		return S_OK;
	}

	return VFW_E_WRONG_STATE;
}

STDMETHODIMP CBaseMuxerFilter::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::SetRate(double dRate) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::GetRate(double* pdRate) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CBaseMuxerFilter::GetPreroll(LONGLONG* pllPreroll) 
{
	return E_NOTIMPL;
}

// CBaseMuxerInputPin

CBaseMuxerInputPin::CBaseMuxerInputPin(LPCWSTR pName, CBaseMuxerFilter* pFilter, HRESULT* phr)
	: CBaseInputPin(NAME("CBaseMuxerInputPin"), pFilter, pFilter, phr, pName)
	, m_queue(1000)
	, m_current(0)
	, m_duration(0)
	, m_packet(0)
{
	static int s_id = 0;

	m_id = s_id++; // not thread safe, but creating pins for the same filter on multiple threads at the same time is very unlikely
}

CBaseMuxerInputPin::~CBaseMuxerInputPin()
{
}

STDMETHODIMP CBaseMuxerInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IBaseMuxerRelatedPin)
		QI(IPropertyBag)
		QI(IPropertyBag2)
		QI(IDSMPropertyBag)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

bool CBaseMuxerInputPin::IsSubtitleStream()
{
	return m_mt.majortype == MEDIATYPE_Subtitle || m_mt.majortype == MEDIATYPE_Text;
}

HRESULT CBaseMuxerInputPin::CheckMediaType(const CMediaType* pmt)
{
	if(pmt->formattype == FORMAT_WaveFormatEx)
	{
		WORD wFormatTag = ((WAVEFORMATEX*)pmt->pbFormat)->wFormatTag;

		if((wFormatTag == WAVE_FORMAT_PCM || wFormatTag == WAVE_FORMAT_EXTENSIBLE || wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
		&& pmt->subtype != FOURCCMap(wFormatTag)
		&& !(pmt->subtype == MEDIASUBTYPE_PCM && wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		&& !(pmt->subtype == MEDIASUBTYPE_PCM && wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
		&& pmt->subtype != MEDIASUBTYPE_DVD_LPCM_AUDIO
		&& pmt->subtype != MEDIASUBTYPE_DOLBY_AC3
		&& pmt->subtype != MEDIASUBTYPE_DTS)
		{
			return E_INVALIDARG;
		}
	}

	return pmt->majortype == MEDIATYPE_Video
		|| pmt->majortype == MEDIATYPE_Audio && pmt->formattype != FORMAT_VorbisFormat
		|| pmt->majortype == MEDIATYPE_Text && pmt->subtype == MEDIASUBTYPE_NULL && pmt->formattype == FORMAT_None
		|| pmt->majortype == MEDIATYPE_Subtitle
		? S_OK
		: E_INVALIDARG;
}

HRESULT CBaseMuxerInputPin::BreakConnect()
{
	HRESULT hr;

	hr = __super::BreakConnect();

	if(FAILED(hr)) 
	{
		return E_FAIL;
	}

	RemoveAll();

	// TODO: remove extra disconnected pins, leave one

	return hr;
}

HRESULT CBaseMuxerInputPin::CompleteConnect(IPin* pReceivePin)
{
	HRESULT hr;
	
	hr = __super::CompleteConnect(pReceivePin);

	if(FAILED(hr)) 
	{
		return hr;
	}

	// duration

	m_duration = 0;

	CComQIPtr<IMediaSeeking> pMS;

	if((pMS = DirectShow::GetFilter(pReceivePin)) || (pMS = pReceivePin))
	{
		pMS->GetDuration(&m_duration);
	}

	// properties

	for(CComPtr<IPin> pPin = pReceivePin; pPin; pPin = DirectShow::GetUpStreamPin(DirectShow::GetFilter(pPin)))
	{
		if(CComQIPtr<IDSMPropertyBag> pPB = pPin)
		{
			ULONG cProperties = 0;

			if(SUCCEEDED(pPB->CountProperties(&cProperties)) && cProperties > 0)
			{
				for(ULONG iProperty = 0; iProperty < cProperties; iProperty++)
				{
					PROPBAG2 pb;
					
					memset(&pb, 0, sizeof(pb));

					ULONG cPropertiesReturned = 0;

					if(FAILED(pPB->GetPropertyInfo(iProperty, 1, &pb, &cPropertiesReturned)))
					{
						continue;
					}

					HRESULT hr;

					CComVariant var;

					if(SUCCEEDED(pPB->Read(1, &pb, NULL, &var, &hr)) && SUCCEEDED(hr))
					{
						SetProperty(pb.pstrName, &var);
					}

					CoTaskMemFree(pb.pstrName);
				}
			}
		}
	}

	((CBaseMuxerFilter*)m_pFilter)->AddInput();

	return S_OK;
}

HRESULT CBaseMuxerInputPin::Active()
{
	m_current = _I64_MIN;
	m_eos = false;
	m_packet = 0;

	return __super::Active();
}

HRESULT CBaseMuxerInputPin::Inactive()
{
	while(m_queue.GetCount() > 0)
	{
		delete m_queue.Dequeue();
	}

	return __super::Inactive();
}

STDMETHODIMP CBaseMuxerInputPin::Receive(IMediaSample* pSample)
{
	CAutoLock cAutoLock(&m_csReceive);

	if(IsFlushing()) return S_OK;

	HRESULT hr;
	
	hr = __super::Receive(pSample);

	if(FAILED(hr)) 
	{
		return hr;
	}

	CMuxerPacket* p = new CMuxerPacket(this);

	long len = pSample->GetActualDataLength();

	BYTE* src = NULL;

	if(FAILED(pSample->GetPointer(&src)) || src == NULL)
	{
		return S_OK;
	}

	p->buff.resize(len);

	memcpy(p->buff.data(), src, len);

	if(S_OK == pSample->IsSyncPoint() || m_mt.majortype == MEDIATYPE_Audio && !m_mt.bTemporalCompression)
	{
		p->flags |= CMuxerPacket::SyncPoint;
	}

	if(S_OK == pSample->GetTime(&p->start, &p->stop))
	{
		p->flags |= CMuxerPacket::TimeValid;

		p->start += m_tStart; 
		p->stop += m_tStart;

		if((p->flags & CMuxerPacket::SyncPoint) && p->start < m_current)
		{
			p->flags &= ~CMuxerPacket::SyncPoint;
			p->flags |= CMuxerPacket::Bogus;
		}

		m_current = std::max<__int64>(m_current, p->start);
	}
	else if(p->flags & CMuxerPacket::SyncPoint)
	{
		p->flags &= ~CMuxerPacket::SyncPoint;
		p->flags |= CMuxerPacket::Bogus;
	}

	if(S_OK == pSample->IsDiscontinuity())
	{
		p->flags |= CMuxerPacket::Discontinuity;
	}

	p->packet = m_packet++;

	m_queue.Enqueue(p);

	return S_OK;
}

STDMETHODIMP CBaseMuxerInputPin::EndOfStream()
{
	CAutoLock cAutoLock(&m_csReceive);

	if(IsFlushing()) return S_OK;

	HRESULT hr;
	
	hr = __super::EndOfStream();

	if(FAILED(hr)) 
	{
		return hr;
	}

	ASSERT(!m_eos);

	CMuxerPacket* p = new CMuxerPacket(this);

	p->flags |= CMuxerPacket::EoS;

	m_queue.Enqueue(p);

	m_eos = true;

	return hr;
}

// CBaseMuxerOutputPin

CBaseMuxerOutputPin::CBaseMuxerOutputPin(LPCWSTR pName, CBaseMuxerFilter* pFilter, HRESULT* phr)
	: CBaseOutputPin(_T("CBaseMuxerOutputPin"), pFilter, pFilter, phr, pName)
{
}

IBitStream* CBaseMuxerOutputPin::GetBitStream()
{
	if(m_stream == NULL)
	{
		if(CComQIPtr<IStream> pStream = GetConnected())
		{
			m_stream = new CBitStream(pStream, true);
		}
	}

	return m_stream;
}

HRESULT CBaseMuxerOutputPin::BreakConnect()
{
	m_stream = NULL;

	return __super::BreakConnect();
}

HRESULT CBaseMuxerOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1;

    return S_OK;
}

HRESULT CBaseMuxerOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Stream && pmt->subtype == MEDIASUBTYPE_NULL ? S_OK : E_INVALIDARG;
}

HRESULT CBaseMuxerOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cAutoLock(m_pLock);

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->InitMediaType();

	pmt->majortype = MEDIATYPE_Stream;
	pmt->formattype = FORMAT_None;

	return S_OK;
}

HRESULT CBaseMuxerOutputPin::DeliverEndOfStream()
{
	m_stream = NULL;

	return __super::DeliverEndOfStream();
}

STDMETHODIMP CBaseMuxerOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	return E_NOTIMPL;
}

// CBaseMuxerRawOutputPin

CBaseMuxerRawOutputPin::CBaseMuxerRawOutputPin(LPCWSTR pName, CBaseMuxerFilter* pFilter, HRESULT* phr)
	: CBaseMuxerOutputPin(pName, pFilter,phr)
{
}

STDMETHODIMP CBaseMuxerRawOutputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IBaseMuxerRelatedPin)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CBaseMuxerRawOutputPin::MuxHeader(const CMediaType& mt)
{
	CComQIPtr<IBitStream> stream = GetBitStream();

	if(!stream) return;

	const BYTE utf8bom[3] = {0xef, 0xbb, 0xbf};
	
	if((mt.subtype == MEDIASUBTYPE_AVC1 || mt.subtype == MEDIASUBTYPE_avc1) && mt.formattype == FORMAT_MPEG2_VIDEO)
	{
		MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.Format();

		BYTE* p = (BYTE*)vih->dwSequenceHeader;

		for(DWORD i = 0; i < vih->cbSequenceHeader - 2; i += 2)
		{
			DWORD size = (p[i + 0] << 8) | p[i + 1];

			stream->BitWrite(0x00000001, 32);
			stream->ByteWrite(&p[i + 2], size);

			i += size;
		}
	}
	else if(mt.subtype == MEDIASUBTYPE_UTF8)
	{
		stream->ByteWrite(utf8bom, sizeof(utf8bom));
	}
	else if(mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2)
	{
		SUBTITLEINFO* si = (SUBTITLEINFO*)mt.Format();

		BYTE* p = (BYTE*)si + si->dwOffset;

		if(memcmp(utf8bom, p, 3) != 0) 
		{
			stream->ByteWrite(utf8bom, sizeof(utf8bom));
		}

		std::string str((char*)p, mt.FormatLength() - (p - mt.Format()));

		stream->StrWrite(str.c_str() + '\n', true);

		if(str.find("[Events]") == std::string::npos) 
		{
			stream->StrWrite("\n\n[Events]\n", true);
		}
	}
	/* FIXME
	else if(mt.subtype == MEDIASUBTYPE_SSF)
	{
		DWORD dwOffset = ((SUBTITLEINFO*)mt.pbFormat)->dwOffset;
		try {m_ssf.Parse(ssf::MemoryInputStream(mt.pbFormat + dwOffset, mt.cbFormat - dwOffset, false, false));}
		catch(ssf::Exception&) {}
	}
	*/
	else if(mt.subtype == MEDIASUBTYPE_VOBSUB)
	{
		m_index.clear();
	}
	else if(mt.majortype == MEDIATYPE_Audio 
		&& (mt.subtype == MEDIASUBTYPE_PCM 
		|| mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
		|| mt.subtype == FOURCCMap(WAVE_FORMAT_EXTENSIBLE) 
		|| mt.subtype == FOURCCMap(WAVE_FORMAT_IEEE_FLOAT))
		&& mt.formattype == FORMAT_WaveFormatEx)
	{
		stream->BitWrite('RIFF', 32);
		stream->BitWrite(0, 32); // file length - 8, set later
		stream->BitWrite('WAVE', 32);

		stream->BitWrite('fmt ', 32);
		stream->ByteWrite(&mt.cbFormat, 4);
		stream->ByteWrite(mt.pbFormat, mt.cbFormat);

		stream->BitWrite('data', 32);
		stream->BitWrite(0, 32); // data length, set later
	}
}

void CBaseMuxerRawOutputPin::MuxPacket(const CMediaType& mt, const CMuxerPacket* p)
{
	CComQIPtr<IBitStream> stream = GetBitStream();

	if(stream == NULL) return;

	const BYTE* buff = p->buff.data();
	const int size = p->buff.size();

	if(mt.subtype == MEDIASUBTYPE_AAC && mt.formattype == FORMAT_WaveFormatEx)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

		int profile;
		int srate_idx;
		int channels;

		if(wfe->cbSize >= 2)
		{
			BYTE* p = (BYTE*)(wfe + 1);

			profile = (p[0] >> 3) - 1;
			srate_idx = ((p[0] & 7) << 1) | ((p[1] & 0x80) >> 7);
			channels = (p[1] >> 3) & 15;
		}
		else
		{
			profile = 0;

			if(92017 <= wfe->nSamplesPerSec) srate_idx = 0;
			else if(75132 <= wfe->nSamplesPerSec) srate_idx = 1;
			else if(55426 <= wfe->nSamplesPerSec) srate_idx = 2;
			else if(46009 <= wfe->nSamplesPerSec) srate_idx = 3;
			else if(37566 <= wfe->nSamplesPerSec) srate_idx = 4;
			else if(27713 <= wfe->nSamplesPerSec) srate_idx = 5;
			else if(23004 <= wfe->nSamplesPerSec) srate_idx = 6;
			else if(18783 <= wfe->nSamplesPerSec) srate_idx = 7;
			else if(13856 <= wfe->nSamplesPerSec) srate_idx = 8;
			else if(11502 <= wfe->nSamplesPerSec) srate_idx = 9;
			else if(9391 <= wfe->nSamplesPerSec) srate_idx = 10;
			else srate_idx = 11;

			channels = wfe->nChannels;
		}

		stream->BitWrite(0xfff, 12); // sync
		stream->BitWrite(9, 4); // version, layer, crc
		stream->BitWrite(profile, 2);
		stream->BitWrite(srate_idx, 4);
		stream->BitWrite(0, 3); // private
		stream->BitWrite(channels, 3);
		stream->BitWrite(0, 4); // original, home, copyright
		stream->BitWrite(size + 7, 13);
		stream->BitWrite(0x7ff, 11); // adts_buffer_fullness
		stream->BitWrite(0, 2); // no_raw_data_blocks_in_frame

		stream->BitFlush();
	}
	else if((mt.subtype == MEDIASUBTYPE_AVC1 || mt.subtype == MEDIASUBTYPE_avc1) && mt.formattype == FORMAT_MPEG2_VIDEO)
	{
		const BYTE* p = buff;
		int i = size;

		while(i >= 4)
		{
			DWORD len = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];

			i -= len + 4;
			p += len + 4;
		}

		if(i == 0)
		{
			p = buff;
			i = size;

			while(i >= 4)
			{
				DWORD len = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];

				stream->BitWrite(0x00000001, 32);

				p += 4; 
				i -= 4;

				if(len > i || len == 1) {len = i; ASSERT(0);}

				stream->ByteWrite(p, len);

				p += len;
				i -= len;
			}

			return;
		}
	}
	else if(mt.subtype == MEDIASUBTYPE_UTF8 || mt.majortype == MEDIATYPE_Text)
	{
		std::string s = Util::Trim(std::string((char*)buff, size));

		if(!s.empty())
		{
			DVD_HMSF_TIMECODE start = DirectShow::RT2HMSF(p->start, 25);
			DVD_HMSF_TIMECODE stop = DirectShow::RT2HMSF(p->stop, 25);

			s = Util::Format("%d\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\n%s\n\n", 
				p->packet + 1,
				start.bHours, start.bMinutes, start.bSeconds, (int)((p->start / 10000) % 1000), 
				stop.bHours, stop.bMinutes, stop.bSeconds, (int)((p->stop / 10000) % 1000),
				s.c_str());

			stream->StrWrite(s.c_str(), true);
		}

		return;
	}
	else if(mt.subtype == MEDIASUBTYPE_SSA || mt.subtype == MEDIASUBTYPE_ASS || mt.subtype == MEDIASUBTYPE_ASS2)
	{
		std::string s = Util::Trim(std::string((char*)buff, size));

		if(!s.empty())
		{
			DVD_HMSF_TIMECODE start = DirectShow::RT2HMSF(p->start, 25);
			DVD_HMSF_TIMECODE stop = DirectShow::RT2HMSF(p->stop, 25);

			int fields = mt.subtype == MEDIASUBTYPE_ASS2 ? 10 : 9;

			std::list<std::string> tokens;

			Util::Explode(s, tokens, ',', fields);

			if(tokens.size() < fields) return;

			auto i = tokens.begin();

			std::string readorder = *i++; // TODO
			std::string layer = *i++;
			std::string style = *i++;
			std::string actor = *i++;
			std::string left = *i++;
			std::string right = *i++;
			std::string top = *i++;
			if(fields == 10) top += ',' + *i++; // bottom
			std::string effect = *i++;
			s = *i++;

			if(mt.subtype == MEDIASUBTYPE_SSA) 
			{
				layer = "Marked=0";
			}

			s = Util::Format("Dialogue: %s,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,%s,%s,%s,%s,%s,%s,%s\n",
				layer.c_str(),
				start.bHours, start.bMinutes, start.bSeconds, (int)((p->start / 100000) % 100), 
				stop.bHours, stop.bMinutes, stop.bSeconds, (int)((p->stop / 100000) % 100),
				style.c_str(), 
				actor.c_str(), 
				left.c_str(), 
				right.c_str(), 
				top.c_str(), 
				effect.c_str(), 
				s.c_str());

			stream->StrWrite(s.c_str(), true);
		}

		return;
	}
	/* FIXME
	else if(mt.subtype == MEDIASUBTYPE_SSF)
	{
		float start = (float)pPacket->rtStart / 10000000;
		float stop = (float)pPacket->rtStop / 10000000;
		m_ssf.Append(ssf::WCharInputStream(UTF8To16(CStringA((char*)pData, DataSize))), start, stop, true);
		return;
	}
	*/
	else if(mt.subtype == MEDIASUBTYPE_VOBSUB)
	{
		bool hastime = p->IsTimeValid();

		if(hastime)
		{
			IndexEntry i;
			i.rt = p->start;
			i.fp = stream->GetPos();
			m_index.push_back(i);
		}

		int remaining = size;

		while(remaining > 0)
		{
			int avail = 0x7ec - (hastime ? 9 : 4);
			int len = std::min<int>(avail, remaining);
			int padding = 0x800 - len - 20 - (hastime ? 9 : 4);

			stream->BitWrite(0x000001ba, 32);
			stream->BitWrite(0x440004000401ui64, 48);
			stream->BitWrite(0x000003f8, 32);
			stream->BitWrite(0x000001bd, 32);

			if(hastime)
			{
				stream->BitWrite(len + 9, 16);
				stream->BitWrite(0x8180052100010001ui64, 64);
			}
			else
			{
				stream->BitWrite(len + 4, 16);
				stream->BitWrite(0x810000, 24);
			}

			stream->BitWrite(0x20, 8);

			stream->ByteWrite(buff, len);

			buff += len;
			remaining -= len;

			if(padding > 0)
			{
				padding -= 6;

				ASSERT(padding >= 0);

				stream->BitWrite(0x000001be, 32);
				stream->BitWrite(padding, 16);

				while(padding-- > 0)
				{
					stream->BitWrite(0xff, 8);
				}
			}

			hastime = false;
		}

		return;
	}
	else if(mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO)
	{
		WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.Format();

		for(int i = 0, bps = wfe->wBitsPerSample / 8; i < size; i += bps)
		{
			for(int j = bps - 1; j >= 0; j--)
			{
				stream->BitWrite(buff[i + j], 8);
			}
		}

		return;
	}
	// else // TODO: restore more streams (vorbis to ogg)

	stream->ByteWrite(buff, size);
}

void CBaseMuxerRawOutputPin::MuxFooter(const CMediaType& mt)
{
	CComQIPtr<IBitStream> stream = GetBitStream();
	
	if(stream == NULL) return;

	if(mt.majortype == MEDIATYPE_Audio 
	&& (mt.subtype == MEDIASUBTYPE_PCM 
	|| mt.subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
	|| mt.subtype == FOURCCMap(WAVE_FORMAT_EXTENSIBLE) 
	|| mt.subtype == FOURCCMap(WAVE_FORMAT_IEEE_FLOAT))
	&& mt.formattype == FORMAT_WaveFormatEx)
	{
		stream->BitFlush();

		ASSERT(stream->GetPos() <= 0xffffffff);

		DWORD size = (DWORD )stream->GetPos();

		size -= 8;
		
		stream->Seek(4);
		stream->ByteWrite(&size, 4);

		size -= sizeof(RIFFLIST) + sizeof(RIFFCHUNK) + mt.FormatLength();

		stream->Seek(sizeof(RIFFLIST) + sizeof(RIFFCHUNK) + mt.FormatLength() + 4);
		stream->ByteWrite(&size, 4);
	}
	/* FIXME
	else if(mt.subtype == MEDIASUBTYPE_SSF)
	{
		ssf::WCharOutputStream s;
		try {m_ssf.Dump(s);} catch(ssf::Exception&) {}
		CStringA str = UTF16To8(s.GetString());
		pBitStream->StrWrite(str, true);
	}
	*/
	else if(mt.subtype == MEDIASUBTYPE_VOBSUB)
	{
		if(CComQIPtr<IFileSinkFilter> pFSF = DirectShow::GetFilter(GetConnected()))
		{
			wchar_t* fn = NULL;

			if(SUCCEEDED(pFSF->GetCurFile(&fn, NULL)))
			{
				wchar_t buff[MAX_PATH];
				wcscpy(buff, fn);
				CoTaskMemFree(fn);

				PathRenameExtension(buff, L".idx");

				if(FILE* f = _wfopen(buff, L"w"))
				{
					SUBTITLEINFO* si = (SUBTITLEINFO*)mt.Format();

					fwprintf(f, L"%s\n", L"# VobSub index file, v7 (do not modify this line!)");

					fwrite(mt.Format() + si->dwOffset, mt.FormatLength() - si->dwOffset, 1, f);

					std::string s = si->IsoLang;

					std::wstring lang = Util::ISO6392To6391(std::wstring(s.begin(), s.end()).c_str());

					if(lang.empty()) lang = _T("--");

					fwprintf(f, L"\nlangidx: 0\n\nid: %s, index: 0\n", lang.c_str());

					if(wcslen(si->TrackName) > 0)
					{
						fwprintf(f, L"alt: %s\n", si->TrackName);
					}

					for(auto i = m_index.begin(); i != m_index.end(); i++)
					{
						DVD_HMSF_TIMECODE start = DirectShow::RT2HMSF(i->rt, 25);

						fwprintf(f, L"timestamp: %02d:%02d:%02d:%03d, filepos: %09I64x\n", 
							start.bHours, start.bMinutes, start.bSeconds, (int)((i->rt / 10000) % 1000),
							i->fp);
					}

					fclose(f);
				}
			}
		}
	}
}

// CBaseMuxerRelatedPin

CBaseMuxerRelatedPin::CBaseMuxerRelatedPin()
{
}

// IBaseMuxerRelatedPin

STDMETHODIMP CBaseMuxerRelatedPin::SetRelatedPin(CBasePin* pPin)
{
	m_other = pPin;

	return S_OK;
}

STDMETHODIMP_(CBasePin*) CBaseMuxerRelatedPin::GetRelatedPin()
{
	return m_other;
}
