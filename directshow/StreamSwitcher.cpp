// Copyright 2003 Gabest.
// http://www.gabest.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html

#include "StdAfx.h"
#include "StreamSwitcher.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

[uuid("138130AF-A79B-45D5-B4AA-87697457BA87")]
class NeroAudioDecoder {};

[uuid("AEFA5024-215A-4FC7-97A4-1043C86FD0B8")]
class MatrixMixer {};

// CStreamSwitcherPassThru

CStreamSwitcherPassThru::CStreamSwitcherPassThru(LPUNKNOWN pUnk, HRESULT* phr, CStreamSwitcherFilter* pFilter)
	: CMediaPosition(NAME("CStreamSwitcherPassThru"), pUnk)
	, m_pFilter(pFilter)
{
}

STDMETHODIMP CStreamSwitcherPassThru::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

    return 
		QI(IMediaSeeking)
		CMediaPosition::NonDelegatingQueryInterface(riid, ppv);
}

template<class T> HRESULT GetPeer(CStreamSwitcherFilter* pFilter, T** ppT)
{
    *ppT = NULL;

	CBasePin* pPin = pFilter->GetInputPin();

	if(!pPin) return E_NOTIMPL;

    CComPtr<IPin> pConnected;

    if(FAILED(pPin->ConnectedTo(&pConnected))) 
	{
		return E_NOTIMPL;
	}

	if(CComQIPtr<T> pT = pConnected)
	{
		*ppT = pT.Detach();

		return S_OK;
	}

	return E_NOTIMPL;
}

// IMediaSeeking

STDMETHODIMP CStreamSwitcherPassThru::GetCapabilities(DWORD* pCaps)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->GetCapabilities(pCaps);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::CheckCapabilities(DWORD* pCaps)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->CheckCapabilities(pCaps);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::IsFormatSupported(const GUID* pFormat)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->IsFormatSupported(pFormat);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::QueryPreferredFormat(GUID* pFormat)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->QueryPreferredFormat(pFormat);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::SetTimeFormat(const GUID* pFormat)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->SetTimeFormat(pFormat);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::GetTimeFormat(GUID* pFormat)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->GetTimeFormat(pFormat);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::IsUsingTimeFormat(const GUID* pFormat)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->IsUsingTimeFormat(pFormat);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::SetPositions(LONGLONG* pCurrent, DWORD CurrentFlags, LONGLONG* pStop, DWORD StopFlags)
{
	HRESULT hr = E_NOTIMPL;

	for(auto i = m_pFilter->m_pInputs.begin(); i != m_pFilter->m_pInputs.end(); i++)
	{
		CComPtr<IPin> pin;

		if(SUCCEEDED((*i)->ConnectedTo(&pin)))
		{
			if(CComQIPtr<IMediaSeeking> pMS = pin)
			{
				HRESULT hr2 = pMS->SetPositions(pCurrent, CurrentFlags, pStop, StopFlags);
				
				if(*i == m_pFilter->GetInputPin())
				{
					hr = hr2;
				}
			}
		}
	}

	return hr;
}

