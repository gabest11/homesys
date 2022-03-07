#pragma once

#include "../3rdparty/baseclasses/baseclasses.h"
#include <bdatif.h>

[uuid("ABBA0018-3075-11D6-88A4-00B0D0200F88")]
interface ITIFUnknown1 : public IUnknown
{
	STDMETHOD(f10)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f11)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f12)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f13)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f14)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f15)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f16)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f17)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f18)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f19)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
};

[uuid("D749E960-0C9F-11D3-8FF2-00A0C9224CF4")]
interface ITIFUnknown2 : public IUnknown
{
	STDMETHOD(f20)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f21)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f22)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f23)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(CompleteChannelChange)(UINT_PTR p0) PURE;
	STDMETHOD(f25)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f26)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f27)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f28)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
	STDMETHOD(f29)(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) PURE;
};

[uuid("FC772AB0-0C7F-11D3-8FF2-00A0C9224CF4")]
class CFakeTIF
	: public CBaseFilter
	, public CCritSec
	, public ITuneRequestInfo
	, public IPSITables
	, public IFrequencyMap
	, public ITIFUnknown1
	, public ITIFUnknown2
	, public IConnectionPointContainer
{
	class CConnectionPoint
		: public CUnknown
		, public IConnectionPoint
	{
		CComPtr<IUnknown> m_pUnk;

	public:
		CConnectionPoint() : CUnknown(L"CConnectionPoint", NULL) {}

		DECLARE_IUNKNOWN	
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
		{
			return 
				QI(IConnectionPoint)
				__super::NonDelegatingQueryInterface(riid, ppv);
		}

		STDMETHODIMP GetConnectionInterface(IID* pIID) { return E_NOTIMPL; }
        STDMETHODIMP GetConnectionPointContainer(IConnectionPointContainer **ppCPC) { return E_NOTIMPL; }
        STDMETHODIMP Advise(IUnknown *pUnkSink, DWORD *pdwCookie) { *pdwCookie = 0; m_pUnk = pUnkSink; return S_OK; }
		STDMETHODIMP Unadvise(DWORD dwCookie) { m_pUnk = NULL; return dwCookie == 0 ? S_OK : E_INVALIDARG; }
        STDMETHODIMP EnumConnections(IEnumConnections **ppEnum) { return E_NOTIMPL; }

	};

	class CFakeTIFInputPin 
		: public CBaseInputPin
	{
		CComPtr<IBDA_TIF_REGISTRATION> m_tifreg;
		ULONG m_context;
		CComPtr<IUnknown> m_mpeg2data;

		void Register();
		void Unregister();

	public:
		CFakeTIFInputPin(CFakeTIF* pFilter, HRESULT* phr);
		virtual ~CFakeTIFInputPin();

		HRESULT CheckMediaType(const CMediaType* pmt) {return S_OK;}
		HRESULT CompleteConnect(IPin* pin);
		HRESULT BreakConnect();

		DECLARE_IUNKNOWN	
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
	};

	CFakeTIFInputPin* m_pInput;

public:
	CFakeTIF(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CFakeTIF();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount() {return 1;}
	CBasePin* GetPin(int n) {return n == 0 ? m_pInput : NULL;}

	// ITuneRequestInfo

    STDMETHODIMP GetLocatorData(ITuneRequest *Request) {return S_OK;}
    STDMETHODIMP GetComponentData(ITuneRequest *CurrentRequest) { return E_NOTIMPL; }
    STDMETHODIMP CreateComponentList(ITuneRequest *CurrentRequest) { return E_NOTIMPL; }
    STDMETHODIMP GetNextProgram(ITuneRequest *CurrentRequest, ITuneRequest** TuneRequest) { return E_NOTIMPL; }
    STDMETHODIMP GetPreviousProgram(ITuneRequest *CurrentRequest, ITuneRequest **TuneRequest) { return E_NOTIMPL; }
    STDMETHODIMP GetNextLocator(ITuneRequest *CurrentRequest, ITuneRequest **TuneRequest) { return E_NOTIMPL; }
    STDMETHODIMP GetPreviousLocator(ITuneRequest *CurrentRequest, ITuneRequest **TuneRequest) { return E_NOTIMPL; }

	// IPSITables

	STDMETHODIMP GetTable(DWORD dwTSID, DWORD dwTID_PID, DWORD dwHashedVer, DWORD dwPara4, IUnknown** ppIUnknown) { return E_NOTIMPL; }

	// IFrequencyMap

    STDMETHODIMP get_FrequencyMapping(ULONG *ulCount, ULONG **ppulList) { return E_NOTIMPL; }
    STDMETHODIMP put_FrequencyMapping(ULONG ulCount, ULONG pList[]) { return E_NOTIMPL; }
    STDMETHODIMP get_CountryCode(ULONG *pulCountryCode) { return E_NOTIMPL; }
    STDMETHODIMP put_CountryCode(ULONG ulCountryCode) { return E_NOTIMPL; }
    STDMETHODIMP get_DefaultFrequencyMapping( ULONG ulCountryCode, ULONG *pulCount, ULONG **ppulList) { return E_NOTIMPL; }
    STDMETHODIMP get_CountryCodeList(ULONG *pulCount, ULONG **ppulList) { return E_NOTIMPL; }

	// ITIFUnknown1

	STDMETHODIMP f10(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f11(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f12(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f13(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f14(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f15(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f16(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f17(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f18(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f19(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }

	// ITIFUnknown2

	STDMETHODIMP f20(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f21(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f22(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f23(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP CompleteChannelChange(UINT_PTR p0) { return S_OK; }
	STDMETHODIMP f25(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f26(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f27(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f28(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }
	STDMETHODIMP f29(UINT_PTR p0, UINT_PTR p1, UINT_PTR p2, UINT_PTR p3) { return E_NOTIMPL; }

	// IConnectionPointContainer

    STDMETHODIMP EnumConnectionPoints(IEnumConnectionPoints ** ppEnum) { return E_NOTIMPL; }
    STDMETHODIMP FindConnectionPoint(REFIID riid, IConnectionPoint ** ppCP) { (*ppCP = new CConnectionPoint())->AddRef(); return S_OK; }
};
/*
class CNetworkProvider 
	: public CFGAggregator
	, public IBDA_TIF_REGISTRATION
{
	CComPtr<IBaseFilter> m_pTIF;

public:
	CNetworkProvider(const CLSID& clsid, IBaseFilter* pTIF, HRESULT& hr)
		: CFGAggregator(clsid, L"", NULL, hr)
	{
		m_pTIF = pTIF;

		hr = S_OK;
	}

    DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
	    CheckPointer(ppv, E_POINTER);

		return
			QI(IBDA_TIF_REGISTRATION)
			__super::NonDelegatingQueryInterface(riid, ppv);
	}

	// IBDA_TIF_REGISTRATION

	STDMETHODIMP RegisterTIFEx(IPin* pTIFInputPin, ULONG* ppvRegistrationContext, IUnknown** ppMpeg2DataControl)
	{
		CheckPointer(m_inner, E_UNEXPECTED);

		HRESULT hr;

		// hr = CComQIPtr<IBDA_TIF_REGISTRATION>(m_inner)->RegisterTIFEx(pTIFInputPin, ppvRegistrationContext, ppMpeg2DataControl);

		((CFakeTIF*)m_pTIF.p)->m_pRealTIF = DirectShow::GetFilter(pTIFInputPin);

		hr = CComQIPtr<IBDA_TIF_REGISTRATION>(m_inner)->RegisterTIFEx(DirectShow::GetFirstPin(m_pTIF), ppvRegistrationContext, ppMpeg2DataControl);

		return hr;
	}
	
	STDMETHODIMP UnregisterTIF(ULONG pvRegistrationContext)
	{
		CheckPointer(m_inner, E_UNEXPECTED);

		HRESULT hr;

		hr = CComQIPtr<IBDA_TIF_REGISTRATION>(m_inner)->UnregisterTIF(pvRegistrationContext);

		((CFakeTIF*)m_pTIF.p)->m_pRealTIF = NULL;

		return hr;
	};
};
*/