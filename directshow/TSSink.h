#pragma once

#include "../3rdparty/baseclasses/baseclasses.h"

[uuid("AB7C5ADE-A392-4128-A518-DCB1A6347FAE")]
interface ITSSourceFilter : public IUnknown
{
	STDMETHOD(NewSegment)(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) = 0;
	STDMETHOD(Receive)(const BYTE* buff, int len) = 0;
	STDMETHOD(SaveInput)() = 0;
};

[uuid("7AF6CFD0-C68F-49EF-A8D8-8D86428FF8F7")]
class CTSSourceFilter
	: public CBaseFilter
	, public CCritSec
	, public ITSSourceFilter
{
	class CTSSourceOutputPin : public CBaseOutputPin
	{
	public:
		CTSSourceOutputPin(CTSSourceFilter* pFilter, HRESULT* phr);

		HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
	    HRESULT CheckMediaType(const CMediaType* pmt);
		HRESULT GetMediaType(int iPosition, CMediaType* pmt);

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}
	};

	CBaseOutputPin* m_output;
	int m_save;
	std::list<std::vector<BYTE>*> m_data;

public:
	CTSSourceFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CTSSourceFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();

	// ITSSourceFilter

	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	STDMETHODIMP Receive(const BYTE* buff, int len);
	STDMETHODIMP SaveInput();
};

[uuid("5956D59A-669E-4AC5-A353-A383C56D73A6")]
interface ITSSinkFilter : public IUnknown
{
	STDMETHOD(SetSource)(ITSSourceFilter* pSource) = 0;
};

[uuid("1F959015-3AC1-4121-8EEC-83307D74657A")]
class CTSSinkFilter 
	: public CBaseFilter
	, public CCritSec
	, public ITSSinkFilter
{
	class CTSSinkInputPin : public CBaseInputPin
	{
	public:
		CTSSinkInputPin(CTSSinkFilter* pFilter, HRESULT* phr);

		HRESULT CheckMediaType(const CMediaType* pmt);

		STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
		STDMETHODIMP Receive(IMediaSample* pSample);
	};

	CBaseInputPin* m_input;
	CBaseOutputPin* m_output;

	CComPtr<ITSSourceFilter> m_source;

public:
	CTSSinkFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CTSSinkFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);

	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT Receive(IMediaSample* pSample);

	// ITSSinkFilter

	STDMETHODIMP SetSource(ITSSourceFilter* pSource);
};

class CTIFOutputPin : public CBaseOutputPin
{
public:
	CTIFOutputPin(CBaseFilter* pFilter, CCritSec* pcsLock, HRESULT* phr);

	HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);
	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);
};
