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
#include "BufferFilter.h"
#include "DirectShow.h"

//
// CBufferFilter
//

CBufferFilter::CBufferFilter(LPUNKNOWN lpunk, HRESULT* phr) 
	: CTransformFilter(NAME("CBufferFilter"), lpunk, __uuidof(this))
	, m_nSamplesToBuffer(2)
{
	HRESULT hr = S_OK;

	m_pInput = new CTransformInputPin(NAME("Transform input pin"), this, &hr, L"In");

	if(FAILED(hr) && phr != NULL) *phr = hr;

	m_pOutput = new CBufferFilterOutputPin(this, &hr);

	if(FAILED(hr) && phr != NULL) *phr = hr;
}

CBufferFilter::~CBufferFilter()
{
}

STDMETHODIMP CBufferFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(IBufferFilter)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IBufferFilter

STDMETHODIMP CBufferFilter::SetBuffers(int nBuffers)
{
	if(!m_pOutput)
	{
		return E_FAIL;
	}

	if(m_pOutput->IsConnected()) // TODO: allow "on-the-fly" changes
	{
		return VFW_E_ALREADY_CONNECTED;
	}

	m_nSamplesToBuffer = nBuffers;

	return S_OK;
}

STDMETHODIMP_(int) CBufferFilter::GetBuffers()
{
	return m_nSamplesToBuffer;
}

STDMETHODIMP_(int) CBufferFilter::GetFreeBuffers()
{
	CBufferFilterOutputPin* pPin = (CBufferFilterOutputPin*)m_pOutput;

	if(pPin && pPin->m_pOutputQueue)
	{
		return m_nSamplesToBuffer - pPin->m_pOutputQueue->GetQueueCount();
	}

	return 0;
}

STDMETHODIMP CBufferFilter::SetPriority(DWORD dwPriority)
{
	CBufferFilterOutputPin* pPin = (CBufferFilterOutputPin*)m_pOutput;

	if(pPin && pPin->m_pOutputQueue)
	{
		return pPin->m_pOutputQueue->SetPriority(dwPriority) ? S_OK : E_FAIL;
	}

	return E_UNEXPECTED;
}

//

HRESULT CBufferFilter::Receive(IMediaSample* pSample)
{
	/*  Check for other streams and pass them on */

	AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();

	if(pProps->dwStreamId != AM_STREAM_MEDIA)
	{
		return m_pOutput->Deliver(pSample);
	}

	HRESULT hr;

	ASSERT(pSample);

	CComPtr<IMediaSample> pOutSample;

	ASSERT(m_pOutput != NULL);

	// Set up the output sample

	hr = InitializeOutputSample(pSample, &pOutSample);

	if(FAILED(hr))
	{
		return hr;
	}

	// have the derived class transform the data

	hr = Transform(pSample, pOutSample);

	if(FAILED(hr))
	{
		DbgLog((LOG_TRACE,1,TEXT("Error from transform")));
	}
	else
	{
		// the Transform() function can return S_FALSE to indicate that the
		// sample should not be delivered; we only deliver the sample if it's
		// really S_OK (same as NOERROR, of course.)

		if(hr == NOERROR)
		{
			hr = m_pOutput->Deliver(pOutSample);

			m_bSampleSkipped = FALSE;   // last thing no longer dropped
		}
		else
		{
			// S_FALSE returned from Transform is a PRIVATE agreement
			// We should return NOERROR from Receive() in this cause because returning S_FALSE
			// from Receive() means that this is the end of the stream and no more data should
			// be sent.

			if(S_FALSE == hr)
			{
				//  Release the sample before calling notify to avoid
				//  deadlocks if the sample holds a lock on the system
				//  such as DirectDraw buffers do

				m_bSampleSkipped = TRUE;

				if(!m_bQualityChanged)
				{
					NotifyEvent(EC_QUALITY_CHANGE, 0, 0);

					m_bQualityChanged = TRUE;
				}

				return NOERROR;
			}
		}
	}

	// release the output buffer. If the connected pin still needs it,
	// it will have addrefed it itself.

	return hr;
}

