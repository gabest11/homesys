#ifndef		__AMTCOMMON1_DEF_H__
#define		__AMTCOMMON1_DEF_H__

#include "DATATYPE_DEF.H"

#define	MAX_BOARD_ID			4				// ﾃﾖｴ・ｺｸｵ・Id ｰｳｼ・

typedef enum _EnumOS_
{
	MSWindows2000_OS = 0,
	MSWindows2000SP1,
	MSWindows2000SP2,
	MSWindows2000SP3,
	MSWindows2000SP4,
	MSWindowsXP_OS,
	MSWindowsXPSP1_OS,
	MSWindowsXPSP2_OS,
	MSWindowsXPSP3_OS,
	MSWindowsServer2003_OS,
	MSWindowsServer2003SP1_OS,
	MSWindowsServer2003SP2_OS,
	MSWindowsVista_OS,
	MSWindowsVistaSP1_OS,
	MSWindowsServer2008_OS,
	MSWindows7_OS,
}EnumOS;

// AMT ｵ蠍ﾌｹ・ｽﾃﾀﾛ ｹ貎ﾄ
typedef enum _enumAMTDRIVER_START_MODE_
{  
	enAMTDRIVER_AutoDetectMode = 0,
	enAMTDRIVER_USBDeviceMode,
	enAMTDRIVER_PCIDeviceMode,
	enAMTDRIVER_FakeHWDeviceMode = 11
}AMTDRIVER_START_MODE; 

typedef enum _EnumBusType_
{  
	USB_BusType = 0,	// USB ｹｺ
	PCI_BusType,		// PCI ｹｺ 
	I2C_BusType,		// I2C ｹｺ 
	VUSB_BusType,		// ｰ｡ｻ・USB ｹｺ
	VPCI_BusType,		// ｰ｡ｻ・PCI ｹｺ 
	VI2C_BusType,		// ｰ｡ｻ・I2C ｹｺ 
	Maximum_BusType		// ﾃﾖｴ・ｹｺ ﾁｾｷ・ｰｳｼ・
}EnumBusType; 

typedef	enum _enumAMTGPIO_
{
	enAMTGPIO_PortA = 0,		// PORTA, PART0	
	enAMTGPIO_PortB,			// PORTB, PART1
	enAMTGPIO_PortC,			// PORTC, PART2	
	enAMTGPIO_PortD,			// PORTD, PART3	
	enAMTGPIO_PortE,			// PORTE, PART4	
	enAMTGPIO_PortF,			// PORTF, PART5	
	enAMTGPIO_MaxCount			// ﾃﾖｴ・GPIO ｰｳｼ・
}EnumAMTGPIO;

typedef	enum _enumAMTRESET_TYPE_
{
	enAMTRESET_Hardware = 0,
	enAMTRESET_Software,
	enAMTRESET_Cold,
	enAMTRESET_Warm,
	enAMTRESET_MaxCount			// ﾃﾖｴ・ｸｮｼﾂ ﾁｾｷ・ｰｳｼ・
}EnumAMTRESET_TYPE;

typedef enum _EnumAMTTIME_UNIT_
{
	enMicroSecondUnit = 10,
	enMilliSecondUnit = 10000,
	enSecondUnit = 10000000,
	en2SecondUnit = 20000000,
	en3SecondUnit = 30000000,
	en5SecondUnit = 50000000,
	en10SecondUnit = 100000000
}EnumAMTTIME_UNIT;

