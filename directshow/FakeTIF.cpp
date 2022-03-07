#include "stdafx.h"
#include "FakeTIF.h"
#include "DirectShow.h"

CFakeTIF::CFakeTIF(LPUNKNOWN lpunk, HRESULT* phr) 
	: CBaseFilter(NAME("CFakeTIF"), lpunk, this, __uuidof(this))
{
	m_pInput = new CFakeTIFInputPin(this, phr);
}

CFakeTIF::~CFakeTIF()
{
	delete m_pInput;
}

STDMETHODIMP CFakeTIF::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ITuneRequestInfo)
		QI(IPSITables)
		QI(IFrequencyMap)
		QI(ITIFUnknown1)
		QI(ITIFUnknown2)
		QI(IConnectionPointContainer)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// CFakeTIFInputPin

CFakeTIF::CFakeTIFInputPin::CFakeTIFInputPin(CFakeTIF* pFilter, HRESULT* phr)
	: CBaseInputPin(_T("Input"), pFilter, pFilter, phr, L"Input")
	, m_context(0)
{
}

CFakeTIF::CFakeTIFInputPin::~CFakeTIFInputPin()
{
	Unregister();
}

STDMETHODIMP CFakeTIF::CFakeTIFInputPin::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

void CFakeTIF::CFakeTIFInputPin::Register()
{
	if(m_context == 0)
	{
		ForeachInterface<IBDA_TIF_REGISTRATION>(m_pFilter->GetFilterGraph(), [&] (IBaseFilter* pBF, IBDA_TIF_REGISTRATION* pTIFReg) -> HRESULT
		{
			m_tifreg = pTIFReg;

			return S_OK;
		});

		if(m_tifreg != NULL)
		{
			m_tifreg->RegisterTIFEx(this, &m_context, &m_mpeg2data);
		}
	}
}

void CFakeTIF::CFakeTIFInputPin::Unregister()
{
	if(m_context != 0)
	{
		if(m_tifreg != NULL)
		{
			m_tifreg->UnregisterTIF(m_context);
			m_tifreg = NULL;
		}

		m_context = 0;
		m_mpeg2data = NULL;
	}
}

HRESULT CFakeTIF::CFakeTIFInputPin::CompleteConnect(IPin* pin)
{
	Register();

	return __super::CompleteConnect(pin);
}

HRESULT CFakeTIF::CFakeTIFInputPin::BreakConnect()
{
	Unregister();

	return __super::BreakConnect();
}