HRESULT CBufferFilter::Transform(IMediaSample* pIn, IMediaSample* pOut)
{
	BYTE* pDataIn = NULL;
	BYTE* pDataOut = NULL;

	pIn->GetPointer(&pDataIn);
	pOut->GetPointer(&pDataOut);

	long len = pIn->GetActualDataLength();
	long size = pOut->GetSize();

	if(!pDataIn || !pDataOut || len > size || len <= 0)
	{
		return S_FALSE;
	}

	memcpy(pDataOut, pDataIn, min(len, size));
	
	pOut->SetActualDataLength(min(len, size));

	return S_OK;
}

HRESULT CBufferFilter::CheckInputType(const CMediaType* mtIn)
{
	return S_OK;
}

HRESULT CBufferFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	return mtIn->MatchesPartial(mtOut) ? S_OK : VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CBufferFilter::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) 
	{
		return E_UNEXPECTED;
	}

	CComPtr<IMemAllocator> pAllocatorIn;

	m_pInput->GetAllocator(&pAllocatorIn);

	if(pAllocatorIn == NULL)
	{
		return E_UNEXPECTED;
	}

	pAllocatorIn->GetProperties(pProperties);

	pProperties->cBuffers = max(m_nSamplesToBuffer, pProperties->cBuffers);

    return NOERROR;
}

HRESULT CBufferFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	// TODO: offer all input types from upstream and allow reconnection at least in stopped state

	if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt = m_pInput->CurrentMediaType();

	return S_OK;
}

HRESULT CBufferFilter::StopStreaming()
{
	CBufferFilterOutputPin* pPin = (CBufferFilterOutputPin*)m_pOutput;

	if(m_pInput && pPin && pPin->m_pOutputQueue)
	{
		while(!m_pInput->IsFlushing() && pPin->m_pOutputQueue->GetQueueCount() > 0) 
		{
			Sleep(50);
		}
	}

	return __super::StopStreaming();
}

//
// CBufferFilterOutputPin
//

CBufferFilterOutputPin::CBufferFilterOutputPin(CTransformFilter* pFilter, HRESULT* phr)
	: CTransformOutputPin(NAME("CBufferFilterOutputPin"), pFilter, phr, L"Out")
	, m_pOutputQueue(NULL)
{
}

CBufferFilterOutputPin::~CBufferFilterOutputPin()
{
	delete m_pOutputQueue;
}

HRESULT CBufferFilterOutputPin::Active()
{
	CAutoLock lock_it(m_pLock);

	if(m_Connected && !m_pOutputQueue)
	{
		HRESULT hr = NOERROR;

		m_pOutputQueue = new CBufferFilterOutputQueue(m_Connected, &hr);

		if(FAILED(hr))
		{
			delete m_pOutputQueue;

			return hr;
		}
	}

	return __super::Active();
}

HRESULT CBufferFilterOutputPin::Inactive()
{
	CAutoLock cAutoLock(m_pLock);

	delete m_pOutputQueue;

	return __super::Inactive();
}

HRESULT CBufferFilterOutputPin::Deliver(IMediaSample* pMediaSample)
{
	if(m_pOutputQueue)
	{
		pMediaSample->AddRef();

		return m_pOutputQueue->Receive(pMediaSample);
	}

	return NOERROR;
}

HRESULT CBufferFilterOutputPin::DeliverEndOfStream()
{
	if(m_pOutputQueue)
	{
		m_pOutputQueue->EOS();
	}

	return NOERROR;
}

HRESULT CBufferFilterOutputPin::DeliverBeginFlush()
{
	if(m_pOutputQueue)
	{
		m_pOutputQueue->BeginFlush();
	}

	return NOERROR;
}

HRESULT CBufferFilterOutputPin::DeliverEndFlush()
{
	if(m_pOutputQueue)
	{
		m_pOutputQueue->EndFlush();
	}

	return NOERROR;
}

HRESULT CBufferFilterOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if(m_pOutputQueue)
	{
		m_pOutputQueue->NewSegment(tStart, tStop, dRate);
	}

	return NOERROR;
}
