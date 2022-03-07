#pragma once

#include "BaseSplitterFile.h"
#include "SmartCard.h"
#include "CSA.h"
#include "WinTVCI.h"

class TSDemuxStream
{
public:
	int type, pid;
	CMediaType mt;
	bool scrambled, gotkey;
	int payloadstart;
	__int64 packet;
	std::map<int, WORD> ecms;
	CSA csa;
	// TODO: language

	TSDemuxStream()
	{
		type = pid = 0;
		scrambled = gotkey = false;
		payloadstart = 0;
		packet = 0;
	}

	bool DetectMediaType(CBaseSplitterFile& f, int len, BYTE pes_id);
};

class TSDemuxProgram
{
public:
	int type, pid, sid, pcr;
	bool scrambled, gotkey;
	int missing;
	__int64 packet;
	std::wstring name;
	std::wstring provider;
	std::map<int, TSDemuxStream*> streams;
	std::map<int, WORD> ecms;
	CSA csa;

	TSDemuxProgram()
	{
		type = pid = sid = pcr = 0;
		scrambled = gotkey = false;
		missing = -5;
		packet = 0;
	}

	size_t GetScrambledCount() const;

	WORD GetSystemId(int ecm_pid) const;
};

class TSDemuxPayload
{
public:
	std::vector<BYTE> m_buff;
	int m_size;
	int m_pid;
	__int64 m_packet;
	std::vector<BYTE> m_scrambled;
	int m_scrambled_size;

	TSDemuxPayload(int pid) : m_pid(pid), m_size(0), m_packet(0), m_scrambled_size(0)
	{
	}

	void Append(const BYTE* buff, int len);
};

class TSDemuxQueueItem
{
public:
	enum {Sample, NewSegment, EndOfStream} type;
	std::vector<BYTE> buff;
	REFERENCE_TIME pts;
	bool discontinuity;

	TSDemuxQueueItem() : pts(_I64_MIN), discontinuity(false)
	{
	}
};

[uuid("139BBFA5-BEC6-4FCC-B72D-58615033817F")]
interface ITSDemuxFilter : public IUnknown
{
	enum TSWriterOutputType {WriteNone, WriteProgram, WriteFullTS};

	STDMETHOD(SetFreq)(int freq) = 0;
	STDMETHOD(ResetPrograms)() = 0;
	STDMETHOD(GetPrograms)(std::list<TSDemuxProgram*>& programs, int sid, bool sdt, bool wait = true) = 0;
	STDMETHOD(SetProgram)(int sid, bool dummy) = 0;
	STDMETHOD(HasProgram)(int sid, int freq) = 0;
	STDMETHOD(IsScrambled)(int sid) = 0;
	STDMETHOD(SetWriterOutput)(TSWriterOutputType type) = 0;
	STDMETHOD_(__int64, GetReceivedBytes)() = 0;
	STDMETHOD_(float, GetReceivedErrorRate)() = 0;
	STDMETHOD(SetKsPropertySet)(IKsPropertySet* ksps) = 0;
	STDMETHOD(SetSmartCardServer)(ISmartCardServer* scs) = 0;
	STDMETHOD(SetCommonInterface)(ICommonInterface* ci) = 0;
};

[uuid("F301AA16-48D0-4912-B713-3A82EDA49135")]
class CTSDemuxFilter 
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IMediaSeeking
	, public ITSDemuxFilter 
	, public ISmartCardClient
{
	class CTSDemuxInputPin : public CBaseInputPin, protected CAMThread
	{
		CThreadSafeQueue<std::vector<BYTE>*> m_queue;
		CAMEvent m_exit;

		DWORD ThreadProc();

	public:
		CTSDemuxInputPin(CTSDemuxFilter* pFilter, HRESULT* phr);

		HRESULT Active();
		HRESULT Inactive();

		HRESULT CheckMediaType(const CMediaType* pmt);
		HRESULT CheckConnect(IPin* pPin);
		HRESULT CompleteConnect(IPin* pPin);
		HRESULT BreakConnect();

		STDMETHODIMP Receive(IMediaSample* pSample);
	};

	class CTSDemuxOutputPin 
		: public CBaseOutputPin
		, protected CAMThread
	{
		struct {CMediaType mt; int type, pid;} m_stream;
		CThreadSafeQueue<TSDemuxQueueItem*> m_queue;
		CAMEvent m_exit;
		bool m_flushing;
		std::vector<BYTE> m_buff;
		int m_size;
		REFERENCE_TIME m_pts;
		bool m_discontinuity;

		DWORD ThreadProc();

		HRESULT Deliver(IMediaSample* pSample) {ASSERT(0); return E_UNEXPECTED;}

		void DeliverSample(TSDemuxQueueItem* item);
		bool NextFrame(CBaseSplitterFile& f, int& len, REFERENCE_TIME& dur);

	public:
		CTSDemuxOutputPin(CTSDemuxFilter* pFilter, const TSDemuxStream& stream, HRESULT* phr, LPCWSTR name = L"Output");

		HRESULT Active();
		HRESULT Inactive();

		HRESULT Deliver(TSDemuxQueueItem* item);
		HRESULT DeliverNewSegment(REFERENCE_TIME pts);
		HRESULT DeliverEndOfStream();
		HRESULT DeliverBeginFlush();
		HRESULT DeliverEndFlush();

	    HRESULT CheckMediaType(const CMediaType* pmt);
		HRESULT GetMediaType(int iPosition, CMediaType* pmt);
		HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}

		int GetPID() const {return m_stream.pid;}
	};

	class CTSDemuxWriterOutputPin 
		: public CBaseOutputPin
	{
	public:
		CTSDemuxWriterOutputPin(CTSDemuxFilter* pFilter, HRESULT* phr, LPCWSTR name = L"Output");

		HRESULT CheckConnect(IPin* pPin);
	    HRESULT CheckMediaType(const CMediaType* pmt);
		HRESULT GetMediaType(int iPosition, CMediaType* pmt);
		HRESULT GetBufferSize(ALLOCATOR_PROPERTIES* pProperties);

		STDMETHODIMP Notify(IBaseFilter* pSender, Quality q) {return S_OK;}
	};