typedef enum _EnumAMTEVENT_TYPE_
{
	enAMTEVENT_Unknown = 0,				// ｾﾋ ｼ・ｾｽ.
	enAMTEVENT_SurpriseRemoval,			// ｿｹｱ籠｡ ｾﾊﾀｺ ﾁｦｰﾅ ﾀﾌｺ･ﾆｮ
	SurpriseRemoval_AMTEvent,			// ｿｹｱ籠｡ ｾﾊﾀｺ ﾁｦｰﾅ ﾀﾌｺ･ﾆｮ
	ChannelChange_AMTEvent,				// ﾃ､ｳﾎ ｺｯｰ・ﾀﾌｺ･ﾆｮ
	TPFrequencyChange_AMTEvent,			// TP ﾁﾖﾆﾄｼ・ｺｯｰ・ﾀﾌｺ･ﾆｮ
	TSProgramNumberUpdate_AMTEvent,		// TS ｽｺﾆｮｸｲ ﾇﾁｷﾎｱﾗｷ･ ｹ｣ ｰｻｽﾅ ﾀﾌｺ･ﾆｮ
	ChannelScanStart_AMTEvent,			// ﾃ､ｳﾎ ｰﾋｻ・ｽﾃﾀﾛ ﾀﾌｺ･ﾆｮ
	ChannelScanStop_AMTEvent,			// ﾃ､ｳﾎ ｰﾋｻ・ﾁﾟﾁ・ﾀﾌｺ･ﾆｮ
	MCTSProgramNumberUpdate_AMTEvent,	// ｹﾌｵ・ｼｾﾅﾍ TS ｽｺﾆｮｸｲ ﾇﾁｷﾎｱﾗｷ･ ｹ｣ ｰｻｽﾅ ﾀﾌｺ･ﾆｮ
	MCTPFrequencyChange_AMTEvent,		// ｹﾌｵ・ｼｾﾅﾍ TP ﾁﾖﾆﾄｼ・ｺｯｰ・ﾀﾌｺ･ﾆｮ
	NumberOfAMTEvent,					// ﾀﾌｺ･ﾆｮ ﾃﾑ ｰｳｼ・
}EnumAMTEVENT_TYPE;

typedef enum _EnumBoardType_
{
	Unknown_AMTBoardType = 0,		// ｾﾋ ｼ・ｾｽ.
	ATV_AMTBoardType,				// ｾﾆｳｯｷﾎｱﾗ TV 
	DTV_AMTBoardType,				// ｵﾐ TV
	ADTV_AMTBoardType,				// ｾﾆｳｯｷﾎｱﾗ ｹﾗ ｵﾐ TV ｰ篩・
	TSEquipment_AMTBoardType,		// TS ｽｺﾆｮｸｲ ﾀ蠎・
	NANDFlashMemory_AMTBoardType	// NAND Flash ｸﾞｸｮ ｺｸｵ・
}EnumBoardType;

typedef enum _EnumAccessType_
{
    AMT_RW_AccessType = 0,		// ﾀﾐｰ・ｾｲｱ・ｰ｡ｴﾉ	
	AMT_R_AccessType,			// ﾀﾐｱ篋ｸ ｰ｡ｴﾉ
	AMT_W_AccessType,			// ｾｲｱ篋ｸ ｰ｡ｴﾉ
	AMT_NON_AccessType			// Skip
}EnumAccessType;

typedef enum _EnumBroadcastSystem_
{
	Unknown_BroadcastSystem = 0,		// 0, ｾﾋ ｼ・ｾｽ.
	NTSC_BroadcastSystem,				// 1, NTSC
	NTSC_M_BroadcastSystem	= NTSC_BroadcastSystem,		// 1, NTSC-M
	ATSC_BroadcastSystem,				// 2, ATSC
	DMB_BroadcastSystem,				// 3, DMB
	DIRECTV_BroadcastSystem,			// 4, DirectTV
	DAB_BroadcastSystem,				// 5, DAB
	PAL_BroadcastSystem,				// 6, PAL
	PAL_BG_BroadcastSystem = PAL_BroadcastSystem,		// 6, PAL-B/G
	SECAM_BroadcastSystem,				// 7, SECAM
	SECAM_L_BroadcastSystem	= SECAM_BroadcastSystem,	// 7, SECAM-L
	DVBT_BroadcastSystem,				// 8, DVB-T
	DVBC_BroadcastSystem,				// 9, DVB-C
	DVBS_BroadcastSystem,				// 10, DVB-S
	DVBS2_BroadcastSystem,				// 11, DVB-S2
	DVBH_BroadcastSystem,				// 12, DVB-H
	DVBHD_BroadcastSystem,				// 13, DVB-HD
	ISDBT_BroadcastSystem,				// 14, ISDB-T
	ISDBC_BroadcastSystem,				// 15, ISDB-C
	ISDBS_BroadcastSystem,				// 16, ISDB-S
	DMBTH_BroadcastSystem,				// 17, DMB-TH
	NTSC_N_BroadcastSystem,				// 18, NTSC-N
	NTSC_J_BroadcastSystem,				// 19, NTSC-J
	PAL_DK_BroadcastSystem,				// 20, PAL-D/K
	PAL_I_BroadcastSystem,				// 21, PAL-I
	PAL_M_BroadcastSystem,				// 22, PAL-M
	PAL_N_BroadcastSystem,				// 23, PAL-N
	SECAM_LP_BroadcastSystem,			// 24, SECAM-L'
	SECAM_DK_BroadcastSystem,			// 25, SECAM-D/K
	AM_Radio_BroadcastSystem,			// 26, AM Radio
	FM_Radio_BroadcastSystem,			// 27, FM Radio
	DTV_6MHz_BroadcastSystem,			// 28, 6MHz ｵﾐ ｹ貍ﾛ.
	DTV_7MHz_BroadcastSystem,			// 29, 7MHz ｵﾐ ｹ貍ﾛ.
	DTV_8MHz_BroadcastSystem,			// 30, 8MHz ｵﾐ ｹ貍ﾛ.
	DTV_7_8MHz_BroadcastSystem,			// 31, 7/8 MHz ｵﾐ ｹ貍ﾛ.
	ClearQAM_BroadcastSystem,			// 32, ClearQAM
}EnumBroadcastSystem;