STDMETHODIMP CStreamSwitcherPassThru::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->GetPositions(pCurrent, pStop);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::GetCurrentPosition(LONGLONG* pCurrent)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->GetCurrentPosition(pCurrent);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::GetStopPosition(LONGLONG* pStop)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->GetStopPosition(pStop);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::GetDuration(LONGLONG* pDuration)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->GetDuration(pDuration);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::GetPreroll(LONGLONG* pllPreroll)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->GetPreroll(pllPreroll);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->GetAvailable(pEarliest, pLatest);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::GetRate(double* pdRate)
{
	CComPtr<IMediaSeeking> pMS;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMS))) 
	{
		return pMS->GetRate(pdRate);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::SetRate(double dRate)
{
	if(0.0 == dRate) 
	{
		return E_INVALIDARG;
	}

	HRESULT hr = E_NOTIMPL;

	for(auto i = m_pFilter->m_pInputs.begin(); i != m_pFilter->m_pInputs.end(); i++)
	{
		CComPtr<IPin> pin;

		if(SUCCEEDED((*i)->ConnectedTo(&pin)))
		{
			if(CComQIPtr<IMediaSeeking> pMS = pin)
			{
				HRESULT hr2 = pMS->SetRate(dRate);
				
				if(*i == m_pFilter->GetInputPin())
				{
					hr = hr2;
				}
			}
		}
	}

	return hr;
}

// IMediaPosition

STDMETHODIMP CStreamSwitcherPassThru::get_Duration(REFTIME* plength)
{
	CComPtr<IMediaPosition> pMP;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMP))) 
	{
		return pMP->get_Duration(plength);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::get_CurrentPosition(REFTIME* pllTime)
{
	CComPtr<IMediaPosition> pMP;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMP))) 
	{
		return pMP->get_CurrentPosition(pllTime);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::put_CurrentPosition(REFTIME llTime)
{
	HRESULT hr = E_NOTIMPL;

	for(auto i = m_pFilter->m_pInputs.begin(); i != m_pFilter->m_pInputs.end(); i++)
	{
		CComPtr<IPin> pin;

		if(SUCCEEDED((*i)->ConnectedTo(&pin)))
		{
			if(CComQIPtr<IMediaPosition> pMP = pin)
			{
				HRESULT hr2 = pMP->put_CurrentPosition(llTime);
				
				if(*i == m_pFilter->GetInputPin())
				{
					hr = hr2;
				}
			}
		}
	}

	return hr;
}

STDMETHODIMP CStreamSwitcherPassThru::get_StopTime(REFTIME* pllTime)
{
	CComPtr<IMediaPosition> pMP;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMP))) 
	{
		return pMP->get_StopTime(pllTime);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::put_StopTime(REFTIME llTime)
{
	HRESULT hr = E_NOTIMPL;

	for(auto i = m_pFilter->m_pInputs.begin(); i != m_pFilter->m_pInputs.end(); i++)
	{
		CComPtr<IPin> pin;

		if(SUCCEEDED((*i)->ConnectedTo(&pin)))
		{
			if(CComQIPtr<IMediaPosition> pMP = pin)
			{
				HRESULT hr2 = pMP->put_StopTime(llTime);
				
				if(*i == m_pFilter->GetInputPin())
				{
					hr = hr2;
				}
			}
		}
	}

	return hr;
}

STDMETHODIMP CStreamSwitcherPassThru::get_PrerollTime(REFTIME * pllTime)
{
	CComPtr<IMediaPosition> pMP;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMP))) 
	{
		return pMP->get_PrerollTime(pllTime);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::put_PrerollTime(REFTIME llTime)
{
	HRESULT hr = E_NOTIMPL;

	for(auto i = m_pFilter->m_pInputs.begin(); i != m_pFilter->m_pInputs.end(); i++)
	{
		CComPtr<IPin> pin;

		if(SUCCEEDED((*i)->ConnectedTo(&pin)))
		{
			if(CComQIPtr<IMediaPosition> pMP = pin)
			{
				HRESULT hr2 = pMP->put_PrerollTime(llTime);
				
				if(*i == m_pFilter->GetInputPin())
				{
					hr = hr2;
				}
			}
		}
	}

	return hr;
}

STDMETHODIMP CStreamSwitcherPassThru::get_Rate(double* pdRate)
{
	CComPtr<IMediaPosition> pMP;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMP))) 
	{
		return pMP->get_Rate(pdRate);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::put_Rate(double dRate)
{
	if(0.0 == dRate) 
	{
		return E_INVALIDARG;
	}

	HRESULT hr = E_NOTIMPL;

	for(auto i = m_pFilter->m_pInputs.begin(); i != m_pFilter->m_pInputs.end(); i++)
	{
		CComPtr<IPin> pin;

		if(SUCCEEDED((*i)->ConnectedTo(&pin)))
		{
			if(CComQIPtr<IMediaPosition> pMP = pin)
			{
				HRESULT hr2 = pMP->put_Rate(dRate);
				
				if(*i == m_pFilter->GetInputPin())
				{
					hr = hr2;
				}
			}
		}
	}

	return hr;
}

STDMETHODIMP CStreamSwitcherPassThru::CanSeekForward(LONG* pCanSeekForward)
{
	CComPtr<IMediaPosition> pMP;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMP))) 
	{
		return pMP->CanSeekForward(pCanSeekForward);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CStreamSwitcherPassThru::CanSeekBackward(LONG* pCanSeekBackward) 
{
	CComPtr<IMediaPosition> pMP;

	if(SUCCEEDED(GetPeer(m_pFilter, &pMP))) 
	{
		return pMP->CanSeekBackward(pCanSeekBackward);
	}

	return E_NOTIMPL;
}

// CStreamSwitcherAllocator

CStreamSwitcherAllocator::CStreamSwitcherAllocator(CStreamSwitcherInputPin* pPin, HRESULT* phr)
	: CMemAllocator(NAME("CStreamSwitcherAllocator"), NULL, phr)
	, m_pPin(pPin)
	, m_fMediaTypeChanged(false)
{
	ASSERT(phr);
	ASSERT(pPin);
}

CStreamSwitcherAllocator::~CStreamSwitcherAllocator()
{
    ASSERT(m_bCommitted == FALSE);
}

STDMETHODIMP_(ULONG) CStreamSwitcherAllocator::NonDelegatingAddRef()
{
	return m_pPin->m_pFilter->AddRef();
}

STDMETHODIMP_(ULONG) CStreamSwitcherAllocator::NonDelegatingRelease()
{
	return m_pPin->m_pFilter->Release();
}

STDMETHODIMP CStreamSwitcherAllocator::GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	HRESULT hr = VFW_E_NOT_COMMITTED;

	if(!m_bCommitted)
	{
        return hr;
	}

	if(m_fMediaTypeChanged)
	{
		if(!m_pPin || !m_pPin->m_pFilter)
		{
			return hr;
		}

		CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pPin->m_pFilter)->GetOutputPin();

		if(!pOut || !pOut->CurrentAllocator())
		{
			return hr;
		}

		ALLOCATOR_PROPERTIES Properties, Actual;

		if(FAILED(pOut->CurrentAllocator()->GetProperties(&Actual))) 
		{
			return hr;
		}

		if(FAILED(GetProperties(&Properties))) 
		{
			return hr;
		}

		if(!m_bCommitted || Properties.cbBuffer < Actual.cbBuffer)
		{
			Properties.cbBuffer = Actual.cbBuffer;
			if(FAILED(Decommit())) return hr;
			if(FAILED(SetProperties(&Properties, &Actual))) return hr;
			if(FAILED(Commit())) return hr;
			ASSERT(Actual.cbBuffer >= Properties.cbBuffer);
			if(Actual.cbBuffer < Properties.cbBuffer) return hr;
		}
	}

	hr = CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);

	if(m_fMediaTypeChanged && SUCCEEDED(hr))
	{
		(*ppBuffer)->SetMediaType(&m_mt);

		m_fMediaTypeChanged = false;
	}

	return hr;
}

