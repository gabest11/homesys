// anyseeSC.h: interface for the CanyseeSC class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ANYSEESC_H__7CAE6095_17CA_4DBE_8947_766291B577C1__INCLUDED_)
#define AFX_ANYSEESC_H__7CAE6095_17CA_4DBE_8947_766291B577C1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>  
#include "anyseeAPI.h"

#define	SC_ATR_MAX_LENGTH	33

typedef enum _EnumSC_ProtocolType_
{
	SC_T0_PT = 0,	
	SC_T1_PT,		
	SC_T2_PT,		
	SC_T3_PT,		
	SC_T4_PT,		
	SC_T5_PT,		
	SC_T6_PT,
	SC_T7_PT,
	SC_T8_PT,
	SC_T9_PT,
	SC_T10_PT,
	SC_T11_PT,
	SC_T12_PT,
	SC_T13_PT,
	SC_T14_PT,		
	SC_T15_PT,
	SC_NUMBER_OF_PT	
}EnumSC_ProtocolType;

typedef enum _EnumSC_ATR_TIFlag_
{
	SC_ATR_TI_INITFlag = 0,
	SC_ATR_FirstT15Flag = BIT0,	
	SC_ATR_TCKFlag = BIT1,		
	SC_ATR_TA1Flag = BIT4,
	SC_ATR_TB1Flag = BIT5,
	SC_ATR_TC1Flag = BIT6,
	SC_ATR_TD1Flag = BIT7,
	SC_ATR_TA2Flag = BIT8,
	SC_ATR_TB2Flag = BIT9,
	SC_ATR_TC2Flag = BIT10,
	SC_ATR_TD2Flag = BIT11,
	SC_ATR_TA3Flag = BIT12,
	SC_ATR_TB3Flag = BIT13,
	SC_ATR_TC3Flag = BIT14,
	SC_ATR_TD3Flag = BIT15,
	SC_ATR_TA4Flag = BIT16,
	SC_ATR_TB4Flag = BIT17,
	SC_ATR_TC4Flag = BIT18,
	SC_ATR_TD4Flag = BIT19,
}EnumSC_ATR_TIFlag;

typedef struct _SC_ATRDATA_ELEMENTS_
{
	BYTE	ATRLength;
	EnumSC_ATR_TIFlag	TIFlag;
	EnumSC_ProtocolType	ProtocolType;
	
	BYTE	TSData;
	BYTE	T0Data;
	BYTE	TAData[4];
	BYTE	TBData[4];
	BYTE	TCData[4];
	BYTE	TDData[4];
	BYTE	TKData[31];		
	BYTE	TCKData;

	BYTE	FI;		// Clock rate conversion factor
	BYTE	DI;		// Bit rate adjustment factor

	BYTE	II;		// Maximum Programming current factor 
	BYTE	PI1;	// Programming voltage1 factor
	BYTE	PI2;	// Programming voltage2 factor

	BYTE	N;		// Extra Guard Time
	BYTE	SpecificModeByte;	// Specific Mode Byte

	BYTE	WI;		// Work Waiting Integer
	BYTE	XI;		// Clock Stop Indicator
	BYTE	UI;		// Class Indicator	

	BYTE	CWI;	// Character Waiting Integer
	BYTE	BWI;	// Block Waiting Time

	BYTE	IFSC;	
	BYTE	IFSD;	

	BYTE	ErrorDetectionCode;	// Bit0 => 0: LRC, 1: CRC

}SC_ATRDATA_ELEMENTS, *PSC_ATRDATA_ELEMENTS;

class CanyseeSC  
{
public:
	int GetBoardInfo(AMTBOARD_INFO &rBoardInfo);

	BYTE Slot(void);
	bool IsPresent(void);

	int SetSlot(BYTE Slot);
	int SetDevice(DWORD dwNumber = 1);

	int ReadWriteT0(PBYTE pWBuffer, DWORD dwWBufferLength, PBYTE pRBuffer, DWORD dwRBufferLength, WORD &rwSW);
	int SendDataT14(PBYTE pWBuffer, DWORD &rdwSLength, PBYTE pRBuffer, DWORD &rdwRLength, WORD &rwSW);
	
	int Activation(BYTE ATRData[SC_ATR_MAX_LENGTH], DWORD &dwATRLength, PSC_ATRDATA_ELEMENTS pATRDataElements = NULL);
	int DeActivation(void);
	
	int Open(DWORD dwNumber, PDEVICE_INFO pDeviceInfo = NULL);
	int Close(DWORD dwNumber = 1);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	DWORD GetNumberOfDevices(void);
	DWORD Version(void);

	CanyseeSC();
	virtual ~CanyseeSC();

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
	BYTE	m_Slot;
	PVOID	m_pReserved;
};

#endif // !defined(AFX_ANYSEESC_H__7CAE6095_17CA_4DBE_8947_766291B577C1__INCLUDED_)
