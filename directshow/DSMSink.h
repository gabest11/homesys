#pragma once

#include "../3rdparty/baseclasses/baseclasses.h"
#include "DSM.h"
#include "ThreadSafeQueue.h"

class CDSMPacket
{
public:
	enum {Sample, NewSegment, EndOfStream};
	enum {None, TimeInvalid = 1, SyncPoint = 2, Discontinuity = 4, Bogus = 8};
	
	BYTE id;
	int type;
	int flags;
	std::vector<BYTE> buff;
	REFERENCE_TIME start, stop, pts;

	CDSMPacket() : type(Sample), flags(None), id(0) {}
	virtual ~CDSMPacket() {}
};

class CDSMSinkFilter;

class CDSMSinkInputPin 
	: public CBaseInputPin
{
	CCritSec m_csReceive;
	CCritSec m_csPacket;
	REFERENCE_TIME m_current;
	CThreadSafeQueue<CDSMPacket*> m_queue;
	struct {CDSMPacket* in; CDSMPacket* out;} m_packet;
	BYTE m_id;

	void FreePackets();

public:
	CDSMSinkInputPin(LPCWSTR pName, CDSMSinkFilter* pFilter, HRESULT* phr);
	virtual ~CDSMSinkInputPin();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin* pReceivePin);

	HRESULT Active();
	HRESULT Inactive();

	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	STDMETHODIMP Receive(IMediaSample* pSample);
	STDMETHODIMP EndOfStream();

	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();

	bool HasPacket() {return m_queue.GetCount() > 0;}
	CDSMPacket* GetPacket(bool peek);

	int GetId() {return m_id;}
};

class CDSMFile
{
	std::wstring m_path;
	HANDLE m_file;
	UINT64 m_size;
	struct {int len, limit; BYTE* buff;} m_byte;
	struct {int len; UINT64 buff;} m_bit;
	enum {None, ModeRead, ModeWrite} m_mode;

	void Write(const void* data, int len);
	void Read(void* data, int len);
	void Flush();

public:
	CDSMFile();
	virtual ~CDSMFile();

	bool Create(const std::wstring& path, const std::vector<CDSMSinkInputPin*>& pins);
	void Write(CDSMPacket* p);
	void Finalize();

	bool Open(const std::wstring& path);
	void Close();

	void WritePacketHeader(dsmp_t type, UINT64 len);
	bool ReadPacketHeader(dsmp_t& type, UINT64& len);

	void ByteWrite(const void* data, int len);
	void BitWrite(UINT64 data, int len);
	void StrWrite(LPCSTR data, bool bFixNewLine);
	UINT64 BitRead(int len);
	void ByteRead(void* data, int len); 
	void BitFlush();

	UINT64 GetPos();
	UINT64 GetLength();

	UINT64 GetWritePos();
	void SetReadPos(UINT64 pos);
};

#pragma pack(push, 1)

struct CDSMSharedData
{
	DWORD sync;
	DWORD streams;
	int first;
	int next;
	REFERENCE_TIME piece;
	REFERENCE_TIME start;
	REFERENCE_TIME stop;
	UINT64 size;
	DWORD index;
};

struct CDSMIndexData
{
	REFERENCE_TIME start;
	DWORD piece;
	DWORD offset;
};

#pragma pack(pop)

class CDSMFileMapping
{
	std::wstring m_name;
	bool m_readonly;
	ATL::CMutex m_mutex;
	HANDLE m_file;
	HANDLE m_mapping;
	CDSMSharedData* m_data;
	struct {HANDLE file; DWORD piece; REFERENCE_TIME stop; BYTE id;} m_index;

	SECURITY_DESCRIPTOR m_sd;
	SECURITY_ATTRIBUTES m_sa;
	void* m_acl;

	static ACL* BuildRestrictedSD(SECURITY_DESCRIPTOR* sd);

public:
	CDSMFileMapping();
	virtual ~CDSMFileMapping();

	bool Create(const std::wstring& path, const std::vector<CDSMSinkInputPin*>& pins);
	bool Open(const std::wstring& path, std::unordered_map<int, CMediaType>& mts);
	void Close();
	void Reset();

	void DeleteOld(int keep);

	bool WriteIndex(BYTE id, const CDSMIndexData& data);
	bool ReadIndex(REFERENCE_TIME rt, CDSMIndexData& data);

	CDSMSharedData* GetData() {return m_data;}
	std::wstring GetName(int n = -1);
	ATL::CMutex& GetMutex() {return m_mutex;}
};

[uuid("7F2B840F-94C6-4B9A-9427-5785D7833A13")]
interface IDSMSinkFilter : public IUnknown
{
	STDMETHOD(StartRecording)(LPCWSTR path) = 0;
	STDMETHOD(StopRecording)() = 0;
};

[uuid("F4C01294-2614-4CF1-AB07-F09BA64D5F1C")]
class CDSMSinkFilter
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public IAMFilterMiscFlags
	, public IFileSinkFilter
	, public IMediaSeeking
	, public IDSMSinkFilter
{
	bool m_streaming;
	std::vector<CDSMSinkInputPin*> m_pInputs;
	std::list<CDSMSinkInputPin*> m_pActivePins;
	CDSMFileMapping m_mapping;
	CDSMFile m_file[2];
	std::wstring m_filename;
	CCritSec m_csWrite;

	DWORD ThreadProc();

	CDSMPacket* GetPacket();
	void WritePacket(CDSMPacket* p);
	void Finalize();

public:
	CDSMSinkFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CDSMSinkFilter();

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);
	
	void AddInput();

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();

	// IAMFilterMiscFlags

	STDMETHODIMP_(ULONG) GetMiscFlags() {return AM_FILTER_MISC_FLAGS_IS_RENDERER;}

	// IFileSinkFilter

	STDMETHODIMP SetFileName(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
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

	// IDSMSinkFilter

	STDMETHODIMP StartRecording(LPCWSTR path);
	STDMETHODIMP StopRecording();
};