void CStreamSwitcherAllocator::NotifyMediaType(const CMediaType& mt)
{
	m_mt = mt;

	m_fMediaTypeChanged = true;
}

// CStreamSwitcherInputPin

CStreamSwitcherInputPin::CStreamSwitcherInputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr, LPCWSTR pName)
    : CBaseInputPin(NAME("CStreamSwitcherInputPin"), pFilter, &pFilter->m_csState, phr, pName)
	, m_Allocator(this, phr)
	, m_bSampleSkipped(FALSE)
	, m_bQualityChanged(FALSE)
	, m_bUsingOwnAllocator(FALSE)
	, m_hNotifyEvent(NULL)
{
	m_bCanReconnectWhenActive = TRUE;
}

STDMETHODIMP CStreamSwitcherInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IStreamSwitcherInputPin)
		IsConnected() && DirectShow::GetCLSID(GetConnected()) == __uuidof(NeroAudioDecoder) && QI(IPinConnection)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IPinConnection

STDMETHODIMP CStreamSwitcherInputPin::DynamicQueryAccept(const AM_MEDIA_TYPE* pmt)
{
	return QueryAccept(pmt);
}

STDMETHODIMP CStreamSwitcherInputPin::NotifyEndOfStream(HANDLE hNotifyEvent)
{
	if(m_hNotifyEvent)
	{
		SetEvent(m_hNotifyEvent);
	}

	m_hNotifyEvent = hNotifyEvent;

	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::IsEndPin()
{
	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::DynamicDisconnect()
{
	CAutoLock cAutoLock(&m_csReceive);

	Disconnect();

	return S_OK;
}

// IStreamSwitcherInputPin

STDMETHODIMP_(bool) CStreamSwitcherInputPin::IsActive()
{
	// TODO: lock onto something here

	return this == ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin();
}

// 

HRESULT CStreamSwitcherInputPin::QueryAcceptDownstream(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = S_OK;

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();

	if(pOut && pOut->IsConnected())
	{
		if(CComPtr<IPinConnection> pPC = pOut->CurrentPinConnection())
		{
			if(pPC->DynamicQueryAccept(pmt) == S_OK)
			{
				return S_OK;
			}
		}

		hr = pOut->GetConnected()->QueryAccept(pmt);
	}

	return hr;
}

HRESULT CStreamSwitcherInputPin::InitializeOutputSample(IMediaSample* pInSample, IMediaSample** ppOutSample)
{
	if(!pInSample || !ppOutSample)
	{
		return E_POINTER;
	}

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();

	ASSERT(pOut->GetConnected());

    CComPtr<IMediaSample> pOutSample;

	DWORD dwFlags = m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0;

    if(!(m_SampleProps.dwSampleFlags & AM_SAMPLE_SPLICEPOINT))
	{
		dwFlags |= AM_GBF_NOTASYNCPOINT;
	}

	HRESULT hr;

	hr = pOut->GetDeliveryBuffer(&pOutSample
        , m_SampleProps.dwSampleFlags & AM_SAMPLE_TIMEVALID ? &m_SampleProps.tStart : NULL
        , m_SampleProps.dwSampleFlags & AM_SAMPLE_STOPVALID ? &m_SampleProps.tStop : NULL
        , dwFlags);

    if(FAILED(hr) || pOutSample == NULL)
	{
		return E_FAIL;
	}

    if(CComQIPtr<IMediaSample2> pOutSample2 = pOutSample)
	{
        AM_SAMPLE2_PROPERTIES props;

		EXECUTE_ASSERT(SUCCEEDED(pOutSample2->GetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, tStart), (PBYTE)&props)));

        props.dwTypeSpecificFlags = m_SampleProps.dwTypeSpecificFlags;
        props.dwSampleFlags = (props.dwSampleFlags & AM_SAMPLE_TYPECHANGED) | (m_SampleProps.dwSampleFlags & ~AM_SAMPLE_TYPECHANGED);
        props.tStart = m_SampleProps.tStart;
        props.tStop  = m_SampleProps.tStop;
        props.cbData = FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId);

        hr = pOutSample2->SetProperties(FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId), (PBYTE)&props);
        
		if(m_SampleProps.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY)
		{
			m_bSampleSkipped = FALSE;
		}
    }
    else
	{
        if(m_SampleProps.dwSampleFlags & AM_SAMPLE_TIMEVALID)
		{
			pOutSample->SetTime(&m_SampleProps.tStart, &m_SampleProps.tStop);
		}

		if(m_SampleProps.dwSampleFlags & AM_SAMPLE_SPLICEPOINT)
		{
			pOutSample->SetSyncPoint(TRUE);
		}

		if(m_SampleProps.dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY)
		{
			pOutSample->SetDiscontinuity(TRUE);

            m_bSampleSkipped = FALSE;
        }

		LONGLONG MediaStart, MediaEnd;

        if(pInSample->GetMediaTime(&MediaStart, &MediaEnd) == NOERROR)
		{
			pOutSample->SetMediaTime(&MediaStart, &MediaEnd);
		}
    }

	*ppOutSample = pOutSample.Detach();

	return S_OK;
}