protected:
	CTSDemuxInputPin* m_input;
	std::vector<CTSDemuxOutputPin*> m_outputs;
	CTSDemuxWriterOutputPin* m_writer;
	int m_freq;
	int m_sid, m_tmpsid;
	struct {REFERENCE_TIME start, pcr, pts, offset, first, last, newstart;} m_time;
	bool m_seekbybitrate;
	TSWriterOutputType m_record;
	bool m_streaming;
	std::vector<BYTE> m_buff;
	int m_size;
	__int64 m_received;
	float m_received_error;
	CComPtr<IAsyncReader> m_reader;
	CComPtr<IKsPropertySet> m_ksps;
	CComPtr<ISmartCardServer> m_scs;
	CComPtr<ICommonInterface> m_ci;
	DWORD m_scs_cookie;
	CCritSec m_csReceive;
	CCritSec m_csPrograms;
	CAMEvent m_endflush;
	struct {TSDemuxStream video, audio;} m_dummy;
	bool m_firesat_pmt_sent;
	int m_ci_pmt_sent;

	bool WritePID(int pid);
	int GetFreq();

	enum {CMD_EXIT, CMD_SEEK};

	DWORD ThreadProc();

	__int64 Seek();

	void DeliverNewSegment(REFERENCE_TIME pts);
	void DeliverEndOfStream();
	void DeliverBeginFlush();
	void DeliverEndFlush();

	// TODO: wrap these into a CBaseSplitterFileEx derived class

	CAMEvent m_pat;
	CAMEvent m_sdt;
	int m_sdt_count;
	std::map<int, WORD> m_emms;
	std::map<int, TSDemuxProgram*> m_programs;
	std::map<int, TSDemuxStream*> m_streams;
	std::map<__int64, CMediaType> m_cached_streams;
	std::map<int, TSDemuxPayload*> m_payload;
	struct {__int64 total, pat, payload;} m_packet;
	std::map<DWORD, std::vector<BYTE>*> m_cache;

	void ResetBuffer();

	void OnReceive(const BYTE* buff, int len);
	void OnPayload(const BYTE* buff, TSDemuxPayload* payload, const CBaseSplitterFile::TSHeader& h, bool decrypt);
	void OnPAT(CBaseSplitterFile& f);
	void OnCAT(CBaseSplitterFile& f);
	void OnEMM(CBaseSplitterFile& f, int pid);
	void OnECM(CBaseSplitterFile& f, int pid);
	void OnPMT(CBaseSplitterFile& f, TSDemuxProgram* p);
	void OnNIT(CBaseSplitterFile& f);
	void OnSDT(CBaseSplitterFile& f);
	void OnEIT(CBaseSplitterFile& f);
	void OnVCT(CBaseSplitterFile& f);
	void OnPES(CBaseSplitterFile& f, TSDemuxStream* s, bool complete, int pid);
	void OnPCR(int pid, REFERENCE_TIME pcr);
	void OnSmartCardPacket(int type, std::vector<BYTE>* buff, WORD pid, WORD system_id, bool required);

	int CheckSection(CBaseSplitterFile& f, __int64& end);

	bool IsProgramReady(int sid);
	bool IsProgramDecryptable(int sid);
	bool IsTypeReady(int sid);

public:
	CTSDemuxFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CTSDemuxFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	STDMETHODIMP Pause();
	STDMETHODIMP Stop();

	HRESULT CheckInput(const CMediaType* pmt);
	HRESULT CheckInput(IPin* pPin);
	HRESULT CompleteInput(IPin* pPin);
	HRESULT BreakInput();

	HRESULT Receive(const BYTE* p, int len);

	int GetPinCount();
	CBasePin* GetPin(int n);

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

	// ITSDemuxFilter

	STDMETHODIMP SetFreq(int freq);
	STDMETHODIMP ResetPrograms();
	STDMETHODIMP GetPrograms(std::list<TSDemuxProgram*>& programs, int sid, bool sdt, bool wait = true);
	STDMETHODIMP SetProgram(int sid, bool dummy);
	STDMETHODIMP HasProgram(int sid, int freq);
	STDMETHODIMP IsScrambled(int sid);
	STDMETHODIMP SetWriterOutput(TSWriterOutputType type);
	STDMETHODIMP_(__int64) GetReceivedBytes();
	STDMETHODIMP_(float) GetReceivedErrorRate();
	STDMETHODIMP SetKsPropertySet(IKsPropertySet* ksps);
	STDMETHODIMP SetSmartCardServer(ISmartCardServer* scs);
	STDMETHODIMP SetCommonInterface(ICommonInterface* ci);

	// ISmartCardClient

	STDMETHODIMP OnDCW(WORD pid, const BYTE* cw);
};
