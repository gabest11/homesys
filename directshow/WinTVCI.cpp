#include "stdafx.h"
#include "WinTVCI.h"
#include "DirectShow.h"

static bool m_inserted = false;

WinTVCI::WinTVCI(IBaseFilter** ppBF)
	: CUnknown(NAME("WinTVCI"), NULL)
	, m_dll(NULL)
	, WinTVCI_Init(NULL)
	, WinTVCI_Shutdown(NULL)
	, WinTVCI_SendPMT(NULL)
	, WinTVCI_SendAPDU(NULL)
	, WinTVCI_OpenMMI(NULL)
{
	m_mutex = CreateMutex(NULL, TRUE, L"HomesysWinTVCI");

	if(GetLastError() != ERROR_ALREADY_EXISTS)
	{
		m_dll = LoadLibrary(L"hcwWinTVCI.dll");

		if(m_dll == NULL)
		{
			m_dll = LoadLibrary(L"../hcwWinTVCI.dll");
		}

		if(m_dll != NULL)
		{
			printf("Loading hcwWinTVCI.dll\n");

			WinTVCI_Init = (WinTVCI_Init_Ptr)GetProcAddress(m_dll, "WinTVCI_Init");
			WinTVCI_Shutdown = (WinTVCI_Shutdown_Ptr)GetProcAddress(m_dll, "WinTVCI_Shutdown");
			WinTVCI_SendPMT = (WinTVCI_Send_Ptr)GetProcAddress(m_dll, "WinTVCI_SendPMT");
			WinTVCI_SendAPDU = (WinTVCI_Send_Ptr)GetProcAddress(m_dll, "WinTVCI_SendAPDU");
			WinTVCI_OpenMMI = (WinTVCI_OpenMMI_Ptr)GetProcAddress(m_dll, "WinTVCI_OpenMMI");
		}
	}
	else
	{
		wprintf(L"***\n");
	}

	if(WinTVCI_Init != NULL && WinTVCI_Shutdown != NULL)
	{
		Foreach(AM_KSCATEGORY_CAPTURE, [&] (IMoniker* m, LPCWSTR id, LPCWSTR name) -> HRESULT
		{
			HRESULT hr;

			if(_wcsicmp(name, L"WINTVCIUSBBDA SOURCE") == 0)
			{
				CComPtr<IBaseFilter> wintvci;
		
				hr = m->BindToObject(NULL, NULL, __uuidof(IBaseFilter), (void**)&wintvci);
		
				printf("WinTVCI bind %08x\n", hr);

				if(SUCCEEDED(hr))
				{			
					hr = WinTVCI_Init(wintvci, Status_Callback, CamInfo_Callback, APDU_Callback, CloseMMI_Callback);

					printf("WinTVCI init %08x\n", hr);

					if(SUCCEEDED(hr))
					{
						m_wintvci = wintvci;

						*ppBF = wintvci.Detach();

						return S_OK;
					}

					printf("WinTVCI shutdown 2\n");
			
					hr = WinTVCI_Shutdown(wintvci);
				}
			}

			return S_CONTINUE;
		});
	}
}

WinTVCI::~WinTVCI()
{
	if(m_wintvci != NULL && WinTVCI_Shutdown != NULL)
	{
		printf("WinTVCI shutdown\n");

		WinTVCI_Shutdown(m_wintvci);

		m_wintvci = NULL;
	}

	if(m_dll != NULL)
	{
		FreeLibrary(m_dll);

		m_dll = NULL;
	}

	if(m_mutex != NULL)
	{
		CloseHandle(m_mutex);
	}
}