HRESULT CStreamSwitcherInputPin::CheckMediaType(const CMediaType* pmt)
{
	return ((CStreamSwitcherFilter*)m_pFilter)->CheckMediaType(pmt);
}

HRESULT CStreamSwitcherInputPin::CheckConnect(IPin* pPin)
{
	return (IPin*)((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin() == pPin ? E_FAIL : __super::CheckConnect(pPin);
}

HRESULT CStreamSwitcherInputPin::CompleteConnect(IPin* pReceivePin)
{
	HRESULT hr;
	
	hr = __super::CompleteConnect(pReceivePin);

	if(FAILED(hr)) 
	{
		return hr;
	}

    ((CStreamSwitcherFilter*)m_pFilter)->CompleteConnect(PINDIR_INPUT, this, pReceivePin);

	std::wstring fileName;
	std::wstring pinName;

    IPin* pPin = (IPin*)this;
	IBaseFilter* pBF = (IBaseFilter*)m_pFilter;

	while((pPin = DirectShow::GetUpStreamPin(pBF, pPin)) && (pBF = DirectShow::GetFilter(pPin)))
	{
		if(DirectShow::IsSplitter(pBF))
		{
			pinName = DirectShow::GetName(pPin);
		}

		if(CComQIPtr<IFileSourceFilter> pFSF = pBF)
		{
			WCHAR* pszName = NULL;
			AM_MEDIA_TYPE mt;

			if(SUCCEEDED(pFSF->GetCurFile(&pszName, &mt)) && pszName)
			{
				fileName = pszName;

				CoTaskMemFree(pszName);

				std::replace(fileName.begin(), fileName.end(), '\\', '/');

				std::wstring fn = fileName.substr(fileName.find_last_of('/') + 1);

				if(!fn.empty())
				{
					fileName = fn;
				}

				if(!pinName.empty()) 
				{
					fileName += L" / " + pinName;
				}

				WCHAR* buff = new WCHAR[fileName.size() + 1];

				wcscpy(buff, fileName.c_str());

				if(m_pName) delete [] m_pName;

				m_pName = buff;
			}

			break;
		}

		pPin = DirectShow::GetFirstPin(pBF);
	}

	m_hNotifyEvent = NULL;

	return S_OK;
}

// IPin

STDMETHODIMP CStreamSwitcherInputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = __super::QueryAccept(pmt);

	if(S_OK != hr) return hr;

	return QueryAcceptDownstream(pmt);
}

STDMETHODIMP CStreamSwitcherInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	// FIXME: this locked up once
	// CAutoLock cAutoLock(&((CStreamSwitcherFilter*)m_pFilter)->m_csReceive);

	HRESULT hr;

	if(S_OK != (hr = QueryAcceptDownstream(pmt)))
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

    if(m_Connected && m_Connected != pConnector)
	{
        return VFW_E_ALREADY_CONNECTED;
	}

	if(m_Connected) 
	{
		m_Connected->Release();
		m_Connected = NULL;
	}

	return __super::ReceiveConnection(pConnector, pmt);
}

STDMETHODIMP CStreamSwitcherInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
    CheckPointer(ppAllocator, E_POINTER);

    if(m_pAllocator == NULL)
	{
        (m_pAllocator = &m_Allocator)->AddRef();
    }

    (*ppAllocator = m_pAllocator)->AddRef();

    return NOERROR;
}

