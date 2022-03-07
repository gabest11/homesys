#pragma once

#include "../3rdparty/baseclasses/baseclasses.h"
#include "ThreadSafeQueue.h"
#include "../util/Socket.h"

[uuid("CB4F3246-BF05-4198-A93C-E39514844312")]
interface ITSWriterFilter : public IUnknown
{
	STDMETHOD(SetOutput)(LPCWSTR path) = 0;
	STDMETHOD(GetPort)(WORD* port) = 0;
};

[uuid("2ABF16BE-FC05-4EDB-9AB3-907B4F30551E")]
class CTSWriterFilter
	: public CBaseFilter
	, public CCritSec
	, public CAMThread
	, public ITSWriterFilter
{
	class CTSWriterInputPin : public CBaseInputPin
	{
	public:
		CTSWriterInputPin(CTSWriterFilter* pFilter, HRESULT* phr);

		HRESULT CheckMediaType(const CMediaType* pmt);

		STDMETHODIMP Receive(IMediaSample* pSample);
	};

	class CHTTPEndpoint : public CAMThread
	{
		Util::Socket* m_socket;
		CThreadSafeQueue<BYTE*> m_queue;
		CAMEvent m_exit;

		DWORD ThreadProc();

	public:
		CHTTPEndpoint(Util::Socket* s);
		virtual ~CHTTPEndpoint();

		bool IsOpen() {return m_socket->IsOpen();}
		void Queue(BYTE* packet);
	};

	CTSWriterInputPin* m_input;
	CCritSec m_csFile;
	FILE* m_fp;
	CAMEvent m_exit;
	std::list<CHTTPEndpoint*> m_clients;
	CCritSec m_clients_lock;
	WORD m_port;

	DWORD ThreadProc();

	void Close();

public:
	CTSWriterFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CTSWriterFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);

	STDMETHODIMP Stop();

	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT Receive(IMediaSample* pSample);

	// ITSWriterFilter

	STDMETHODIMP SetOutput(LPCWSTR path);
	STDMETHODIMP GetPort(WORD* port);
};
