#ifndef __anyseeAPI_H__
#define __anyseeAPI_H__


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "DATATYPE_DEF.H"
#include "AMTCOMMON1_DEF.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __AMTDESCRAMBLER_DEF_H__
	typedef enum _EnumDVBDescramblerKeyType_
	{
		Even_DVBDescramblerKeyType = 1,
		Odd_DVBDescramblerKeyType,
		Both_DVBDescramblerKeyType
	}EnumDVBDescramblerKeyType;
#endif

#ifndef __AMTNIM_DEF_H__
	#define	AMTNIM_MAX_NIM_COUNT	4		// ｺｸｵ・ｴ・ﾃﾖｴ・NIM ｿｬｰ・ｰｳｼ・ ｰｪ ｹ・ｧ: 1 ~ 0xFFFFFFFE 

	typedef enum _EnumDiSEqCToneBurst_
	{
		No_DiSEqCToneBurst = 0,
		SA_DiSEqCToneBurst,
		SB_DiSEqCToneBurst
	}EnumDiSEqCToneBurst;

	typedef enum _EnumLNBPolarization_
	{
		Horizontal_LNBPolarization = 0,
		Vertical_LNBPolarization 
	}EnumLNBPolarization;

	typedef enum _EnumLNBType_
	{
		Standard_LNBType = 0,
		Universal_LNBType,
	}EnumLNBType;

	typedef struct _AMTLNB_INFO_
	{
		DWORD	dwFlag;	
		DWORD	dwSwicthFrequencyMHz;		// LO ｼｱﾅﾃ ｱ簔ﾘ ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: MHz 
		DWORD	dwHighLOFrequencyMHz;		// High Local oscillator ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: MHz 
		DWORD	dwLowLOFrequencyMHz;		// Low Local oscillator ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: MHz 
		DWORD	dwCurrentLOFrequencyMHz;	// ﾇ・ｻ鄙・ﾏｰ・ﾀﾖｴﾂ Local oscillator ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: MHz 
	}AMTLNB_INFO, *PAMTLNB_INFO;

	typedef enum _EnumAMTNIM_PILOT_
	{
		AMTNIM_Pilot_Off = 0,
		AMTNIM_Pilot_On,
		AMTNIM_Pilot_Auto,
	}EnumAMTNIM_PILOT;

	typedef enum _EnumAMTNIM_ROLLOFF_
	{
		AMTNIM_RollOff_0P2 = 0,		// Roll-Off 0.2
		AMTNIM_RollOff_0P25,		// Roll-Off 0.25
		AMTNIM_RollOff_0P35,		// Roll-Off 0.35
	}EnumAMTNIM_ROLLOFF;

	typedef enum _NIM_Mode_
	{
		enNIM_None = 0,
		enNIM_QPSK,			                    // 1, DVB-S
		enNIM_16QAM,		                    // 2
		enNIM_32QAM,		                    // 3
		enNIM_64QAM,		                    // 4
		enNIM_128QAM,		                    // 5
		enNIM_256QAM,		                    // 6
		enNIM_VSB,			                    // 7
		enNIM_OFDM,			                    // 8
		enNIM_NTSC,			                    // 9
		enNIM_PAL_BG,		                    // 10
		enNIM_PAL_I,		                    // 11
		enNIM_PAL_DK,		                    // 12
		enNIM_SECAM_L,		                    // 13
		enNIM_SECAM_LP,		                    // 14, SECAM L' System
		enNIM_FM_RADIO,                         // 15, FM ｶﾀ
		enNIM_DMB,			                    // 16, DMB ｹ貎ﾄ
		enNIM_DAB,			                    // 17, DAB ｹ貎ﾄ
		enNIM_4QAM,			                    // 18, 4-QAM
		enNIM_BPSK,			                    // 19, BPSK
		enNIM_AM_RADIO,                         // 20, AM ｶﾀ
		enNIM_16QAM_3_4,						// 21, DVB-S, 16QAM 3/4
		enNIM_16QAM_7_8,						// 22, DVB-S, 16QAM 7/8
		enNIM_8PSK_2_3,							// 23, DVB-S, 8PSK 2/3
		enNIM_8PSK_5_6,							// 24, DVB-S, 8PSK 5/6
		enNIM_8PSK_8_9,							// 25, DVB-S, 8PSK 8/9
		enNIM_QPSK_1_2,		                    // 26, DVB-S, QPSK 1/2
		enNIM_QPSK_2_3,		                    // 27, DVB-S, QPSK 2/3
		enNIM_QPSK_3_4,		                    // 28, DVB-S, QPSK 3/4
		enNIM_QPSK_5_6,		                    // 29, DVB-S, QPSK 5/6
		enNIM_QPSK_6_7,		                    // 30, DVB-S, QPSK 6/7
		enNIM_QPSK_7_8,		                    // 31, DVB-S, QPSK 7/8
		enNIM_LDPCBCH_QPSK_1_2,					// 32, DVB-S2(LDPC/BCH) QPSK 1/2
		enNIM_LDPCBCH_QPSK_3_5,					// 33, DVB-S2(LDPC/BCH) QPSK 3/5
		enNIM_LDPCBCH_QPSK_2_3,					// 34, DVB-S2(LDPC/BCH) QPSK 2/3
		enNIM_LDPCBCH_QPSK_3_4,					// 35, DVB-S2(LDPC/BCH) QPSK 3/4
		enNIM_LDPCBCH_QPSK_4_5,					// 36, DVB-S2(LDPC/BCH) QPSK 4/5
		enNIM_LDPCBCH_QPSK_5_6,					// 37, DVB-S2(LDPC/BCH) QPSK 5/6
		enNIM_LDPCBCH_QPSK_8_9,					// 38, DVB-S2(LDPC/BCH) QPSK 8/9
		enNIM_LDPCBCH_QPSK_9_10,				// 39, DVB-S2(LDPC/BCH) QPSK 9/10	
		enNIM_LDPCBCH_8PSK_3_5,					// 40, DVB-S2(LDPC/BCH) 8PSK 3/5
		enNIM_LDPCBCH_8PSK_2_3,					// 41, DVB-S2(LDPC/BCH) 8PSK 2/3
		enNIM_LDPCBCH_8PSK_3_4,					// 42, DVB-S2(LDPC/BCH) 8PSK 3/4
		enNIM_LDPCBCH_8PSK_5_6,					// 43, DVB-S2(LDPC/BCH) 8PSK 5/6
		enNIM_LDPCBCH_8PSK_8_9,					// 44, DVB-S2(LDPC/BCH) 8PSK 8/9
		enNIM_LDPCBCH_8PSK_9_10,				// 45, DVB-S2(LDPC/BCH) 8PSK 9/10	
		enNIM_LDPCBCH_16APSK_2_3,				// 46, DVB-S2(LDPC/BCH) 16APSK 2/3
		enNIM_LDPCBCH_16APSK_3_4,				// 47, DVB-S2(LDPC/BCH) 16APSK 3/4
		enNIM_LDPCBCH_16APSK_4_5,				// 48, DVB-S2(LDPC/BCH) 16APSK 4/5
		enNIM_LDPCBCH_16APSK_5_6,				// 49, DVB-S2(LDPC/BCH) 16APSK 5/6
		enNIM_LDPCBCH_16APSK_8_9,				// 50, DVB-S2(LDPC/BCH) 16APSK 8/9
		enNIM_LDPCBCH_16APSK_9_10,				// 51, DVB-S2(LDPC/BCH) 16APSK 9/10
		enNIM_LDPCBCH_32APSK_3_4,				// 52, DVB-S2(LDPC/BCH) 32APSK 3/4
		enNIM_LDPCBCH_32APSK_4_5,				// 53, DVB-S2(LDPC/BCH) 32APSK 4/5
		enNIM_LDPCBCH_32APSK_5_6,				// 54, DVB-S2(LDPC/BCH) 32APSK 5/6
		enNIM_LDPCBCH_32APSK_8_9,				// 55, DVB-S2(LDPC/BCH) 32APSK 8/9
		enNIM_LDPCBCH_32APSK_9_10,				// 56, DVB-S2(LDPC/BCH) 32APSK 9/10
		enNIM_DTV_1_2,							// 57, DTV Legacy 1/2
		enNIM_DTV_2_3,							// 58, DTV Legacy 2/3
		enNIM_DTV_6_7,							// 59, DTV Legacy 6/7
		enNIM_8PSK,								// 60, 8PSK 
		enNIM_LDPCBCH_QPSK,						// 61, LDPC/BCH QPSK
		enNIM_LDPCBCH_8PSK,						// 62, LDPC/BCH QPSK
		enNIM_LDPCBCH_16APSK,					// 63, LDPC/BCH 16APSK
		enNIM_LDPCBCH_32APSK,					// 64, LDPC/BCH 32APSK
	}eNIM_Mode;

	typedef enum _EnumAMTNIM_Type_
	{
		AMTNIM_Unknown_Type = 0,					// 0
		AMTNIM_DNOS404Zx101x_Type,					// 1
		AMTNIM_DNOS404Zx102x_Type,					// 2
		AMTNIM_DNOS404Zx103x_Type,					// 3
		AMTNIM_TDVSH062P_Type,						// 4
		AMTNIM_MT352_FMD1216ME_Type,				// 5
		AMTNIM_ZL10353_FMD1216ME_Type,				// 6
		AMTNIM_PN3030_ITD3010_Type,					// 7
		AMTNIM_DNOS881Zx121A_Type,					// 8
		AMTNIM_STV0297J_DNOS881Zx121A_Type,			// 9
		AMTNIM_DNQS441PH261A_Type,					// 10
		AMTNIM_TDA10023HT_DTOS203IH102A_Type,		// 11
		AMTNIM_DNBU10321IRT_Type,					// 12
		AMTNIM_FakeHW_Type,							// 13
		AMTNIM_BS2N10WCC01_Type,					// 14, DVB-S2, Cosy NIM
		AMTNIM_ZL10353_XC5000_Type,					// 15
		AMTNIM_TDA10023HT_XC5000_Type,				// 16
		AMTNIM_ZL10353_DTOS203IH102A_Type,			// 17
		AMTNIM_ZL10353_DTOS403Ix102x_Type,			// 18
		AMTNIM_TDA10023HT_DTOS403Ix102x_Type,		// 19
		AMTNIM_MaxCount_Type						// ﾃﾖｴ・AMTNIM Type ｰｳｼ・
	}EnumAMTNIM_Type;

	typedef enum _EnumAMTNIMInversion_
	{
		AMTNIM_Inversion_None = 0,
		AMTNIM_Inversion,
		AMTNIM_Inversion_Auto
	}EnumAMTNIMInversion;

	typedef enum _EnumScanDirection_
	{
		AMTNIM_ScanUp = 0,
		AMTNIM_ScanDown
	}EnumScanDirection;

	typedef enum _EnumAMTNIMCapabilitiesFlag_
	{
		SymbolRate_CapsFlag = BIT0,			// ｽﾉｹ・ｷｹﾀﾌﾆｮ ﾀｯﾈｿ ﾇﾃｷ｡ｱﾗ
		SearchStep_CapsFlag = BIT1,			// Search Step ﾁﾖﾆﾄｼ・ﾀｯﾈｿ ﾇﾃｷ｡ｱﾗ
		LNB_CapsFlag = BIT2,				// LNB ﾀｯﾈｿ ﾇﾃｷ｡ｱﾗ
		RollOff_CapsFlag = BIT3,			// Roll-Off ｰ霈・ﾀｯﾈｿ ﾇﾃｷ｡ｱﾗ
		Pilot_CapsFlag = BIT4,				// Pilot ﾀｯﾈｿ ﾇﾃｷ｡ｱﾗ
	}EnumAMTNIMCapabilitiesFlag;

	typedef struct _AMTNIM_CAPABILITIES_
	{
		DWORD	dwMinFrequencyKHz;	// ﾃﾖｼﾒ ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: KHz
		DWORD	dwMaxFrequencyKHz;	// ﾃﾖｴ・ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: KHz
		
		DWORD	dwMinSymbolRate;	// ﾃﾖｼﾒ ｽﾉｹ・ｷｹﾀﾌﾆｮ, ｴﾜﾀｧ: Baud(=Symbols/Sec)
		DWORD	dwMaxSymbolRate;	// ﾃﾖｴ・ｽﾉｹ・ｷｹﾀﾌﾆｮ, ｴﾜﾀｧ: Baud(=Symbols/Sec)

		LONG	lMinSearchStepHz;	// ﾃﾖｼﾒ ｰﾋｻ・ｽｺﾅﾜ ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: Hz
		LONG	lMaxSearchStepHz;	// ﾃﾖｴ・ｰﾋｻ・ｽｺﾅﾜ ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: Hz

		EnumAMTNIMCapabilitiesFlag	Flags;		// ﾇﾃｷ｡ｱﾗ 
		EnumBroadcastSystem	BroadcastSystem;	// ｹ貍ﾛ ｽﾃｽｺﾅﾛ 
	}AMTNIM_CAPABILITIES, *PAMTNIM_CAPABILITIES;

	typedef struct	_AMTNIM_CFGINFO_
	{
		DWORD	dwSymbolRate;			// ｴﾜﾀｧ: Baud(=Symbols/Sec)
		LONG	lSweepRate;				// ｴﾜﾀｧ: Hz/s

		DWORD	dwFrequency;			// ﾇ・ｼｳﾁ､ｵﾈ ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: KHz
		LONG	lCarrierOffsetKHz;		// ﾄｳｸｮｾ・ﾁﾖﾆﾄｼﾍﾀﾇ ｿﾀﾂ・ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: KHz
		BYTE	BandWidth;				// ﾇ・ｼｳﾁ､ｵﾈ ｴ・ｪ, ｴﾜﾀｧ: MHz
		
		EnumAMTNIM_Type	Type;			// NIM ﾁｾｷ・
		eNIM_Mode		AnalogueMode;	// ﾇ・ｼｳﾁ､ｵﾈ ｾﾆｳｯｷﾎｱﾗ ｵｿﾀﾛ ｹ貎ﾄ
		eNIM_Mode		DigitalMode;	// ﾇ・ｼｳﾁ､ｵﾈ ｵﾐ ｵｿﾀﾛ ｹ貎ﾄ
		EnumAMTNIMInversion	SpectrumInversion;		// ｽｺﾆ衄ｮｷｳ ｹﾝﾀ・ｹ貎ﾄ
		EnumScanDirection	Direction;	// ｰﾋｻ・ｹ貮・
	}AMTNIM_CFGINFO, *PAMTNIM_CFGINFO;

	typedef struct _AMTNIM_ID_
	{
		DWORD	dwNumberOfNIM;								// NIM ｰｳｼ・
		DWORD	dwTypes[AMTNIM_MAX_NIM_COUNT];				// NIM ﾁｾｷ・
		DWORD	dwIds[AMTNIM_MAX_NIM_COUNT];				// NIM Idｵ・
		DWORD	dwBroadcastSystems[AMTNIM_MAX_NIM_COUNT];	// NIM ｹ貍ﾛ ｽﾃｽｺﾅﾛ
	}AMTNIM_ID, *PAMTNIM_ID;

	// 1.0 ｹ・AMTNIM ﾁ､ｺｸ ｱｸﾁｶﾃｼ
	typedef struct _AMTNIM_INFO_1V0_
	{
		DWORD		dwFlags;		// AMTNIMEx ﾇﾃｷ｡ｱﾗ
		DWORD		dwFWFlags;		// AMTNIMFW ﾇﾃｷ｡ｱﾗ
		AMTNIM_ID	NIMId;			// NIM Id ﾁ､ｺｸ
	}AMTNIM_INFO_1V0, *PAMTNIM_INFO_1V0;

	typedef struct _AMTNIM_INFO_
	{
		DWORD	dwVersion;			// AMTNIM ﾁ､ｺｸ ｱｸﾁｶﾃｼ ｹ・
	#ifdef ANYSEE_PROPERTY_x64
		QWORD	pAMTNIMInfo;		// AMTNIM ﾁ､ｺｸ ｱｸﾁｶﾃｼ ﾆﾎﾅﾍ.
	#else
		PVOID	pAMTNIMInfo;		// AMTNIM ﾁ､ｺｸ ｱｸﾁｶﾃｼ ﾆﾎﾅﾍ.
	#endif
	}AMTNIM_INFO, *PAMTNIM_INFO;


