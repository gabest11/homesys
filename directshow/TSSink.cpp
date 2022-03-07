#include "stdafx.h"
#include "TSSink.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

// CTSSourceFilter

CTSSourceFilter::CTSSourceFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseFilter(NAME("CTSDemuxFilter"), lpunk, this, __uuidof(this))
	, m_save(0)
{
	if(phr) *phr = S_OK;

	m_output = new CTSSourceOutputPin(this, phr);
}

CTSSourceFilter::~CTSSourceFilter()
{
	delete m_output;

	for(auto i = m_data.begin(); i != m_data.end(); i++)
	{
		delete *i;
	}

	m_data.clear();
}

STDMETHODIMP CTSSourceFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ITSSourceFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

int CTSSourceFilter::GetPinCount()
{
	return 1;
}

CBasePin* CTSSourceFilter::GetPin(int n)
{
	return n == 0 ? m_output : NULL;
}

STDMETHODIMP CTSSourceFilter::Stop()
{
	CAutoLock cAutoLock(this);

	if(m_save == 1)
	{
		m_save = 2;
	}

	return __super::Stop();
}

// ITSSourceFilter

STDMETHODIMP CTSSourceFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	return m_output->NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CTSSourceFilter::Receive(const BYTE* buff, int len)
{
	HRESULT hr;

	if(m_save > 0)
	{
		if(len > 0)
		{
			std::vector<BYTE>* p = new std::vector<BYTE>(len);

			memcpy(p->data(), buff, len);

			m_data.push_back(p);
		}
	}

	CComPtr<IMediaSample> sample;

	if(FAILED(m_output->GetDeliveryBuffer(&sample, NULL, NULL, 0)))
	{
		return E_FAIL;
	}

	if(m_save == 2)
	{
		m_save = 0;

		sample = NULL;

		std::list<std::vector<BYTE>*> data;

		data.swap(m_data);

		hr = S_OK;

		for(auto i = data.begin(); i != data.end(); i++)
		{
			hr = Receive((*i)->data(), (*i)->size());

			if(FAILED(hr)) break;
		}

		for(auto i = data.begin(); i != data.end(); i++)
		{
			delete *i;
		}

		return hr;
	}

	BYTE* dst = NULL;

	if(FAILED(sample->GetPointer(&dst)) || dst == NULL)
	{
		return E_FAIL;
	}

	int size = sample->GetSize();

	if(len > size)
	{
		len = size;
	}

	memcpy(dst, buff, len);

	sample->SetActualDataLength(len);

	sample->SetTime(NULL, NULL);
	sample->SetMediaTime(NULL, NULL);

	sample->SetDiscontinuity(FALSE);
	sample->SetPreroll(FALSE);
	sample->SetSyncPoint(FALSE);

	return m_output->Deliver(sample);
}

STDMETHODIMP CTSSourceFilter::SaveInput()
{
	m_save = 1;

	return S_OK;
}

// CTSSourceFilter::CTSSourceOutputPin

CTSSourceFilter::CTSSourceOutputPin::CTSSourceOutputPin(CTSSourceFilter* pFilter, HRESULT* phr)
	: CBaseOutputPin(_T("Output"), pFilter, pFilter, phr, L"Output")
{
}

HRESULT CTSSourceFilter::CTSSourceOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == MEDIATYPE_Stream 
		&& pmt->subtype == MEDIASUBTYPE_MPEG2_TRANSPORT 
		? S_OK 
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CTSSourceFilter::CTSSourceOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 1) return VFW_S_NO_MORE_ITEMS;

	pmt->majortype = MEDIATYPE_Stream;

	switch(iPosition)
	{
	case 0: pmt->subtype = MEDIASUBTYPE_MPEG2_TRANSPORT; break;
	case 1: pmt->subtype = KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT; break;
	}

	pmt->formattype = FORMAT_None;

	return S_OK;
}

HRESULT CTSSourceFilter::CTSSourceOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 1 << 20;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	return NOERROR;
}

// CTSSinkFilter

CTSSinkFilter::CTSSinkFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseFilter(NAME("CTSSinkFilter"), lpunk, this, __uuidof(this))
	, m_source(NULL)
{
	if(phr) *phr = S_OK;

	m_input = new CTSSinkInputPin(this, phr);
	m_output = new CTIFOutputPin(this, this, phr);
}

CTSSinkFilter::~CTSSinkFilter()
{
	delete m_input;
	delete m_output;
}

STDMETHODIMP CTSSinkFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ITSSinkFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

//

int CTSSinkFilter::GetPinCount()
{
	return 2;
}

CBasePin* CTSSinkFilter::GetPin(int n)
{
	if(n == 0) return m_input;
	else if(n == 1) return m_output;
	else return NULL;
}

HRESULT CTSSinkFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if(m_source != NULL)
	{
		m_source->NewSegment(tStart, tStop, dRate);
	}

	return S_OK;
}

HRESULT CTSSinkFilter::Receive(IMediaSample* pSample)
{
	if(m_source != NULL)
	{
		int len = pSample->GetActualDataLength();

		if(len > 0)
		{
			BYTE* src = NULL;

			if(SUCCEEDED(pSample->GetPointer(&src)))
			{
				m_source->Receive(src, len);
			}
		}
	}

	return S_OK;
}

// ITSSinkFilter

STDMETHODIMP CTSSinkFilter::SetSource(ITSSourceFilter* pSource)
{
	CAutoLock csAutoLock(this);

	if(m_State != State_Stopped)
	{
		return E_UNEXPECTED;
	}

	m_source = pSource;

	return S_OK;
}

// CTSSinkFilter::CTSSinkInputPin

CTSSinkFilter::CTSSinkInputPin::CTSSinkInputPin(CTSSinkFilter* pFilter, HRESULT* phr)
	: CBaseInputPin(_T("Input"), pFilter, pFilter, phr, L"Input")
{
}

HRESULT CTSSinkFilter::CTSSinkInputPin::CheckMediaType(const CMediaType* pmt)
{
	if(pmt->majortype == MEDIATYPE_Stream)
	{
		if(pmt->subtype == KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT || pmt->subtype == MEDIASUBTYPE_MPEG2_TRANSPORT)
		{
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

STDMETHODIMP CTSSinkFilter::CTSSinkInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	return ((CTSSinkFilter*)m_pFilter)->NewSegment(tStart, tStop, dRate);
}

STDMETHODIMP CTSSinkFilter::CTSSinkInputPin::Receive(IMediaSample* pSample)
{
	return ((CTSSinkFilter*)m_pFilter)->Receive(pSample);
}

// CTIFOutputPin

CTIFOutputPin::CTIFOutputPin(CBaseFilter* pFilter, CCritSec* pcsLock, HRESULT* phr)
	: CBaseOutputPin(_T("TIF Output"), pFilter, pcsLock, phr, L"~TIF Output")
{
}

HRESULT CTIFOutputPin::CheckMediaType(const CMediaType* pmt)
{
	return pmt->majortype == KSDATAFORMAT_TYPE_MPEG2_SECTIONS && pmt->subtype == KSDATAFORMAT_SUBTYPE_DVB_SI 
		? S_OK 
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CTIFOutputPin::GetMediaType(int iPosition, CMediaType* pmt)
{
	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->majortype = KSDATAFORMAT_TYPE_MPEG2_SECTIONS;
	pmt->subtype = KSDATAFORMAT_SUBTYPE_DVB_SI;

	return S_OK;
}

HRESULT CTIFOutputPin::GetBufferSize(ALLOCATOR_PROPERTIES* pProperties)
{
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = 4096;

	return NOERROR;
}