typedef enum _EnumBroadcastSystemFlag_
{
	Unknown_BroadcastSystemFlag = 0,				// ｾﾋ ｼ・ｾｽ.
	NTSC_BroadcastSystemFlag	= BIT0,				// NTSC
	ATSC_BroadcastSystemFlag	= BIT1,				// ATSC
	DMB_BroadcastSystemFlag		= BIT2,				// DMB
	DIRECTV_BroadcastSystemFlag = BIT3,				// DirectTV
	DAB_BroadcastSystemFlag		= BIT4,				// DAB
	PAL_BroadcastSystemFlag		= BIT5,				// PAL
	SECAM_BroadcastSystemFlag	= BIT6,				// SECAM
	DVBT_BroadcastSystemFlag	= BIT7,				// DVB-T
	DVBC_BroadcastSystemFlag	= BIT8,				// DVB-C
	DVBS_BroadcastSystemFlag	= BIT9,				// DVB-S
	DVBS2_BroadcastSystemFlag	= BIT10,			// DVB-S2
	DVBH_BroadcastSystemFlag	= BIT11,			// DVB-H
	DVBHD_BroadcastSystemFlag	= BIT12,			// DVB-HD
	ISDBT_BroadcastSystemFlag	= BIT13,			// ISDB-T
	ISDBC_BroadcastSystemFlag	= BIT14,			// ISDB-C
	ISDBS_BroadcastSystemFlag	= BIT15,			// ISDB-S
	DMBTH_BroadcastSystemFlag	= BIT16,			// ISDB-S
	NTSC_M_BroadcastSystemFlag	= NTSC_BroadcastSystemFlag,		// NTSC-M
	NTSC_N_BroadcastSystemFlag	= BIT17,			// NTSC-N
	NTSC_J_BroadcastSystemFlag	= BIT18,			// NTSC-J
	PAL_BG_BroadcastSystemFlag	= PAL_BroadcastSystemFlag,		// PAL-B/G
	PAL_DK_BroadcastSystemFlag	= BIT19,			// PAL-D/K
	PAL_I_BroadcastSystemFlag	= BIT20,			// PAL-I
	PAL_M_BroadcastSystemFlag	= BIT21,			// PAL-M
	PAL_N_BroadcastSystemFlag	= BIT22,			// PAL-N
	SECAM_L_BroadcastSystemFlag	= SECAM_BroadcastSystemFlag,	// SECAM-L
	SECAM_LP_BroadcastSystemFlag	= BIT23,		// SECAM-L'
	SECAM_DK_BroadcastSystemFlag	= BIT24,		// SECAM-D/K
	AM_Radio_BroadcastSystemFlag	= BIT25,		// AM Radio
	FM_Radio_BroadcastSystemFlag	= BIT26,		// FM Radio
	ClearQAM_BroadcastSystemFlag	= BIT27,		// ClearQAM
}EnumBroadcastSystemFlag;

// USBASI ｵｿﾀﾛ ｸ・
typedef enum _EnumUSBASIMode_
{  
	USBASI_BypassMode = 0,             // ASI ｹﾙﾀﾌﾆﾐｽｺ ｸ・ ｱ篌ｻｰｪ
	USBASI_InMode,                     // ASI ﾀﾔｷﾂ ｸ・ ASI -> USB
	USBASI_OutMode	                   // ASI ﾃ箙ﾂ ｸ・ USB -> ASI
}EnumUSBASIMode; 