#endif	// #ifndef __AMTNIM_DEF_H__

#ifndef	__STREAM_DEF_H__

#define AMTSTREAM_BUFFER_SIZE	( 128 * KB )
typedef struct _AMTSTREAM_
{
	DWORD	dwId;								// Id
	DWORD	dwUsed;								// ｹﾛ ｻ鄙・ｱ貘ﾌ
	BYTE	Buffer[ AMTSTREAM_BUFFER_SIZE ];	// ｹﾛ
}AMTSTREAM, *PAMTSTREAM;

#endif	// #ifndef		__STREAM_DEF_H__

#ifndef	__anyseeDeviceCapture_DEF_H__

#define MAX_MCEDVBT_FREQUENCY_MAP	162
typedef struct _MCEDVBT_FREQUENCY_MAP_
{
	DWORD	dwDVBTFrequencyKHz;		// DVB-T ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: KHz
	DWORD	dwMapFrequencyKHz;		// ｸﾊﾇﾎｽﾃﾅｳ ﾁﾖﾆﾄｼ・ ｴﾜﾀｧ: KHz
	DWORD	dwSymbolrateSps;		// ｽﾉｹ弡ｹﾀﾌﾆｮ, ｴﾜﾀｧ: Sps
	DWORD	dwHighLOFMHz;			// High-LOF, ｴﾜﾀｧ: MHz
	DWORD	dwLowLOFMHz;			// Low-LOF, ｴﾜﾀｧ: MHz
	DWORD	dwSwitchLOFMHz;			// Switch-LOF, ｴﾜﾀｧ: MHz
	BYTE	NIMMode;				// NIM ｵｿﾀﾛ ｹ貎ﾄ
	BYTE	LNBPolarization;		// LNB ﾆ枻ﾄ ｱﾘｼｺ, H(ｼ・: 0, V(ｼ・: 1
	BYTE	LNBSwitchPort;			// LNB ｽｺﾀｧﾄ｡ ﾆｮ ｹ｣, 0: ｻ鄙・ｾﾈﾇﾔ. 
	BYTE	BandWidthMHz;			// ｴ・ｪ, ｴﾜﾀｧ: MHz
}MCEDVBT_FREQUENCY_MAP, *PMCEDVBT_FREQUENCY_MAP;

