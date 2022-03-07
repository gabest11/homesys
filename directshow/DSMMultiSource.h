#pragma once

#include "DSMSink.h"

[uuid("5911DECE-5B64-49BC-B9C4-3828BDD69CC9")]
interface IDSMMultiSourceFilter  : public IUnknown
{
	STDMETHOD_(bool, IsClosed)() = 0;
};

[uuid("2FAA2599-757D-45EF-AB16-DE11AD8EF96C")]
class CDSMMultiSourceFilter 
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
	, public IAMOpenProgress
	, public IDSMMultiSourceFilter
{
	class CDSMMultiSourceOutputPin 
		: public CBaseOutputPin
		, protected CAMThread
	{
		int m_id;
		CThreadSafeQueue<CDSMPacket*> m_queue;
		REFERENCE_TIME m_queue_start, m_queue_stop;
		CAMEvent m_exit;
		bool m_flushing;

		DWORD ThreadProc();

		HRESULT Deliver(IMediaSample* pSample) {ASSERT(0); return E_UNEXPECTED;}

		void DeliverSample(CDSMPacket* item);

	public:
		CDSMMultiSourceOutputPin(CDSMMultiSourceFilter* pFilter, int id, const CMediaType& mt, HRESULT* phr, LPCWSTR name);

		DECLARE_IUNKNOWN
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

		HRESULT Active();
		HRESULT Inactive();

	    HRESULT CheckMediaType(const CMediaType* pmt);
		HRESULT GetMediaType(int iPosition, CMediaType* pmt);
		HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);

		HRESULT Deliver(CDSMPacket* item);
		HRESULT DeliverNewSegment(REFERENCE_TIME start, REFERENCE_TIME stop);
		HRESULT DeliverEndOfStream();
		HRESULT DeliverBeginFlush();
		HRESULT DeliverEndFlush();

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}

		int GetId() {return m_id;}
		REFERENCE_TIME GetQueueDuration() {return m_queue_stop - m_queue_start;}
		size_t GetQueueCount() {return m_queue.GetCount();}
		size_t GetQueueMaxCount() {return m_queue.GetMaxCount();}
		bool IsFull() {return m_queue.GetCount() == m_queue.GetMaxCount();}
	};

protected:
	std::wstring m_path;
	std::vector<CDSMMultiSourceOutputPin*> m_outputs;
	CDSMFileMapping m_mapping;
	CAMEvent m_exit;
	bool m_starvation;
	int m_progress;

	DWORD ThreadProc();

	CDSMFile m_file;
	int m_index;
	struct {REFERENCE_TIME start, newstart, offset, filter, pin;} m_time;

	void DeliverBeginFlush();
	void DeliverEndFlush();
	void DeliverNewSegment();
	void DeliverEndOfStream();

	bool Seek(REFERENCE_TIME rt);
	bool SeekNext();

	CCritSec m_csPinTime;

public:
	CDSMMultiSourceFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CDSMMultiSourceFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IMediaSeeking

	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

	// IAMOpenProgress

	STDMETHODIMP QueryProgress(LONGLONG* pllTotal, LONGLONG* pllCurrent);
	STDMETHODIMP AbortOperation();

	// IDSMMultiSourceFilter

	STDMETHODIMP_(bool) IsClosed();
};