/* ------------------------------------------------------------------
	 TS(Transport Stream) ｽｺﾆｮｸｲ ｹﾗ ﾇﾃｷｧﾆ・ｵｿﾀﾛ ｹ貎ﾄ ﾁｦｾ・ﾄﾚｵ・
     
	 - 2004.10.15, Airfox
		1. USBTS_xxx
			(1) EZ-USBFX2ｳｻﾀﾇ ｺｿｱ・FIFO ｵｿﾀﾛ ｹ貎ﾄ
			(2) ﾁｦｾ鍗ﾚｵ蟶ｦ USB ﾆﾟｿｾ﨧ﾎ ｹﾙｷﾎ ﾀ・ﾞﾇﾑｴﾙ.
			(3) ｱ簔ｸ ｹ・・ﾈ｣ﾈｯｼｺﾀｻ ﾀｧﾇﾘ ｳｲｰﾜｵﾐｴﾙ.
		2. PLATFORM_xxx
			(1) ﾇﾃｷｧﾆ・ｵｿﾀﾛ ｹ貎ﾄ
			(2) ﾁｦｾ鍗ﾚｵ蟆｡ USB ﾆﾟｿｾ﨧ﾎ ｹﾙｷﾎ ﾀ・ﾞｵﾇﾁ・ｾﾊｰ・ WDM ｵ蠍ﾌｹ｡ ﾀﾇﾇﾘ	ｸ銧｡ﾁ・ｵｿﾀﾛ ﾃﾊｱ篳ｭ ｰ愠､ﾀｻ ｰﾅﾄ｣ｴﾙ.
   ------------------------------------------------------------------ */
typedef enum _EnumBoardMode_
{  
	USBTS_BypassMode = 0,				// ｹﾙﾀﾌﾆﾐｽｺ ｸ・ ｱ篌ｻｰｪ
	USBTS_InMode,						// ﾀﾔｷﾂ ｸ・ TSout(TNIM) -> USB
	USBTS_OutMode,						// ﾃ箙ﾂ ｸ・ USB -> TSin(MPEG Decoder)		
	
	// 2004.10.15, Airfox
	PLATFORM_eAsyncFIFOIn,				// ｺｿｱ簓・FIFO ﾀﾔｷﾂ
	PLATFORM_eAsyncFIFOOut,				// ｺｿｱ簓・FIFO ﾃ箙ﾂ
	PLATFORM_eSyncFIFOIn,				// ｵｿｱ・FIFO ﾀﾔｷﾂ ｸ・
	PLATFORM_eSyncFIFOOut,				// ｵｿｱ・FIFO ﾃ箙ﾂ ｸ・
	PLATFORM_eATV = 11,					// ｾﾆｳｯｷﾎｱﾗ TV ｸ・
	PLATFORM_eDTV,						// ｵﾐ TV ｸ・
	PLATFORM_eTS,						// Transport Stream ｸ・
	PLATFORM_eAExtAV,					// ｾﾆｳｯｷﾎｱﾗ ｿﾜｺﾎ AVﾀﾔｷﾂ ｸ・
	PLATFORM_eNoSetup,					// ｼｳﾁ､ｾｽ.
	PLATFORM_eGetFailure,				// ﾇﾃｷｧﾆ・ｱｸﾇﾏｱ・ｽﾇﾆﾐ

	GPIO_BoardMode,						// GPIO ｹ貎ﾄ 
	GPIF_BoardMode,						// GPIF ｹ貎ﾄ 

	Unknown_BoardMode = PLATFORM_eNoSetup,	// ｸ｣ｴﾂ ｺｸｵ・ｵｿﾀﾛ ｹ貎ﾄ
}EnumBoardMode; 

// ID ﾄﾚｵ・ﾀﾐｴﾂ ｹ貎ﾄ
typedef enum _EnumIDReadMode_
{  
	AMTUSB_eNone = 0,	// ﾀﾐﾁ・ｾﾊﾀｽ.
	AMTUSB_eCPLD,		// CPLDｸｦ ﾅ・ﾘ ﾀﾐﾀｽ.
	AMTUSB_eI2C,		// I2C ｹ貎ﾄﾀｸｷﾎ ﾀﾐﾀｽ.
	AMTUSB_eMiCOM,		// MiCOM ｹ貎ﾄﾀｸｷﾎ ﾀﾐﾀｽ.
}EnumIDReadMode; 

