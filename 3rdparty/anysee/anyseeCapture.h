// anyseeCapture.h: interface for the CanyseeCapture class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_anyseeCapture_H__3D44C93F_AC1A_487F_A5A7_3F60A3D6597D__INCLUDED_)
#define AFX_anyseeCapture_H__3D44C93F_AC1A_487F_A5A7_3F60A3D6597D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "anyseeAPI.h"

class CanyseeCapture  
{
public:
	DWORD GetBoardcastSystem(void);

	int GetTSProgramNumber(DWORD &rdwProgramNumber);
	int SetMediaCenterProcessId(HANDLE hMCEPId);
	int MCEDVBTFrequencyMap(PMCEDVBT_FREQUENCY_MAP pMap, DWORD dwSize, bool bSet);
	int EnableTerrestrialTPMap(bool bEnable);
	int IsSignalLocked();

	int Descrambler_AddPID(DWORD dwId, DWORD dwPid);
	int Descrambler_RemovePID(DWORD dwId, DWORD dwPid);
	int Descrambler_SetKey(DWORD dwId, DWORD dwPid, PBYTE pData, EnumDVBDescramblerKeyType Type);

	int Descrambler_Open(DWORD dwQITems, DWORD &rdwId);
	int Descrambler_Close(DWORD dwId);
	int Descrambler_Init(DWORD dwId);
	int Descrambler_EnableStreamFlag(DWORD dwId, bool bEnable);

	int LinkKernelEvent(HANDLE hEvent, EnumAMTEVENT_TYPE Type, bool bLink);
	int SetDevice(DWORD dwNumber = 1);
	int GetBoardInfo(AMTBOARD_INFO &rBoardInfo);

	int Start(PFUserProcess pfUserProcess, PVOID pContext, bool bStartStreaming = true );
	int Stop(void);
	int Pause(void);
	int Resume(void);

	int SetTSKSPinStreamFlag(BYTE KsPinType, DWORD dwFlag, bool bSet);
	EnumState State();

	int SetBoardMode(BYTE Mode);
	int GetBoardMode(BYTE& rMode);

	int GetDriverVersion(DWORD &rdwVersion);
	int Open(DWORD dwNumber, PDEVICE_INFO pDeviceInfo = NULL);
	int Close(DWORD dwNumber = 1);

////////////////////////////////////////////////////////////////////////////////////////

	DWORD GetNumberOfDevices(PVOID pList = NULL);
	DWORD Version(void);

	CanyseeCapture();
	virtual ~CanyseeCapture();

////////////////////////////////////////////////////////////////////////////////////////

private:
	PVOID	m_pReserved;
};

#endif // !defined(AFX_anyseeCapture_H__3D44C93F_AC1A_487F_A5A7_3F60A3D6597D__INCLUDED_)