#endif

typedef struct _DEVICE_INFO_
{
	HANDLE	hRemoveDeviceEvent;		// Device Removal Event Handle
	HANDLE	hChannelChangeEvent;	// Channel Change Event Handle

	DWORD	dwHubId;				// Hub Id
	DWORD	dwPortNumber;			// Port Number

	PVOID	pReserved;				
}DEVICE_INFO, *PDEVICE_INFO;

typedef struct _TSFILE_INFO_
{
	PCHAR	pFile;
	WORD	wPCR_PID;			// PCR PID

	DWORD	dwTransportRate;	// ﾀ・ﾛ ｼﾓｵｵ
	DWORD	dwTotalTime_ms;		// ﾃﾑ ｽﾃｰ｣, ｴﾜﾀｧ: ms

	__int64	FileSize;			// ﾆﾄﾀﾏ ﾅｩｱ・
	__int64	FirstPCRFileOffset;	// FirstPCR TS ﾆﾐﾅｶ(ｵｿｱ篁ﾙﾀﾌﾆｮ(0x47)) ﾆﾄﾀﾏ ﾀｧﾄ｡
	__int64	LastPCRFileOffset;	// qwLastPCR TS ﾆﾐﾅｶ(ｵｿｱ篁ﾙﾀﾌﾆｮ(0x47)) ﾆﾄﾀﾏ ﾀｧﾄ｡

	QWORD	qwFirstPCR;			// ﾃｳﾀｽﾀｸｷﾎ ｱｸﾇﾑ PCRｰｪ
	QWORD	qwLastPCR;			// ｸｶﾁｷﾀｸｷﾎ ｱｸﾇﾑ PCRｰｪ
}TSFILE_INFO, *PTSFILE_INFO;