typedef enum _EnumDataType_
{
	Byte_DataType = 0,
	Word_DataType,
	DWord_DataType,
	QWord_DataType	
}EnumDataType;

typedef enum _EnumDeviceType_
{
	Unknown_DeviceType = 0,						// ｾﾋ ｼ・ｾｽ.
	USB_DeviceType = BIT0,						// USB
	NIM_DeviceType = BIT1,						// NIM
	AudioDecoder_DeviceType = BIT2,				// ｿﾀｵﾀ ｵﾚｴ・
	VideoDecoder_DeviceType = BIT3,				// ｺﾀ ｵﾚｴ・
	SmartCard_DeviceType = BIT4,				// ｽｺｸｶﾆｮ ﾄｫｵ・
	IR_DeviceType = BIT5,						// IR
	AudioEncoder_DeviceType = BIT6,				// ｿﾀｵﾀ ｿ｣ﾄﾚｴ・
	VideoEncoder_DeviceType = BIT7,				// ｺﾀ ｿ｣ﾄﾚｴ・
	StreamCapture_DeviceType = BIT8,			// ｽｺﾆｮｸｲ ﾄｸﾃﾄ ﾀ蠧｡
	NANDFlashMemory_DeviceType = BIT9,			// NAND Flash Memory
	IO_DeviceType = BIT10,						// I/O ﾀ蠧｡
}EnumDeviceType;

typedef enum _EnumLetterCase_
{
	Normal_LetterCase = 0,		// ﾀﾏｹﾝ ｹｮﾀﾚ
	Lower_LetterCase,			// ｼﾒｹｮﾀﾚ
	Upper_LetterCase,			// ｴ・ｮﾀﾚ
}EnumLetterCase;

typedef enum _EnumStreamingState_
{
	Stop_StreamingState = 0,
	Pause_StreamingState,
	Run_StreamingState,
}EnumStreamingState;

typedef enum _EnumState_
{
	FailedToGetState = 0,		// ｻﾂ ｱｸﾇﾏｱ・ｽﾇﾆﾐ
	Stop_State,					// ﾁﾟﾁ・ｻﾂ
	Start_State,				// ｽﾃﾀﾛ ｻﾂ
	Run_State,					// ｽﾇﾇ・ｻﾂ
	RequestStop_State,			// ﾁﾟﾁｻ ｿ菘ｻﾇﾑ ｻﾂ
	RequestStart_State,			// ｽﾃﾀﾛﾀｻ ｿ菘ｻﾇﾑ ｻﾂ
	RequestRun_State,			// ｽﾇﾇ狢ｻ ｿ菘ｻﾇﾑ ｻﾂ
	Pause_State,				// ﾀﾏｽﾃﾁﾟﾁ・ｻﾂ
}EnumState;

typedef enum _EnumI2CBusState_
{
	Idle_I2CBusState = 0,
	Start_I2CBusState,
	Data_I2CBusState,
	Stop_I2CBusState,
}EnumI2CBusState;

typedef enum _EnumBoardFlag_
{
	Unknown_BoardFlag = 0,
	HighSpeedUSB_BoardFlag = BIT1,		// ｰ晴ﾓ USB ｺｸｵ・
	MultiNIM_BoardFlag = BIT2,			// ｴﾙﾁﾟ NIM ｽﾃｽｺﾅﾛ
	PowerDown_BoardFlag = BIT3,			// ｺｸｵ蟆｡ ﾀﾀ・ｻﾂﾀﾓ.
}EnumBoardFlag;

typedef enum _EnumFlagType_
{
	Default_FlagType = 0,
	AMTRP_FlagType = Default_FlagType,
	Board_FlagType,
	AMTNIMEx_FlagType,
	Maximum_FlagType,
}EnumFlagType;

typedef struct _AMTRange_
{
	LONG	lMin;
	LONG	lMax;
}AMTRange, *PAMTRange;

typedef enum _EnumAddressFormat_
{
	NoAddressFormat = 0,		// ｹ・ｾｽ 
	NormalAddressFormat,		// ﾀﾏｹﾝﾀ・ｹ・ﾇ・ﾄ
	RowColumnAddressFormat,		// ﾇ・ｹﾍ ｿｭ ｹﾎ ｱｸｼｺｵﾈ ｹ・ﾇ・ﾄ
	BlockPageAddressFormat,		// ｺ昞ｰ, ﾆ菎ﾌﾁﾎ ｱｸｼｺｵﾈ ｹ・ﾇ・ﾄ
}EnumAddressFormat;

