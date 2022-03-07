#pragma once

#include "BaseSplitterFile.h"
#include "AsyncReader.h"
#include "ThreadSafeQueue.h"
#include "Silverlight.h"
#include "../util/Socket.h"

[uuid("91BA8D0C-7931-4D7B-AE4A-D4D8BBA068BA")]
class CSilverlightSourceFilter 
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
{
	class CSilverlightSourceOutputPin 
		: public CBaseOutputPin
		, protected CAMThread
	{
		struct SubSample {int clear, encrypted;};
		struct Sample {REFERENCE_TIME start, duration; UINT64 pos; DWORD size; bool syncpoint; BYTE iv[16]; std::vector<SubSample> subsamples;};
		struct QueueItem {Sample sample; std::vector<BYTE> buff;};
		CThreadSafeQueue<QueueItem*> m_queue;
		CAMEvent m_exit;
		bool m_flushing;
		std::string m_url;
		Silverlight::StreamIndex* m_stream;
		Silverlight::PlayReady m_playready;
		CBaseSplitterFile* m_file;
		std::list<REFERENCE_TIME> m_chunks;
		std::list<Sample> m_samples;
		UINT64 m_mdat_pos;
		REFERENCE_TIME m_start;
		std::vector<BYTE> m_h264prefix;

		void ParseMOOF(REFERENCE_TIME start);

		DWORD ThreadProc();

		HRESULT Deliver(IMediaSample* pSample) {ASSERT(0); return E_UNEXPECTED;}

	public:
		CSilverlightSourceOutputPin(CSilverlightSourceFilter* pFilter, const std::string& url, Silverlight::StreamIndex* stream, HRESULT* phr);
		virtual ~CSilverlightSourceOutputPin();

		HRESULT Active();
		HRESULT Inactive();

		HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, float dRate);
		HRESULT Deliver(QueueItem* item);
		HRESULT DeliverBeginFlush();
		HRESULT DeliverEndFlush();

	    HRESULT CheckMediaType(const CMediaType* pmt);
		HRESULT GetMediaType(int iPosition, CMediaType* pmt);
		HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}

		bool GetNextSampleStart(REFERENCE_TIME& next);
		void QueueNextSample();
	};

protected:
	std::wstring m_url;
	Silverlight::SmoothStreamingMedia m_media;
	std::vector<CSilverlightSourceOutputPin*> m_outputs;
	REFERENCE_TIME m_current;

	enum {CMD_EXIT, CMD_FLUSH, CMD_START};

	DWORD ThreadProc();

	HRESULT Load(Util::Socket& s, LPCSTR url);

public:
	CSilverlightSourceFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CSilverlightSourceFilter();

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
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
};