STDMETHODIMP CStreamSwitcherInputPin::NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly)
{
	HRESULT hr = __super::NotifyAllocator(pAllocator, bReadOnly);

	if(FAILED(hr)) return hr;

	m_bUsingOwnAllocator = (pAllocator == (IMemAllocator*)&m_Allocator);

	return S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::BeginFlush()
{
    CAutoLock cAutoLock(&((CStreamSwitcherFilter*)m_pFilter)->m_csState);

	HRESULT hr;

	CStreamSwitcherFilter* pSSF = (CStreamSwitcherFilter*)m_pFilter;

	CStreamSwitcherOutputPin* pOut = pSSF->GetOutputPin();

    if(!IsConnected() || !pOut || !pOut->IsConnected())
	{
		return VFW_E_NOT_CONNECTED;
	}

	hr = __super::BeginFlush();

    if(FAILED(hr)) 
	{
		return hr;
	}

	return IsActive() ? pSSF->DeliverBeginFlush() : S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::EndFlush()
{
	CAutoLock cAutoLock(&((CStreamSwitcherFilter*)m_pFilter)->m_csState);

	HRESULT hr;

	CStreamSwitcherFilter* pSSF = (CStreamSwitcherFilter*)m_pFilter;

	CStreamSwitcherOutputPin* pOut = pSSF->GetOutputPin();

    if(!IsConnected() || !pOut || !pOut->IsConnected())
	{
		return VFW_E_NOT_CONNECTED;
	}

	hr = __super::EndFlush();

    if(FAILED(hr)) 
	{
		return hr;
	}

	return IsActive() ? pSSF->DeliverEndFlush() : S_OK;
}

STDMETHODIMP CStreamSwitcherInputPin::EndOfStream()
{
    CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherFilter* pSSF = (CStreamSwitcherFilter*)m_pFilter;

	CStreamSwitcherOutputPin* pOut = pSSF->GetOutputPin();

	if(!IsConnected() || !pOut || !pOut->IsConnected())
	{
		return VFW_E_NOT_CONNECTED;
	}

	if(m_hNotifyEvent)
	{
		SetEvent(m_hNotifyEvent);

		m_hNotifyEvent = NULL;

		return S_OK;
	}

	return IsActive() ? pSSF->DeliverEndOfStream() : S_OK;
}

// IMemInputPin

STDMETHODIMP CStreamSwitcherInputPin::Receive(IMediaSample* pSample)
{
	AM_MEDIA_TYPE* pmt = NULL;

	if(SUCCEEDED(pSample->GetMediaType(&pmt)) && pmt != NULL)
	{
		CMediaType mt = *pmt;

		SetMediaType(&mt);

		DeleteMediaType(pmt);
	}

	if(!IsActive())
	{
		return E_FAIL;
	}

    CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherOutputPin* pOut = ((CStreamSwitcherFilter*)m_pFilter)->GetOutputPin();

	ASSERT(pOut->GetConnected());

	HRESULT hr = __super::Receive(pSample);

	if(S_OK != hr) 
	{
		return hr;
	}

	if(m_SampleProps.dwStreamId != AM_STREAM_MEDIA)
	{
		return pOut->Deliver(pSample);
	}

	//

	ALLOCATOR_PROPERTIES props, actual;

	hr = m_pAllocator->GetProperties(&props);
	hr = pOut->CurrentAllocator()->GetProperties(&actual);

	REFERENCE_TIME rtStart = 0, rtStop = 0;

	if(S_OK == pSample->GetTime(&rtStart, &rtStop))
	{
		//
	}

	long cbBuffer = pSample->GetActualDataLength();

	CMediaType mtOut = m_mt;

	mtOut = ((CStreamSwitcherFilter*)m_pFilter)->CreateNewOutputMediaType(mtOut, cbBuffer);

	bool fTypeChanged = false;

	if(mtOut != pOut->CurrentMediaType() || cbBuffer > actual.cbBuffer)
	{
		fTypeChanged = true;

		m_SampleProps.dwSampleFlags |= AM_SAMPLE_TYPECHANGED; // | AM_SAMPLE_DATADISCONTINUITY | AM_SAMPLE_TIMEDISCONTINUITY;
/*
		if(CComQIPtr<IPinConnection> pPC = pOut->CurrentPinConnection())
		{
			HANDLE hEOS = CreateEvent(NULL, FALSE, FALSE, NULL);
			hr = pPC->NotifyEndOfStream(hEOS);
			hr = pOut->DeliverEndOfStream();
			WaitForSingleObject(hEOS, 3000);
			CloseHandle(hEOS);
			hr = pOut->DeliverBeginFlush();
			hr = pOut->DeliverEndFlush();
		}
*/
		if(props.cBuffers < 8 && mtOut.majortype == MEDIATYPE_Audio)
		{
			props.cBuffers = 1;//8;
		}

		props.cbBuffer = cbBuffer;

		if(actual.cbAlign != props.cbAlign
		|| actual.cbPrefix != props.cbPrefix
		|| actual.cBuffers < props.cBuffers
		|| actual.cbBuffer < props.cbBuffer)
		{
			hr = pOut->DeliverBeginFlush();
			hr = pOut->DeliverEndFlush();
			hr = pOut->CurrentAllocator()->Decommit();
			hr = pOut->CurrentAllocator()->SetProperties(&props, &actual);
			hr = pOut->CurrentAllocator()->Commit();
		}
	}

	CComPtr<IMediaSample> pOutSample;

	if(FAILED(InitializeOutputSample(pSample, &pOutSample)))
	{
		return E_FAIL;
	}

	pmt = NULL;

	if(SUCCEEDED(pOutSample->GetMediaType(&pmt)) && pmt != NULL)
	{
		ASSERT(0);

		// TODO

		DeleteMediaType(pmt);
	}

	if(fTypeChanged)
	{
		pOut->SetMediaType(&mtOut);

		((CStreamSwitcherFilter*)m_pFilter)->OnNewOutputMediaType(m_mt, mtOut);

		pOutSample->SetMediaType(&mtOut);
	}

	hr = ((CStreamSwitcherFilter*)m_pFilter)->Transform(pSample, pOutSample);

    if(S_OK == hr)
	{
		hr = pOut->Deliver(pOutSample);

        m_bSampleSkipped = FALSE;
/*
		if(FAILED(hr))
		{
			ASSERT(0);
		}
*/
	}
    else if(S_FALSE == hr)
	{
		hr = S_OK;

		pOutSample = NULL;

		m_bSampleSkipped = TRUE;
		
		if(!m_bQualityChanged)
		{
			m_pFilter->NotifyEvent(EC_QUALITY_CHANGE, 0, 0);

			m_bQualityChanged = TRUE;
		}
	}

	return hr;
}

STDMETHODIMP CStreamSwitcherInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if(!IsConnected())
	{
		return S_OK;
	}

	CAutoLock cAutoLock(&m_csReceive);

	CStreamSwitcherFilter* pSSF = (CStreamSwitcherFilter*)m_pFilter;

	CStreamSwitcherOutputPin* pOut = pSSF->GetOutputPin();

    if(!pOut || !pOut->IsConnected())
	{
		return VFW_E_NOT_CONNECTED;
	}

	return pSSF->DeliverNewSegment(tStart, tStop, dRate);
}

//
// CStreamSwitcherOutputPin
//

CStreamSwitcherOutputPin::CStreamSwitcherOutputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr)
	: CBaseOutputPin(NAME("CStreamSwitcherOutputPin"), pFilter, &pFilter->m_csState, phr, L"Out")
{
//	m_bCanReconnectWhenActive = TRUE;
}