typedef enum _EnumAMTDebugLevel_
{
	AMTDebugLevel_0	= 0,
	AMTDebugLevel_1,
	AMTDebugLevel_2,
	AMTDebugLevel_3,
	AMTDebugLevel_4,
}AMTDebugLevel;

typedef struct _AMTBOARD_INFO_
{
	DWORD	dwVersion;					// ｱｸﾁｶﾃｼ ｹ・

	DWORD	dwBusType;					// ﾀ蠧｡ ｹｺ ﾁｾｷ・ EnumBusType ﾂ・ｶ.
	int		BoardType;					// ｺｸｵ・ﾁｾｷ・ EnumBoardType ﾂ・ｶ.
	DWORD	dwBoardFlag;				// ｺｸｵ・ﾇﾃｷ｡ｱﾗ 
	DWORD	dwBoardMode;				// ﾇ・ｺｸｵ・ｵｿﾀﾛ ｹ貎ﾄ, EnumBoardMode ﾂ・ｶ.

	DWORD	dwBoardIds[MAX_BOARD_ID];	// ｺｸｵ・Idｵ・
										// ｻﾎ ｹ｣ 0: ﾁﾖ ｺｸｵ・Id, ｻﾎ ｹ｣ 0ｿﾜ: ｺｸﾁｶ Idｵ・

	DWORD	dwFWVersion;				// ﾆﾟｿｾ・ｹ・
	DWORD	dwFWSubId;					// ﾆﾟｿｾ・ｺｸﾁｶ Id

	DWORD	dwAvailableDevices;			// ｺｸｵ・ｳｻｿ｡ｼｭﾀﾇ ｻ鄙・ｰ｡ｴﾉﾇﾑ ﾀ蠧｡ｵ・ EnumDeviceType ﾂ・ｶ.

	DWORD	dwBroadcastSystems;			// ｹ貍ﾛ ｽﾃｽｺﾅﾛ ﾁ・ﾇﾃｷ｡ｱﾗ, EnumBroadcastSystemFlag ﾂ・ｶ.
	DWORD	dwAuxBroadcastSystems;		// ｺｸﾁｶ ｹ貍ﾛ ｽﾃｽｺﾅﾛ ﾁ・ﾇﾃｷ｡ｱﾗ, ｹﾌﾁ・

#ifdef	ANYSEE_PROPERTY_x64				// ｵ蠍ﾌｹ｡ 64ｺｮｷﾎ ﾄﾄﾆﾄﾀﾏｵﾇｰ・ ｾ鏞ﾃｸｮﾄﾉﾀﾌｼﾇﾀﾌ 32ｺｮｷﾎ ﾄﾄﾆﾄﾀﾏｵﾈ ｰ豼・｡ ｻ鄙・ﾈｴﾙ.
	QWORD	pBoardInfoEx;
#else
	PVOID	pBoardInfoEx;				// ﾈｮﾀ・ｺｸｵ・ﾁ､ｺｸ, ｹ・｡ ｵ郞・ｴﾙｸ｣ｴﾙ. ﾇ邏ﾂ ｹﾌﾁ・ 
#endif

}AMTBOARD_INFO, *PAMTBOARD_INFO;

#ifndef MAX_AMTMEM_FILE_PATH
	#define MAX_AMTMEM_FILE_PATH          260
#endif

typedef struct _AMTMEM_INFO_
{
	PVOID	pMem;							// ｸﾞｸｮ ｹ・
	char	File[MAX_AMTMEM_FILE_PATH];		// ｸﾞｸｮ ﾇﾒｴ鄲ｻ ｿ菘ｻﾇﾑ ﾆﾄﾀﾏｸ・
	DWORD	dwLines;						// ﾄﾚｵ・ﾀｧﾄ｡
	DWORD	dwSize;							// ｸﾞｸｮ ｿ・ｮ
}AMTMEM_INFO, *PAMTMEM_INFO;

