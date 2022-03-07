// anyseeNIMEx.h: interface for the CanyseeNIMEx class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ANYSEENIMEX_H__4D429763_A52C_4831_B039_9255A392B127__INCLUDED_)
#define AFX_ANYSEENIMEX_H__4D429763_A52C_4831_B039_9255A392B127__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "anyseeAPI.h"

class CanyseeNIMEx  
{
public:
	/////////////////////// [ DiSEqC Device ] /////////////////////////////////////////////////
	int SendDiSEqCData(PBYTE pData, DWORD dwLength, EnumDiSEqCToneBurst ToneBurst = No_DiSEqCToneBurst);

	int EnableLimit(bool bEnabled = true);
	int SetLimit(bool bEast, bool bEnabled = true );
	int ReCalculatePositions(BYTE Data[3], BOOLEAN bInitiate = true );
	int GotoSatellitePosition(BYTE PositionNumber = 0);
	int StoreSatellitePosition(BYTE PositionNumber = 0);
	int StartStopMotor(BYTE Timeout_Step, bool bStart, bool bEast = true);
	int GotoMotorAngular(int Degree, WORD wFraction);
	int SelectLNB(BYTE DiSEqC10Flag = 0, bool bDiSEqC1v1 = false);
	int SendToneBurst(bool bSA = true);
	int LNBSwitcher(BYTE nLNB, DWORD dwTPFreqMHz, EnumLNBPolarization Polarization);

	/////////////////////// [ LNB Device ] /////////////////////////////////////////////////

	int SetLNBType(EnumLNBType Type = Standard_LNBType );
	int SetLOFBand(DWORD dwLOFrequencyMHz, bool bLowBand = true);
	
	int GetLNBInfo(AMTLNB_INFO	&rLNBInfo);

	int SetLNBPolarization(BOOLEAN bHorizontal = TRUE);	
	int EnableLNB12V(BOOLEAN bEnabled = TRUE);
	int EnableLNB22KHz(BOOLEAN bEnabled = TRUE);
	int EnableLNB(BOOLEAN bEnabled = TRUE);
	
	int SetLNBSwitchFrequency(DWORD dwFrequencyMHz = 0);
	int SetLNBSwitchFrequency(DWORD dwLowLOFMHz, DWORD dwHighLOFMHz, PDWORD pdwSwitchFreqMHz = NULL);

	/////////////////////// [ NIM Device ] /////////////////////////////////////////////////

	int SetPilot(EnumAMTNIM_PILOT Pilot);
	int GetPilot(EnumAMTNIM_PILOT &rPilot);
	
	int SetRollOff(EnumAMTNIM_ROLLOFF RollOff);
	int GetRollOff(EnumAMTNIM_ROLLOFF &rRollOff);
	
	int AttenuateRFSignal(DWORD dwdB = 0);
	int GetUncorrectedTSPCount(DWORD &rdwUncorrectedCount);

	int SetMode(eNIM_Mode Mode);
	int GetMode(eNIM_Mode &rMode);

	bool Locked(void);
	BYTE GetSignalQualityPercent(void);
	BYTE GetSignalStrengthPercent(void);

	int SetSymbolRate(DWORD dwSR);
	int SetFrequency(DWORD dwFrequencyKHz, DWORD dwBandWidthMHz = 0);

	bool IsSupportBroadcastSystem(EnumBroadcastSystem BroadcastSystem);
	int GetNIMIds(AMTNIM_ID &rIds);
	int GetNIMInfo(AMTNIM_INFO &NIMInfo, DWORD dwVersion = 0x01000000);
	int GetNIMInfo(AMTNIM_CFGINFO &rNIMCfgInfo);
	int GetCapabilities(AMTNIM_CAPABILITIES &rCapabilities);

	int GetBoardInfo(AMTBOARD_INFO &rBoardInfo);
	int SetDevice(DWORD dwNumber = 1);

	int Close(DWORD dwNumber = 1);
	int Open(DWORD dwNumber, PDEVICE_INFO pDeviceInfo = NULL);

	////////////////////////////////////////////////////////////////////////////////////////

	DWORD Version(PDWORD pdwVersion = NULL);
	DWORD GetNumberOfDevices(void);

	CanyseeNIMEx();
	virtual ~CanyseeNIMEx();

	////////////////////////////////////////////////////////////////////////////////////////

private:
	PVOID	m_pReserved;
};

#endif // !defined(AFX_ANYSEENIMEX_H__4D429763_A52C_4831_B039_9255A392B127__INCLUDED_)