typedef	int (*PFUserProcess)(PVOID pContext, PVOID pData, DWORD dwDataLength);
typedef	int (*PFanyseeStreamProcess)(PVOID pContext, PAMTSTREAM pStream);

#define anyseeAPI_INRANGE(Min,Number,Max) (( Min<=Number ) && ( Number <= Max ))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int anyseeAPI_Open(void);
int anyseeAPI_Close(void);
int anyseeAPI_GetOSBit(void);
int anyseeAPI_VersionString(DWORD dwVersion, PCHAR pBuffer, DWORD dwSize);
int anyseeAPI_GetOSVersion(LPOSVERSIONINFO lpVersionInformation);

int anyseeAPI_Print(const char *format, ...);
int anyseeAPI_PrintBuffer(PBYTE pBuffer, DWORD dwBufferSize, PCHAR pBufferName, BYTE NumberOfOutputPerLine);
int anyseeAPI_PrintBuffer(PBYTE pBuffer, DWORD dwBufferSize, PCHAR pBufferName, DWORD dwBegin, BYTE NumberOfOutputPerLine);

int anyseeAPI_SetTopWindow(bool bAlwaysTop, HWND hWnd, HWND* phOldWnd);

void anyseeAPI_InitTickTime(void);
QWORD anyseeAPI_GetTickTime(void);

long anyseeAPI_strcpy(PCHAR pDest, PCHAR pSrc, DWORD dwDestBufSize);
int anyseeAPI_strnicmp(const char *string1, const char *string2, size_t count);
int anyseeAPI_GetHostIP(PCHAR pBuffer, DWORD dwSize);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // #ifndef __anyseeAPI_H__