typedef struct _AMTPARAMS_
{
	PVOID	pBuffer;				// ｹﾛ ﾆﾎﾅﾍ
	DWORD	dwBufferSize;			// ｹﾛ ﾅｩｱ・
	DWORD	dwUsed;					// ｻ鄙・ﾈ ｱ貘ﾌ
}AMTPARAMS, *PAMTPARAMS;

#define MAX_AMTCLOCK_NAME_LENGTH	32
typedef struct _AMTCLOCK_
{
	LONG	volatile Locks;
	LONG	Id;
	DWORD	dwWaitTime_100ns;
	CHAR	Name[ MAX_AMTCLOCK_NAME_LENGTH ];
}AMTCLOCK, *PAMTCLOCK;

///////////////////////////////////////////////////////////////////////////////////////////////

#define AMT_CACHE_ALIGN(_Lines_) __declspec(align(_Lines_))
#define AMT_ABS(X) ((X)<0 ? (-X) : (X))
#define AMT_MAX(X,Y) ((X)>=(Y) ? (X) : (Y))
#define AMT_MIN(X,Y) ((X)<=(Y) ? (X) : (Y)) 
#define AMT_INRANGE(X,Y,Z) (((X<=Y) && (Y<=Z))||((Z<=Y) && (Y<=X)) ? 1 : 0)  
#define	AMT_PERCENTSCALE(X,Y,Z) (( Z >= X ) ? ( ( Z - X ) * Y ) / 100 : ( ( X - Z ) * Y ) / 100 );

#define	GET_DIFFERENCE_DWORD( _X_, _Y_ ) (( (_X_) >= (_Y_) ) ? (_X_) - (_Y_) : ((DWORD)0xFFFFFFFF - (_Y_)) + (_X_) + 1)

#define	IS_INPUT_ENDPOINT(_EPAddress_)	(((_EPAddress_)&0x80) ? TRUE : FALSE)
#define	IS_READMODE_I2C(_I2CAddress_)	(((_I2CAddress_)&0x01) ? TRUE : FALSE)

#define	AMT_WORD2BYTE(_word_, _byte_, _LSBFirst_ )	if( _LSBFirst_ ){	\
	_byte_[0] = (BYTE)(_word_&0x0FF);			\
	_byte_[1] = (BYTE)((_word_>>8)&0x0FF);		\
}else{											\
	_byte_[1] = (BYTE)(_word_&0x0FF);			\
	_byte_[0] = (BYTE)((_word_>>8)&0x0FF); }
											
#define	AMT_DWORD2BYTE(_dword_, _byte_,_LSBFirst_)	if( _LSBFirst_ ){	\
	_byte_[0] = (BYTE)(_dword_&0x0FF);			\
	_byte_[1] = (BYTE)((_dword_>>8)&0x0FF);		\
	_byte_[2] = (BYTE)((_dword_>>16)&0x0FF);	\
	_byte_[3] = (BYTE)((_dword_>>24)&0x0FF);	\
}else{											\
	_byte_[3] = (BYTE)(_dword_&0x0FF);			\
	_byte_[2] = (BYTE)((_dword_>>8)&0x0FF);		\
	_byte_[1] = (BYTE)((_dword_>>16)&0x0FF);	\
	_byte_[0] = (BYTE)((_dword_>>24)&0x0FF); }


#define	AMT_BYTE2WORD(_byte_, _word_,_LSBFirst_) _word_ = _LSBFirst_ ? ( (WORD)_byte_[1]<<8 ) | _byte_[0] : ( (WORD)_byte_[0]<<8 ) | _byte_[1]
#define	AMT_BYTE2DWORD(_byte_, _dword_,_LSBFirst_) _dword_ = _LSBFirst_ ? ( (DWORD)_byte_[3]<<24 ) | ( (DWORD)_byte_[2]<<16 ) | ( (DWORD)_byte_[1]<<8 ) | _byte_[0] : ( (DWORD)_byte_[0]<<24 ) | ( (DWORD)_byte_[1]<<16 ) | ( (DWORD)_byte_[2]<<8 ) | _byte_[3]

#define AMT_IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))

#define	AMT_IsRisingEdge( _Oldbit_, _Curbit_ )  ( !_Oldbit_ && _Curbit_ )
#define	AMT_IsFallingEdge( _Oldbit_, _Curbit_ )  ( _Oldbit_ && !_Curbit_ )

///////////////////////////////////////////////////////////////////////////////////////////////

#endif  // #ifndef		__AMTCOMMON1_DEF_H__