STDMETHODIMP CStreamSwitcherOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    *ppv = NULL;

    if(riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
	{
        if(m_pStreamSwitcherPassThru == NULL)
		{
			HRESULT hr = S_OK;

			m_pStreamSwitcherPassThru = (IUnknown*)(INonDelegatingUnknown*)new CStreamSwitcherPassThru(GetOwner(), &hr, (CStreamSwitcherFilter*)m_pFilter);

            if(FAILED(hr)) return hr;
        }

        return m_pStreamSwitcherPassThru->QueryInterface(riid, ppv);
    }
/*
	else if(riid == IID_IStreamBuilder)
	{
		return GetInterface((IStreamBuilder*)this, ppv);		
	}
*/
	return __super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CStreamSwitcherOutputPin::QueryAcceptUpstream(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = S_FALSE;

	CStreamSwitcherInputPin* pIn = ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin();

	if(pIn && pIn->IsConnected() && (pIn->IsUsingOwnAllocator() || pIn->CurrentMediaType() == *pmt))
	{
		if(CComQIPtr<IPin> pPinTo = pIn->GetConnected())
		{
			if(S_OK != (hr = pPinTo->QueryAccept(pmt)))
			{
				return VFW_E_TYPE_NOT_ACCEPTED;
			}
		}
		else
		{
			return E_FAIL;
		}
	}

	return hr;
}

HRESULT CStreamSwitcherOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	CStreamSwitcherInputPin* pIn = ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin();

	if(!pIn || !pIn->IsConnected()) 
	{
		return E_UNEXPECTED;
	}

	HRESULT hr;

	CComPtr<IMemAllocator> pAllocatorIn;

	hr = pIn->GetAllocator(&pAllocatorIn);

	if(FAILED(hr) || !pAllocatorIn)
	{
		return E_UNEXPECTED;
	}

    if(FAILED(hr = pAllocatorIn->GetProperties(pProperties))) 
	{
		return hr;
	}

	if(pProperties->cBuffers < 8 && pIn->CurrentMediaType().majortype == MEDIATYPE_Audio)
	{
		pProperties->cBuffers = 1;//8;
	}

	return NOERROR;
}

HRESULT CStreamSwitcherOutputPin::CheckConnect(IPin* pPin)
{
	CComPtr<IBaseFilter> pBF = DirectShow::GetFilter(pPin);

	return DirectShow::IsAudioWaveRenderer(pBF) || DirectShow::GetCLSID(pBF) == __uuidof(MatrixMixer) ? __super::CheckConnect(pPin) : E_FAIL;

//	return CComQIPtr<IPinConnection>(pPin) ? CBaseOutputPin::CheckConnect(pPin) : E_NOINTERFACE;
//	return CBaseOutputPin::CheckConnect(pPin);
}

HRESULT CStreamSwitcherOutputPin::BreakConnect()
{
	m_pPinConnection = NULL;

	return __super::BreakConnect();
}

