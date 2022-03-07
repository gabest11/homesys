#pragma once

#include "BaseSplitterFile.h"
#include "AsyncReader.h"
#include "ThreadSafeQueue.h"
#include "Strobe.h"
#include "../util/Socket.h"

[uuid("E3072BEC-053D-4159-BA46-4AD831935A04")]
class CStrobeSourceFilter 
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IFileSourceFilter
	, public IMediaSeeking
{
	struct Chunk
	{
		DWORD segment;
		DWORD fragment;
		REFERENCE_TIME start;
		REFERENCE_TIME duration;
	};

	struct Sample 
	{
		DWORD id;
		REFERENCE_TIME start, duration; 
		UINT64 pos; 
		DWORD size; 
		bool syncpoint;
	};

	struct QueueItem 
	{
		Sample sample; 
		std::vector<BYTE> buff;
	};

	class CStrobeSourceOutputPin 
		: public CBaseOutputPin
		, protected CAMThread
	{
		CThreadSafeQueue<QueueItem*> m_queue;
		CAMEvent m_exit;
		bool m_flushing;
		REFERENCE_TIME m_start;
		bool m_h264;
		std::list<QueueItem*> m_h264queue;

		void FlushH264();

		DWORD ThreadProc();

		HRESULT Deliver(IMediaSample* pSample) {ASSERT(0); return E_UNEXPECTED;}

	public:
		CStrobeSourceOutputPin(const CMediaType& mt, CStrobeSourceFilter* pFilter, HRESULT* phr);
		virtual ~CStrobeSourceOutputPin();

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
	};

	struct AFRA
	{
		struct Entry
		{
			UINT64 Time;
			UINT64 Offset;
		};

		struct GlobalEntry
		{
			UINT64 Time;
			DWORD Segment;
			DWORD Fragment;
			UINT64 AfraOffset;
			UINT64 OffsetFromAfra;
		};

		DWORD TimeScale;
		
		std::vector<Entry> Entries;
		std::vector<GlobalEntry> GlobalEntries;
	};

	struct ABST
	{
		struct Segment
		{
			struct Entry
			{
				DWORD FirstSegment;
				DWORD FragmentsPerSegment;
			};

			std::vector<std::string> QualitySegmentUrlModifiers;
			std::vector<Entry> Entries;
		};

		struct Fragment
		{
			struct Entry
			{
				DWORD FirstFragment;
				UINT64 FirstFragmentTimestamp;
				DWORD FragmentDuration;
				BYTE DiscontinuityIndicator;
			};

			DWORD TimeScale;

			std::vector<std::string> QualitySegmentUrlModifiers;
			std::vector<Entry> Entries;
		};

		DWORD BootstrapinfoVersion;

		BYTE Profile;
		BYTE Live;
		BYTE Update;

		DWORD TimeScale;

		UINT64 CurrentMediaTime;
		UINT64 SmpteTimeCodeOffset;

		std::string MovieIdentifier;
		std::vector<std::string> ServerBaseURL;
		std::vector<std::string> QualitySegmentUrlModifier;
		std::string DrmData;
		std::string MetaData;

		std::vector<Segment> Segments;
		std::vector<Fragment> Fragments;
	};

	static bool ParseAFRA(CBaseSplitterFile& f, AFRA& afra);
	static bool ParseABST(CBaseSplitterFile& f, ABST& abst);
	static bool ParseMOOF(CBaseSplitterFile& f, REFERENCE_TIME chunk_start, DWORD cur_track_id, std::list<Sample>& samples);

protected:

	std::wstring m_url;
	std::string m_baseurl;
	Strobe::Manifest m_manifest;
	ABST m_abst;
	std::vector<CStrobeSourceOutputPin*> m_outputs;
	REFERENCE_TIME m_start, m_current, m_duration;

	CBaseSplitterFile* m_file;
	std::list<Chunk> m_chunks;
	std::list<Chunk>::iterator m_current_chunk;
	std::list<Sample> m_samples;
	UINT64 m_mdat_pos;
	// UINT64 m_mdat_offset;
	DWORD m_track_id;

	enum {CMD_EXIT, CMD_FLUSH, CMD_START};

	DWORD ThreadProc();
	bool SeekToNextSample();
	bool QueueNextSample();

	HRESULT Load(Util::Socket& s, LPCSTR url, int bitrate = 0);

public:
	CStrobeSourceFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CStrobeSourceFilter();

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