STDMETHODIMP WinTVCI::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ICommonInterface)
		QI(IWinTVCI)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP WinTVCI::SendPMT(const BYTE* pmt, int len)
{
	if(m_wintvci == NULL || WinTVCI_SendPMT == NULL)
	{
		return E_UNEXPECTED;
	}

	if(!m_inserted)
	{
		return S_FALSE;
	}

	if(pmt == NULL || len == 0)
	{
		m_pmt.resize(0);

		return S_OK;
	}

	/*
	printf("WinTVCI::SendPMT %d\n", len);

	for(int i = 0; i < len; i += 16)
	{
		for(int j = i, k = std::min<int>(i + 16, len); j < k; j++)
		{
			printf("%02x ", pmt[j]);
		}

		printf("\n");
	}
	*/
	if(0) //if(m_pmt.size() == len && memcmp(m_pmt.data(), pmt, len) == 0)
	{
		printf("WinTVCI PMT dup\n");

		return S_OK;
	}

	m_pmt.resize(len);

	memcpy(m_pmt.data(), pmt, len);
	
	HRESULT hr = WinTVCI_SendPMT(m_wintvci, pmt, len);
	/**/
	printf("%08x <= WinTVCI_SendPMT\n", hr);
	
	return hr;
}

STDMETHODIMP WinTVCI::HasCard()
{
	return m_inserted ? S_OK : S_FALSE;
}

HRESULT WinTVCI::Status_Callback(IBaseFilter* pBF, int status)
{
	printf("WinTVCI::Status_Callback %d\n", status);

	switch(status)
	{
	case 1: m_inserted = true; break;
	case 2: m_inserted = false; break;
	default: break;
	}
	// 1 card removed?
	// 2 card inserted?

	return S_OK;
}

HRESULT WinTVCI::CamInfo_Callback(void* context, DWORD appType, DWORD appManuf, DWORD manufCode, LPCSTR info)
{
	printf("WinTVCI::CamInfo_Callback: appType %d, appManuf %d, manufCode %d, info %s\n", appType, appManuf, manufCode, info);

	return S_OK;
}

HRESULT WinTVCI::APDU_Callback(IBaseFilter* pBF, const BYTE* pAPDU, int size)
{
	return S_OK;
}

HRESULT WinTVCI::CloseMMI_Callback(IBaseFilter* pBF)
{
	return S_OK;
}

//

GeniaCI::GeniaCI()
	: CUnknown(NAME("GeniaCI"), NULL)
	, m_dll(NULL)
	, m_Initialize(NULL)
	, m_ExitCI(NULL)
{
	m_mutex = CreateMutex(NULL, TRUE, L"HomesysGeniaCI");

	if(GetLastError() != ERROR_ALREADY_EXISTS)
	{
		m_dll = LoadLibrary(L"Genia_CIStack.dll");

		if(m_dll == NULL)
		{
			m_dll = LoadLibrary(L"../Genia_CIStack.dll");
		}

		if(m_dll != NULL)
		{
			m_Initialize = (CI_Initialize)GetProcAddress(m_dll, "Genia_CI_Initialize");
			m_ExitCI = (CI_ExitCI)GetProcAddress(m_dll, "Genia_CI_ExitCI");
		}

		if(m_Initialize != NULL && m_ExitCI != NULL)
		{
			m_Initialize(CallBack, TimerSetCallBack, this);
		}
	}
	else
	{
		wprintf(L"***\n");
	}
}

GeniaCI::~GeniaCI()
{
	if(m_ExitCI != NULL)
	{
		printf("GeniaCI shutdown\n");

		//m_ExitCI();
	}

	if(m_dll != NULL)
	{
		FreeLibrary(m_dll);

		m_dll = NULL;
	}

	if(m_mutex != NULL)
	{
		CloseHandle(m_mutex);
	}
}

STDMETHODIMP GeniaCI::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return
		QI(ICommonInterface)
		QI(IGeniaCI)
		 __super::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP GeniaCI::SendPMT(const BYTE* pmt, int len)
{
	return E_NOTIMPL;
}

STDMETHODIMP GeniaCI::HasCard()
{
	return S_OK; // TODO
}

void GeniaCI::CallBack(WORD wType,void *pParams,WORD wLen)
{
}

void GeniaCI::TimerSetCallBack(UINT milliSeconds,PVOID context)
{
}