HRESULT CStreamSwitcherOutputPin::CompleteConnect(IPin* pReceivePin)
{
	m_pPinConnection = CComQIPtr<IPinConnection>(pReceivePin);

	return __super::CompleteConnect(pReceivePin);
}

HRESULT CStreamSwitcherOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return ((CStreamSwitcherFilter*)m_pFilter)->CheckMediaType(pmt);
}

HRESULT CStreamSwitcherOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	CStreamSwitcherInputPin* pIn = ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin();

	if(!pIn || !pIn->IsConnected())
	{
		return E_UNEXPECTED;
	}

	CComPtr<IEnumMediaTypes> pEM;

	if(FAILED(pIn->GetConnected()->EnumMediaTypes(&pEM)))
	{
		return VFW_S_NO_MORE_ITEMS;
	}

	if(iPosition > 0 && FAILED(pEM->Skip(iPosition)))
	{
		return VFW_S_NO_MORE_ITEMS;
	}

	AM_MEDIA_TYPE* tmp = NULL;

	if(S_OK != pEM->Next(1, &tmp, NULL) || tmp == NULL)
	{
		return VFW_S_NO_MORE_ITEMS;
	}

	CopyMediaType(pmt, tmp);

	DeleteMediaType(tmp);
/*
	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	CopyMediaType(pmt, &pIn->CurrentMediaType());
*/
	return S_OK;
}

// IPin

STDMETHODIMP CStreamSwitcherOutputPin::QueryAccept(const AM_MEDIA_TYPE* pmt)
{
	HRESULT hr = __super::QueryAccept(pmt);

	if(S_OK != hr) return hr;

	return QueryAcceptUpstream(pmt);
}

// IQualityControl

STDMETHODIMP CStreamSwitcherOutputPin::Notify(IBaseFilter* pSender, Quality q)
{
	CStreamSwitcherInputPin* pIn = ((CStreamSwitcherFilter*)m_pFilter)->GetInputPin();

	if(!pIn || !pIn->IsConnected()) return VFW_E_NOT_CONNECTED;

    return pIn->PassNotify(q);
}

// IStreamBuilder

STDMETHODIMP CStreamSwitcherOutputPin::Render(IPin* ppinOut, IGraphBuilder* pGraph)
{
	CComPtr<IBaseFilter> pBF;

	pBF.CoCreateInstance(CLSID_DSoundRender);

	if(!pBF || FAILED(pGraph->AddFilter(pBF, L"Default DirectSound Device")))
	{
		return E_FAIL;
	}

	CComPtr<IPin> pin = DirectShow::GetFirstDisconnectedPin(pBF, PINDIR_INPUT);

	if(FAILED(pGraph->ConnectDirect(ppinOut, pin, NULL)))
	{
		pGraph->RemoveFilter(pBF);

		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CStreamSwitcherOutputPin::Backout(IPin* ppinOut, IGraphBuilder* pGraph)
{
	return S_OK;
}

//
// CStreamSwitcherFilter
//

CStreamSwitcherFilter::CStreamSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr, const CLSID& clsid) 
	: CBaseFilter(NAME("CStreamSwitcherFilter"), lpunk, &m_csState, clsid)
{
	if(phr) *phr = S_OK;

	HRESULT hr = S_OK;

	m_pInput = new CStreamSwitcherInputPin(this, &hr, L"Channel 1");
	m_pOutput = new CStreamSwitcherOutputPin(this, &hr);

	m_pInputs.push_back(m_pInput);
}

CStreamSwitcherFilter::~CStreamSwitcherFilter()
{
	std::for_each(m_pInputs.begin(), m_pInputs.end(), [] (CStreamSwitcherInputPin* pin) {delete pin;});
	delete m_pOutput;

	m_pInput = NULL;
	m_pOutput = NULL;
}

STDMETHODIMP CStreamSwitcherFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IAMStreamSelect)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

int CStreamSwitcherFilter::GetPinCount()
{
	CAutoLock cAutoLock(&m_csPins);

	return(1 + (int)m_pInputs.size());
}

CBasePin* CStreamSwitcherFilter::GetPin(int n)
{
	CAutoLock cAutoLock(&m_csPins);

	if(n == 0) 
	{
		return m_pOutput;
	}

	n--;

	if(n >= 0 && n < GetPinCount()) 
	{
		return m_pInputs[n];
	}

	return NULL;
}

int CStreamSwitcherFilter::GetConnectedInputPinCount()
{
	CAutoLock cAutoLock(&m_csPins);

	int n = 0;

	std::for_each(m_pInputs.begin(), m_pInputs.end(), [&] (CStreamSwitcherInputPin* pin) 
	{
		if(pin->IsConnected()) 
		{
			n++;
		}
	});

	return n;
}

CStreamSwitcherInputPin* CStreamSwitcherFilter::GetConnectedInputPin(int n)
{
	CStreamSwitcherInputPin* ret = NULL;

	std::for_each(m_pInputs.begin(), m_pInputs.end(), [&] (CStreamSwitcherInputPin* pin) 
	{
		if(pin->IsConnected()) 
		{
			if(n-- == 0) 
			{
				ret = pin;
			}
		}
	});

	return ret;
}

