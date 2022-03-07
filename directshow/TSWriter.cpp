#include "stdafx.h"
#include "TSWriter.h"
#include "DirectShow.h"
#include <initguid.h>
#include "moreuuids.h"

// CTSWriterFilter

CTSWriterFilter::CTSWriterFilter(LPUNKNOWN lpunk, HRESULT* phr)
	: CBaseFilter(NAME("CTSWriterFilter"), lpunk, this, __uuidof(this))
	, m_fp(NULL)
	, m_port(0)
{
	if(phr) *phr = S_OK;

	m_input = new CTSWriterInputPin(this, phr);

	CAMThread::Create();
}

CTSWriterFilter::~CTSWriterFilter()
{
	m_exit.Set();

	CAMThread::Close();

	delete m_input;

	Close();
}

void CTSWriterFilter::Close()
{
	CAutoLock csAutoLock(&m_csFile);

	if(m_fp != NULL) {fclose(m_fp); m_fp = NULL;}
}

STDMETHODIMP CTSWriterFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ITSWriterFilter)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

int CTSWriterFilter::GetPinCount()
{
	return 1;
}

CBasePin* CTSWriterFilter::GetPin(int n)
{
	if(n == 0) return m_input;
	else return NULL;
}

STDMETHODIMP CTSWriterFilter::Stop()
{
	Close();

	return __super::Stop();
}

HRESULT CTSWriterFilter::Receive(IMediaSample* pSample)
{
	CAutoLock csAutoLock(&m_csFile);

	int len = pSample->GetActualDataLength();

	if(len == 188)
	{
		BYTE* src = NULL;

		if(SUCCEEDED(pSample->GetPointer(&src)))
		{
			if(m_fp != NULL)
			{
				fwrite(src, len, 1, m_fp);
			}

			CAutoLock csAutoLock(&m_clients_lock);
			
			for(auto i = m_clients.begin(); i != m_clients.end(); i++)
			{
				BYTE* buff = new BYTE[len];

				memcpy(buff, src, len);

				(*i)->Queue(buff);
			}
		}
	}

	return S_OK;
}

DWORD CTSWriterFilter::ThreadProc()
{
	Util::Socket::Startup();

	Util::Socket server;
	 
	if(server.Listen(NULL, 0))
	{
		m_port = server.GetPort();
		
		while(!m_exit.Wait(1))
		{
			Util::Socket* client = new Util::Socket();

			if(server.Accept(*client))
			{
				CAutoLock csAutoLock(&m_clients_lock);

				m_clients.push_back(new CHTTPEndpoint(client));
			}
			else
			{
				delete client;
			}

			{
				CAutoLock csAutoLock(&m_clients_lock);

				auto i = m_clients.begin(); 
			
				while(i != m_clients.end())
				{
					auto j = i++;

					if(!(*j)->IsOpen())
					{
						delete *j;

						m_clients.erase(j);
					}
				}
			}
		}

		m_port = 0;
	}

	{
		CAutoLock csAutoLock(&m_clients_lock);

		for(auto i = m_clients.begin(); i != m_clients.end(); i++)
		{
			delete *i;
		}

		m_clients.clear();
	}

	Util::Socket::Cleanup();

	m_hThread = NULL;

	return 0;
}

// ITSWriterFilter

STDMETHODIMP CTSWriterFilter::SetOutput(LPCWSTR path)
{
	CAutoLock csAutoLock(&m_csFile);

	Close();

	if(path != NULL)
	{
		m_fp = _wfopen(path, L"wb");
	
		return m_fp != NULL ? S_OK : E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CTSWriterFilter::GetPort(WORD* port)
{
	CheckPointer(port, E_POINTER);

	*port = m_port;

	return S_OK;
}

// CTSWriterFilter::CTSWriterInputPin

CTSWriterFilter::CTSWriterInputPin::CTSWriterInputPin(CTSWriterFilter* pFilter, HRESULT* phr)
	: CBaseInputPin(_T("Input"), pFilter, pFilter, phr, L"Input")
{
}

HRESULT CTSWriterFilter::CTSWriterInputPin::CheckMediaType(const CMediaType* pmt)
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

STDMETHODIMP CTSWriterFilter::CTSWriterInputPin::Receive(IMediaSample* pSample)
{
	return ((CTSWriterFilter*)m_pFilter)->Receive(pSample);
}


// CTSWriterFilter::CHTTPEndpoint

CTSWriterFilter::CHTTPEndpoint::CHTTPEndpoint(Util::Socket* s)
	: m_socket(s)
	, m_queue(10000)
{
	CAMThread::Create();
}

CTSWriterFilter::CHTTPEndpoint::~CHTTPEndpoint()
{
	m_exit.Set();

	CAMThread::Close();

	while(m_queue.GetCount() > 0)
	{
		delete [] m_queue.Dequeue();
	}

	m_socket->Close();

	delete m_socket;
}

void CTSWriterFilter::CHTTPEndpoint::Queue(BYTE* packet)
{
	if(m_queue.GetCount() < m_queue.GetMaxCount())
	{
		m_queue.Enqueue(packet);
	}
	else
	{
		delete [] packet;
	}
}

DWORD CTSWriterFilter::CHTTPEndpoint::ThreadProc()
{
	Util::Socket::Startup();

	std::string s;
	std::list<std::string> sl;

	while(m_socket->Read(s))
	{
		s = Util::Trim(s);

		if(sl.empty() && s.find("GET / ") != 0) break;

		printf("HTTP: %s\n", s.c_str());

		sl.push_back(s);

		if(s.empty()) break;
	}

	if(!sl.empty())
	{
		m_socket->Write("HTTP/1.1 200 OK\r\n");
		m_socket->Write("Content-type: application/octect-stream\r\n");
		m_socket->Write("Connecton: close\r\n");
		m_socket->Write("\r\n");

		HANDLE handles[] = {m_exit, m_queue.GetEnqueueEvent()};

		while(m_hThread != NULL)
		{
			switch(::WaitForMultipleObjects(countof(handles), handles, FALSE, INFINITE))
			{
			default:
			case WAIT_OBJECT_0 + 0: 

				m_hThread = NULL;

				break;

			case WAIT_OBJECT_0 + 1:

				while(m_queue.GetCount() > 0 && m_socket->IsOpen())
				{
					BYTE* packet = m_queue.Dequeue();

					m_socket->Write(packet, 188);

					delete [] packet;
				}

				break;
			}
		}
	}
	else
	{
		m_socket->Write("HTTP/1.1 404 Not Found\r\n");
		m_socket->Write("Connecton: close\r\n");
		m_socket->Write("\r\n");
	}

	m_socket->Close();

	Util::Socket::Cleanup();

	m_hThread = NULL;

	return 0;
}
