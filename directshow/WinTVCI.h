#pragma once

[uuid("7401A524-D56B-4315-84E7-D84016A70277")]
interface ICommonInterface : public IUnknown
{
	STDMETHOD (SendPMT) (const BYTE* pmt, int len) = 0;
	STDMETHOD (HasCard) () = 0;
};

[uuid("7401A524-D56B-4315-84E7-D84016A70277")]
interface IWinTVCI : public ICommonInterface
{
};

 class WinTVCI : public CUnknown, public IWinTVCI
{
	CComPtr<IBaseFilter> m_wintvci;
	HANDLE m_mutex;
	HMODULE m_dll;

	typedef HRESULT (__cdecl * Status_Callback_Ptr)(IBaseFilter* pBF, int status);
	typedef HRESULT (__cdecl * CamInfo_Callback_Ptr)(void* context, DWORD appType, DWORD appManuf, DWORD manufCode, LPCSTR info);
	typedef HRESULT (__cdecl * APDU_Callback_Ptr)(IBaseFilter* pBF, const BYTE* pAPDU, int size);
	typedef HRESULT (__cdecl * CloseMMI_Callback_Ptr)(IBaseFilter* pBF);

	typedef HRESULT (__cdecl * WinTVCI_Init_Ptr)(IBaseFilter* pBF, Status_Callback_Ptr onStatus, CamInfo_Callback_Ptr onCamInfo, APDU_Callback_Ptr onAPDU, CloseMMI_Callback_Ptr onCloseMMI);
	typedef HRESULT (__cdecl * WinTVCI_Shutdown_Ptr)(IBaseFilter* pBF);
	typedef HRESULT (__cdecl * WinTVCI_Send_Ptr)(IBaseFilter* pBF, const BYTE* data, int len);
	typedef HRESULT (__cdecl * WinTVCI_OpenMMI_Ptr)(IBaseFilter* pBF);

	WinTVCI_Init_Ptr WinTVCI_Init;
	WinTVCI_Shutdown_Ptr WinTVCI_Shutdown;
	WinTVCI_Send_Ptr WinTVCI_SendPMT;
	WinTVCI_Send_Ptr WinTVCI_SendAPDU;
	WinTVCI_OpenMMI_Ptr WinTVCI_OpenMMI;

	static HRESULT Status_Callback(IBaseFilter* pBF, int status);
	static HRESULT CamInfo_Callback(void* context, DWORD appType, DWORD appManuf, DWORD manufCode, LPCSTR info);
	static HRESULT APDU_Callback(IBaseFilter* pBF, const BYTE* pAPDU, int size);
	static HRESULT CloseMMI_Callback(IBaseFilter* pBF);

	std::vector<BYTE> m_pmt;

public:
	WinTVCI(IBaseFilter** ppBF);
	virtual ~WinTVCI();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ICommonInterface

	STDMETHODIMP SendPMT(const BYTE* pmt, int len);
	STDMETHODIMP HasCard();
};
 
[uuid("E0D9D561-5BDB-48A9-A8AC-B5B4B3A3D149")]
interface IGeniaCI : public ICommonInterface
{
};

class GeniaCI : public CUnknown, public IGeniaCI
{
	HANDLE m_mutex;
	HMODULE m_dll;

	#define MAXMODULEINFOSTRLEN  20

	struct MODULEINFO
	{            
		unsigned char  bModuleOnNow;  //0=removed, 1=OnNow
		char  strVersion[MAXMODULEINFOSTRLEN];
		char  strProvider[MAXMODULEINFOSTRLEN];
	};

	struct SLOTS_STATUS
	{
		MODULEINFO Module[2];
	};

	enum
	{
		SLOT_ST = 0,
		PINCODE,
		NORMAL_INPUT,
		CIMENU_LIST,
		CIMENU_RESPONSE,
		CIMENU_EXIT,
		READYCARDNO,
	
		FIGERPRINT,
		E_MAIL,
		FORCE_MESSAGE,
		CITEXT_INPUT,
		MODULE_DETECT,
		MODULE_INIT,
	};

	typedef void (*pFTACallBack)(WORD wType,void *pParams,WORD wLen);
	typedef void (*pFTATimerSetCallBack)(UINT milliSeconds,PVOID context);
	
	typedef int (cdecl * CI_Initialize)(void *pFTACB,void *pFTATimerCB,PVOID context);
	typedef int (cdecl * CI_GetSlotStatus)(SLOTS_STATUS *pSlotSt);
	typedef int (cdecl * CI_EnterMainMenu)(BYTE bSlotIndex);
	typedef int (cdecl * CI_EnterSubMenu)(BYTE bMenuItem);
	typedef int (cdecl * CI_ExitMenu)(void);
	typedef int (cdecl * CI_SetPinCode)(BYTE *pbCode,BYTE bCodeLen );
	typedef int (cdecl * CI_SetNormalInput)(BYTE *pbText,BYTE bTextLen);
	typedef int (cdecl * CI_SetPMT)(BYTE * pbPMTSection,WORD wSectionLen);
	typedef int (cdecl * CI_SetAllPMT)(BYTE pbPMTSection[32][1024],BYTE bTotalPMTNum,WORD wCurProgNum);
	typedef int (cdecl * CI_SetAllPMT_ProgNum)(BYTE pbPMTSection[32][1024],BYTE bTotalPMTNum,WORD wCurProgNum[32],BYTE bTotalProg);
	typedef int (cdecl * CI_ReSetPMT)(void);
	typedef int (cdecl * CI_SetUTC_time)(BYTE *pbUTCtime);
	typedef int (cdecl * CI_RequestSCStatus)(BYTE index,BYTE *pbBuf,WORD wLen);
	typedef int (cdecl * CI_ReIniCAM)(void);
	typedef int (cdecl * CI_TsOutToCAM)(unsigned char bOut);
	typedef int (cdecl * CI_TimerProcess)(void);
	typedef BOOL (cdecl * CI_ExitCI)();

	CI_Initialize m_Initialize;
	CI_GetSlotStatus m_GetSlotStatus;
	CI_EnterMainMenu m_EnterMainMenu;
	CI_EnterSubMenu m_EnterSubMenu;
	CI_ExitMenu m_ExitMenu;
	CI_SetPinCode m_SetPinCode;
	CI_SetNormalInput m_SetNormalInput;
	CI_SetPMT m_SetPMT;
	CI_SetAllPMT m_SetAllPMT;
	CI_SetAllPMT_ProgNum m_SetAllPMT_ProgNum;
	CI_ReSetPMT m_ReSetPMT;
	CI_SetUTC_time m_SetUTC_time;
	CI_RequestSCStatus m_RequestSCStatus;
	CI_ReIniCAM m_ReIniCAM;
	CI_TsOutToCAM m_TsOutToCAM;
	CI_TimerProcess m_TimerProcess;
	CI_ExitCI m_ExitCI;

	static void CallBack(WORD wType,void *pParams,WORD wLen);
	static void TimerSetCallBack(UINT milliSeconds,PVOID context);

public:
	GeniaCI();
	virtual ~GeniaCI();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ICommonInterface

	STDMETHODIMP SendPMT(const BYTE* pmt, int len);
	STDMETHODIMP HasCard();
};