CStreamSwitcherInputPin* CStreamSwitcherFilter::GetInputPin()
{
	return m_pInput;
}

CStreamSwitcherOutputPin* CStreamSwitcherFilter::GetOutputPin()
{
	return m_pOutput;
}

HRESULT CStreamSwitcherFilter::CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin, IPin* pReceivePin)
{
	if(dir == PINDIR_INPUT)
	{
		CAutoLock cAutoLock(&m_csPins);

		int nConnected = GetConnectedInputPinCount();

		if(nConnected == 1)
		{
			m_pInput = (CStreamSwitcherInputPin*)pPin;
		}

		if(nConnected == m_pInputs.size())
		{
			std::wstring name = Util::Format(L"Channel %d", ++m_PinVersion);

			HRESULT hr = S_OK;

			CStreamSwitcherInputPin* pin = new CStreamSwitcherInputPin(this, &hr, name.c_str());

			m_pInputs.push_back(pin);
		}
	}

	return S_OK;
}

// this should be very thread safe, I hope it is, it must be... :)

void CStreamSwitcherFilter::SelectInput(CStreamSwitcherInputPin* pInput)
{
	// make sure no input thinks it is active

	m_pInput = NULL;

	// this will let waiting GetBuffer() calls go on inside our Receive()

	if(m_pOutput)
	{
		m_pOutput->DeliverBeginFlush();
		m_pOutput->DeliverEndFlush();
	}

	// set new input

	m_pInput = pInput;
}

HRESULT CStreamSwitcherFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	HRESULT hr;

	if(FAILED(hr = pIn->GetPointer(&pDataIn)))
	{
		return hr;
	}

	if(FAILED(hr = pOut->GetPointer(&pDataOut))) 
	{
		return hr;
	}

	long len = pIn->GetActualDataLength();
	long size = pOut->GetSize();

	if(!pDataIn || !pDataOut /*|| len > size || len <= 0*/) 
	{
		return S_FALSE; // FIXME
	}

	memcpy(pDataOut, pDataIn, min(len, size));

	pOut->SetActualDataLength(min(len, size));

	return S_OK;
}

CMediaType CStreamSwitcherFilter::CreateNewOutputMediaType(CMediaType mt, long& cbBuffer)
{
	return mt;
}

HRESULT CStreamSwitcherFilter::DeliverEndOfStream()
{
	return m_pOutput ? m_pOutput->DeliverEndOfStream() : E_FAIL;
}

HRESULT CStreamSwitcherFilter::DeliverBeginFlush()
{
	return m_pOutput ? m_pOutput->DeliverBeginFlush() : E_FAIL;
}

HRESULT CStreamSwitcherFilter::DeliverEndFlush()
{
	return m_pOutput ? m_pOutput->DeliverEndFlush() : E_FAIL;
}

HRESULT CStreamSwitcherFilter::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	return m_pOutput ? m_pOutput->DeliverNewSegment(tStart, tStop, dRate) : E_FAIL;
}

// IAMStreamSelect

STDMETHODIMP CStreamSwitcherFilter::Count(DWORD* pcStreams)
{
	if(!pcStreams) return E_POINTER;

	CAutoLock cAutoLock(&m_csPins);

	*pcStreams = GetConnectedInputPinCount();

	return S_OK;
}

STDMETHODIMP CStreamSwitcherFilter::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	CAutoLock cAutoLock(&m_csPins);

	CBasePin* pPin = GetConnectedInputPin(lIndex);

	if(!pPin) 
	{
		return E_INVALIDARG;
	}

	if(ppmt)
	{
		*ppmt = CreateMediaType(&m_pOutput->CurrentMediaType());
	}

	if(pdwFlags)
	{
		*pdwFlags = (m_pInput == pPin) ? AMSTREAMSELECTINFO_EXCLUSIVE : 0;
	}

	if(plcid)
	{
		*plcid = 0;
	}

	if(pdwGroup)
	{
		*pdwGroup = 0;
	}

	if(ppszName && (*ppszName = (WCHAR*)CoTaskMemAlloc((wcslen(pPin->Name()) + 1) * sizeof(WCHAR))))
	{
		wcscpy(*ppszName, pPin->Name());
	}

	if(ppObject)
	{
		*ppObject = NULL;
	}

	if(ppUnk)
	{
		(*ppUnk = (IPin*)pPin)->AddRef();
	}

	return S_OK;
}

STDMETHODIMP CStreamSwitcherFilter::Enable(long lIndex, DWORD dwFlags)
{
	if(dwFlags != AMSTREAMSELECTENABLE_ENABLE)
	{
		return E_NOTIMPL;
	}

	PauseGraph;

	if(CStreamSwitcherInputPin* pNewInput = GetConnectedInputPin(lIndex))
	{
		SelectInput(pNewInput);
	}

	ResumeGraph;

	return E_INVALIDARG;
}