
/* 
云旗鷹叙恬葎僥楼冩梢岻朕議聞喘,萩艇噐24弌扮坪徭状繍凪評茅,萩齢哘喘噐斌匍試強賜凪万哺旋來試強嶄・ 
倦夸朔惚徭減・ 
*/ 
 
 
/****************************************************** 
 * 猟周兆・conax.c 
 * 孔  嬬・侃尖CCA凋綜 
 * 恬  宀・ 
 * 晩  豚・ 
 *****************************************************/ 
 
#include "conax.h" 
 
Conax_Info_t* pstConaxInfo = NULL; 
 
 
/*   TAG 協吶　*/ 
#define ASCII_TXT_TAG			0x01 
#define OCTET_TAG				0x20 
#define TIME_TAG				0x30 
#define PRODUCT_ID_TAG			0x46 
#define PRICE_TAG				0x47 
#define EMM_ADDRESS_TAG			0x23 
 
 
/* 凋綜歌方PI */ 
#define CASS_VER_PI				0x20 
#define CA_SYS_ID_PI			0x28 
#define COUNTRY_INDICATOR_PI	0x2F 
#define MAT_RAT_LEVEL_PI		0x30 
#define SESSION_INFO_PI			0x23 
#define CA_DESC_EMM_PI			0x22 
#define HOST_DATA_PI			0x80 
#define ACCESS_STATUS_PI		0x31 
#define CW_PI					0x25 
#define CARD_NUMBER_PI			0x74 
 
 
typedef struct  
{ 
	U16		cw_id;					/* CW ID (not used) */ 
	U8		odd_even_indicator;		/* ODD EVEN Indicator */ 
	U8		aucCWBuf[8];			/* CW buf */ 
	U8		iCWLen;					/* CW length */ 
}Conax_CW_t;						/* CW歌方 */ 
 
typedef struct 
{ 
	unsigned not_sufficient_info	: 1; 
	unsigned access_granted			: 1; 
	unsigned order_ppv_event		: 1; 
	unsigned geographical_blackout	: 1; 
	unsigned maturity_rating		: 1; 
	unsigned accept_viewing			: 1; 
	unsigned ppv_preview			: 1; 
	unsigned ppv_expried			: 1; 
	unsigned no_access_to_network	: 1; 
	U8		 current_mat_rat_value; 
	U16		 minutes_left; 
	U8		 product_ref[6]; 
}Conax_AccessStatus_t;				/* Conax俊辺彜蓑歌方(PI=0x31) */ 
 
 
static Smart_ErrorCode_t ConaxRead(Smart_Handle_t  Handle, 
							     U8*		   Buffer, 
							     U16		   NumberToRead, 
							     U16*		   Read, 
							     U8*		   Status, 
							     U8			   SID); 
 static bool ConaxInitCASS(Smart_Handle_t Handle); 
 static bool ConaxResetSESS(Smart_Handle_t Handle, U8 sid); 
 static bool ConaxParseResp(Smart_Handle_t Handle,U8* Response,U16 len); 
 static bool ConaxParseCADescEMM(Smart_Handle_t Handle,U8* pBuf); 
 static bool ConaxParseAccessStatus(Smart_Handle_t Handle,U8* Response); 
 static bool ConaxReqCardNumber(Smart_Handle_t Handle); 
 static bool ConaxCAStatusSelect(Smart_Handle_t   Handle, 
 						 U8				  type, 
 						 U8*			  buf); 
 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・conax_init(Smart_Handle_t Handle)                    */ 
/*    痕方孔嬬・Conax CA兜兵晒痕方                                   */ 
/*    歌    方・                                                     */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
bool conax_init(Smart_Handle_t Handle) 
{ 
	pstConaxInfo = &conax; 
	memset(pstConaxInfo,0,sizeof(Conax_Info_t)); 
 
	if(!ConaxInitCASS(Handle)) 
	{ 
		printf("Conax Init : Init CASS failed !!\n"); 
		return false; 
	} 
   
	if(!ConaxResetSESS(Handle,0)) 
	{ 
		printf("Conax Init : Reset SESS failed !!\n"); 
		return false; 
	} 
 
 	if(!ConaxCAStatusSelect(Handle,0x00,NULL)) 
 	{ 
 		printf("Conax Init : Select Status 0 failed !!\n"); 
 		return false; 
 	} 
  
 	if(!ConaxCAStatusSelect(Handle,0x01,NULL)) 
 	{ 
 		printf("Conax Init : Select Status 1 failed !!\n"); 
 		return false; 
 	} 
 
	if(!ConaxReqCardNumber(Handle)) 
	{ 
		printf("Conax Init : Read Card Number failed !!\n"); 
		return false; 
	} 
 
	printf("Conax CA init OK!!!!!!!!!!!!\n"); 
	return true; 
} 
 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxRead                                            */ 
/*    痕方孔嬬・Conax CA兜兵晒痕方                                   */ 
/*    歌    方・Handle: SmartCard鞘凹                                */ 
/*              NumberToRead: 勣響議忖准倖方                         */ 
/*              Buffer: 贋慧侭響函方象議産喝曝                       */ 
/*              Read: 糞縞響函議方象倖方                             */ 
/*              Status: 荷恬彜蓑                                     */ 
/*              SID: 氏三催                                          */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static Smart_ErrorCode_t ConaxRead(Smart_Handle_t   Handle, 
							    U8*				Buffer, 
							    U16				NumberToRead, 
							    U16*			Read, 
							    U8*				Status, 
							    U8				SID) 
{ 
	U8 ins[] = {0xDD,0xCA,0x00,0xFF,0xFF};		/* ISO part of Read Command */ 
	Smart_ErrorCode_t error; 
 
	ins[3] = SID;								/* Set session ID */ 
	ins[4] = (U8)NumberToRead;						/* Set param of the read command */ 
	error = Smart_Transfer(Handle,			/* Send the command to the card */ 
						 ins, 
						 5, 
						 Buffer, 
						 0, 
						 Read, 
						 Status); 
 
	return error; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxWrite                                           */ 
/*    痕方孔嬬・Conax CA窟僕凋綜痕方                                 */ 
/*    歌    方・Handle: SmartCard鞘凹                                */ 
/*              Buffer: 勣窟僕議凋綜                                 */ 
/*              NumberToWrite: 凋綜海業                              */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxWrite(Smart_Handle_t   Handle, 
							      U8*			   Buffer, 
							      U16			   NumberToWrite) 
{ 
	Smart_ErrorCode_t  error; 
	U8				 Status[2]; 
	U8				 Response[255]; 
	U16				 Read; 
	bool             tmpresult; 
 
	error = Smart_Transfer(Handle, 
						   Buffer, 
						   NumberToWrite, 
						   Response, 
						   0, 
						   &Read, 
						   Status); 
	if(error == SMC_NO_ERROR) 
	{ 
		/* We must do different things according to different Procedure bytes */ 
		switch(Status[0])  
		{ 
			case 0x90: 
				switch(Status[1]) 
				{ 
					case 0x00: 
 
						tmpresult = true; 
						break; 
					 
					default: 
 
						tmpresult = false; 
						break; 
				} 
				break; 
			case 0x98: 
 
				/* We must read the data provide by the card */ 
				error = ConaxRead(Handle, 
								  Response, 
								  Status[1], 
								  &Read, 
								  Status, 
								  0); 
				 
				/* We have to know what data we have got */ 
				if(error == SMC_NO_ERROR) 
				{ 
					if(ConaxParseResp(Handle,Response,Read)) 
						tmpresult = false; 
					else 
					{ 
						printf("Conax Parse Response Failed !!\n"); 
						tmpresult = false; 
					} 
				} 
				else 
				{ 
					tmpresult = false; 
				} 
				break; 
			default: 
				   tmpresult = false; 
				break; 
		} 
	} 
	 
	return tmpresult; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxParseResp                                       */ 
/*    痕方孔嬬・蛍裂Conax触議・哘方象                                */ 
/*    歌    方・Response: ・哘方象遊峺寞                             */ 
/*              len:　　　・哘方象海業　　　　　　　　　　　　　　　 */ 
/*    卦 指 峙・撹孔・true                                           */ 
/*              払移・false                                          */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxParseResp(Smart_Handle_t Handle,U8* Response,U16 len) 
{ 
	U8* buf; 
	U8	pi; 
	int iLength = 0; 
	U16 iTmpLen = 0; 
	U16 iOctLen = 0; 
	int iParsedLength = 0; 
	Conax_CW_t	ConaxCW; 
 
	buf = Response; 
 
	while(iParsedLength < len) 
	{ 
		pi = buf[0]; 
		iLength = buf[1]; 
 
		/* Each data have a pi parameter,we use this to identify them */ 
		switch(pi)  
		{ 
			/* The following five objects can be received after INIT_CASS command */ 
			case CASS_VER_PI: 
 
				if(iLength != 1) 
					return false; 
				pstConaxInfo->stParam.Version = buf[2]; 
				break; 
			case CA_SYS_ID_PI: 
 
				if(iLength != 2) 
					return false; 
				pstConaxInfo->stParam.CaSysId = (buf[2] << 8) | buf[3]; 
				break; 
			case COUNTRY_INDICATOR_PI: 
 
				if(iLength != 2) 
					return false; 
				pstConaxInfo->stParam.CountryIndicatorValue = (buf[2] << 8) | buf[3]; 
				break; 
			case MAT_RAT_LEVEL_PI: 
 
				if(iLength != 1) 
					return false; 
				pstConaxInfo->stParam.MatRat = buf[2]; 
				break; 
			case SESSION_INFO_PI: 
 
				if(iLength != 1) 
					return false; 
				pstConaxInfo->stParam.MaxSessionNum = buf[2]; 
				break; 
			 
			/* This Object can be received after EMM or ECM send */ 
			case HOST_DATA_PI: 
				break; 
 
			/* This Object can be received after CAT send */ 
			case CA_DESC_EMM_PI: 
 
				if(!ConaxParseCADescEMM(Handle,buf)) 
					return false; 
				break; 
			 
			/* The following four objects can be received after ECM send */ 
			case ACCESS_STATUS_PI: 
 
				if(!ConaxParseAccessStatus(Handle,buf)) 
					return false; 
				break; 
			case CW_PI: 
 
				if(iLength < 5) 
					return false; 
				memset(&ConaxCW,0,sizeof(Conax_CW_t)); 
				ConaxCW.cw_id = (buf[2] << 8) | buf[3]; 
				ConaxCW.odd_even_indicator = buf[4]; 
				ConaxCW.iCWLen = iLength-5; 
				{ 
					memset(ConaxCW.aucCWBuf,0,ConaxCW.iCWLen); 
					memcpy(ConaxCW.aucCWBuf,buf+7,ConaxCW.iCWLen); 
				} 
 
				break; 
				 
			/* This object can be received when Req Card Number Command is send */ 
			case CARD_NUMBER_PI: 
			if(iLength != 4) 
					return false; 
				pstConaxInfo->iCardNumber = (Response[2] << 24) | (Response[3] << 16) | 
											(Response[4] << 8) | Response[5]; 
				break; 
			 
			default: 
				break; 
		} 
		iParsedLength += (iLength+2); 
		buf += (iLength + 2); 
	} 
	return true; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxInitCASS(void)                                  */ 
/*    痕方孔嬬・INIT_CASS凋綜                                        */ 
/*    歌    方・Handle: SmartCard鞘凹                                */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxInitCASS(Smart_Handle_t Handle) 
{ 
	U8 ins[] = {0xDD,0x26,0x00,0x00,0x03,0x10,0x01,0x40}; 
	bool error; 
	error = ConaxWrite(Handle,ins,8); 
	return error; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxResetSESS(void)                                 */ 
/*    痕方孔嬬・RESET_SESSION凋綜                                    */ 
/*    歌    方・Handle: SmartCard鞘凹                                */ 
/*              sid:    勣鹸了議氏三催                               */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxResetSESS(Smart_Handle_t Handle, 
									 U8               sid) 
{ 
	U8 ins[] = {0xDD,0x26,0x00,0x00,0x03,0x69,0x01,0xFF}; 
	bool error; 
	ins [7] = sid; 
	error = ConaxWrite(Handle,ins,8); 
	return error; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxInitOAA                                         */ 
/*    痕方孔嬬・侃尖 INITOAA凋綜　　　                               */ 
/*    歌    方・Handle: SmartCard鞘凹　　　　　　　　　　　　　　　  */ 
/*              sid   : 氏三催　　　　　　　　　　　　　　　　　　　 */ 
/*              pBuf  : Cat燕遊峺寞　　　　　　　　　　　　　　　　　*/ 
/*              len   : Cat方象海業　　　　　　　　　　　　　　　　　*/ 
/*    卦 指 峙・撹孔・true                                           */ 
/*              払移・false                                          */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxInitOAA(Smart_Handle_t Handle, 
							U8				sid, 
							U8*			    pBuf, 
							U16			    len) 
{ 
	U8			   ins[] = {0xDD,0x82,0x00,0xFF,0xFF}; 
	U8			   Command[255]; 
	U16			   NumberToWrite = 0; 
	bool error; 
 
	ins[3] = sid; 
	ins[4] = (U8)len; 
	NumberToWrite = len+5; 
	memset(Command,0,len+5); 
	memcpy(Command,ins,5); 
	memcpy(Command+5,pBuf,len); 
	error = ConaxWrite(Handle,Command,NumberToWrite); 
	 
	return error; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・conax_parse_cat                                      */ 
/*    痕方孔嬬・侃尖Cat燕　　　 　　　                               */ 
/*    歌    方・pBuf  : Cat燕遊峺寞　　　　　　　　　　　　　　　　　*/ 
/*              len   : Cat方象海業　　　　　　　　　　　　　　　　　*/ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・勣箔CAT燕嶄CA Descriptor葎Conax狼由・拝根嗤CRC丕刮鷹 */ 
/*-------------------------------------------------------------------*/ 
void conax_parse_cat(Smart_Handle_t Handle,U8* pBuf,U16 len) 
{ 
	U8 aucCatSection[300]; 
	memset(aucCatSection,0,300); 
	aucCatSection[0] = 0x11; 
	aucCatSection[1] = (U8)len; 
	memcpy(aucCatSection+2,pBuf,len); 
	ConaxInitOAA(Handle,0,aucCatSection,len+2); 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxParseCADescEMM                                  */ 
/*    痕方孔嬬・侃尖CA_DESC_EMM 　　　                               */ 
/*    歌    方・Response: 方象遊峺寞　　　　　　　　　　　　　　　　 */ 
/*    卦 指 峙・撹孔・true                                           */ 
/*              払移・fasle　                                        */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxParseCADescEMM(Smart_Handle_t Handle,U8* Response) 
{ 
	int i; 
	U8* pBuf; 
	int length; 
	int iAddressLength; 
	int iDescLength; 
	int iParsedAddLen; 
	U8  EMM_Address_TABLE[10][7]; 
	int EMM_Address_Count; 
	pBuf = Response; 
	length = pBuf[1]; 
	iDescLength = pBuf[3]; 
	iAddressLength = length - iDescLength - 2; 
	pBuf += (iDescLength+4); 
	EMM_Address_Count = 0; 
	iParsedAddLen = 0; 
	while(iParsedAddLen < iAddressLength) 
	{ 
		if(pBuf[0] == EMM_ADDRESS_TAG && pBuf[1] == 7) 
		{ 
			memcpy(EMM_Address_TABLE[EMM_Address_Count],pBuf+2,7); 
			EMM_Address_Count++; 
			iParsedAddLen += 9; 
			pBuf += 9; 
		} 
		else 
		{ 
			return false; 
		} 
	} 
 
	pstConaxInfo->iEMMAddressCount = EMM_Address_Count; 
	for(i=0;i<EMM_Address_Count;i++) 
		memcpy(pstConaxInfo->auEMMAddress[i],EMM_Address_TABLE[i],7); 
	if(EMM_Address_Count < 1) 
		return false; 
	return true; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxEmmCOM                                          */ 
/*    痕方孔嬬・侃尖 EMM_COM凋綜　　　                               */ 
/*    歌    方・Handle: SmartCard鞘凹　　　　　　　　　　　　　　　  */ 
/*              sid   : 氏三催　　　　　　　　　　　　　　　　　　　 */ 
/*              pBuf  : EMM燕遊峺寞　　　　　　　　　　　　　　　　　*/ 
/*              len   : EMM方象海業　　　　　　　　　　　　　　　　　*/ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxEmmCOM(Smart_Handle_t  Handle, 
								  U8				sid, 
								  U8*			    pBuf, 
								  U16			    len) 
{ 
	U8			   ins[] = {0xDD,0x84,0x00,0xFF,0xFF}; 
	U8			   Command[300]; 
	U16			   NumberToWrite = 0; 
	bool           error; 
 
	if(len > 290) 
	{ 
		printf("ConaxEMMCom Error : Message too long !!\n"); 
		return false; 
	} 
 
	ins[3] = sid; 
	ins[4] = (U8)len; 
	NumberToWrite = len+5; 
	memset(Command,0,sizeof(Command)); 
	memcpy(Command,ins,5); 
	memcpy(Command+5,pBuf,len); 
	error = ConaxWrite(Handle,Command,NumberToWrite); 
	return error; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・conax_parse_emm                                        */ 
/*    痕方孔嬬・侃尖EMM方象　　 　　　                               */ 
/*    歌    方・pBuf  : EMM燕遊峺寞　　　　　　　　　　　　　　　　　*/ 
/*              len   : EMM方象海業　　　　　　　　　　　　　　　　　*/ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
bool conax_parse_emm(Smart_Handle_t Handle,U8* pBuf,U16 len) 
{ 
	U8 aucEmmSection[255]; 
	aucEmmSection[0] = 0x12; 
	aucEmmSection[1] = (U8)len; 
	memcpy(aucEmmSection+2,pBuf,len); 
	ConaxEmmCOM(Handle,0,aucEmmSection,len+2); 
	return true; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxEcmCOM                                          */ 
/*    痕方孔嬬・侃尖 ECM_COM凋綜　　　                               */ 
/*    歌    方・Handle: SmartCard鞘凹　　　　　　　　　　　　　　　  */ 
/*              sid   : 氏三催　　　　　　　　　　　　　　　　　　　 */ 
/*              pBuf  : ECM燕遊峺寞　　　　　　　　　　　　　　　　　*/ 
/*              len   : ECM方象海業　　　　　　　　　　　　　　　　　*/ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxEcmCOM(Smart_Handle_t Handle, 
						   U8			  sid, 
						   U8*			  pBuf, 
						   U16			  len, 
						   U8*			  pucCWBuffer) 
{ 
	U8			   ins[] = {0xDD,0xA2,0x00,0xFF,0xFF}; 
	U8			   Command[300]; 
	U16			   NumberToWrite = 0; 
	U8			   Response[255]; 
	U16			   Read; 
	U8			   Status[2]; 
	Smart_ErrorCode_t error; 
	int				iParsedLength; 
	bool			bRet = false; 
	U8				pi; 
	int				iLength; 
	U8*				buf = Response; 
	ins[3] = sid; 
	ins[4] = (U8)len; 
	NumberToWrite = len+5; 
	if(len+5 > 290) 
	{ 
		printf("Conax Ecm Com Error : Ecm Message too long !!\n"); 
		return false; 
	} 
	memset(Command,0,len+5); 
	memcpy(Command,ins,5); 
	memcpy(Command+5,pBuf,len); 
	 
	error = Smart_Transfer(Handle,			/* Send the command to the card */ 
						 Command, 
						 NumberToWrite, 
						 Response, 
						 0, 
						 &Read, 
						 Status); 
	if(error == SMC_NO_ERROR) 
	{ 
		/* We must do different things according to diffent Procedure bytes */ 
		switch(Status[0])  
		{ 
			case 0x98: 
				/* We must read the data provide by the card */ 
				error = ConaxRead(Handle, 
								  Response, 
								  Status[1], 
								  &Read, 
								  Status, 
								  0); 
				 
				/* We have to know what data we have got */ 
				if(error == SMC_NO_ERROR) 
				{ 
					iParsedLength = 0; 
					while(iParsedLength < Read) 
					{ 
						pi = buf[0]; 
						iLength = buf[1]; 
						/* Each data have a pi parameter,we use this to identify them */ 
						switch(pi)  
						{ 
							/* The following four objects can be received after ECM send */ 
							case CW_PI: 
								if(iLength < 5) 
									return false; 
								if(buf[4]==0x00) 
								{ 
									memcpy(pucCWBuffer,buf+7,8); 
								} 
								else 
								{ 
									memcpy(pucCWBuffer+8,buf+7,8); 
								} 
								bRet = true; 
								break; 
							case ACCESS_STATUS_PI: 
 
							if(!ConaxParseAccessStatus(0,buf)) 
								bRet = false; 
							else 
								bRet = true; 
							break; 
								 
							default: 
								break; 
						} 
						iParsedLength += (iLength+2); 
						buf += (iLength + 2); 
					} 
				} 
				break; 
			default: 
				break; 
			} 
	}	 
	 
	return bRet; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・conax_parse_ecm                                      */ 
/*    痕方孔嬬・侃尖ECM方象　　 　　　                               */ 
/*    歌    方・pbuf  : ECM燕遊峺寞　　　　　　　　　　　　　　　　　*/ 
/*              pucCW : CW  　　　 　　　　　　　　　　　　　　      */ 
/*    卦 指 峙・撹孔:true , 払移 : false                             */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・廣吭ECM方象議鯉塀                                    */ 
/*-------------------------------------------------------------------*/ 
bool conax_parse_ecm(Smart_Handle_t Handle,U8* pbuf,U8* pucCW) 
{ 
	U8 aucEcmSection[300]; 
	int len=((pbuf[1]&0x0F)<<8)+pucCW[2]+7; 
	memset(aucEcmSection,0,sizeof(aucEcmSection)); 
	aucEcmSection[0] = 0x14; 
	aucEcmSection[1] = len + 1; 
	aucEcmSection[2] = 0;			    /* Pay attention to ECM mode indicate bit! */ 
	memcpy(aucEcmSection+3,pbuf,len);    /* Copy the ECM data to the command */ 
	return ConaxEcmCOM(Handle,0,aucEcmSection,len+3,pucCW); 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxParseAccessStatus                               */ 
/*    痕方孔嬬・侃尖ACCESS_STATUS 　　                               */ 
/*    歌    方・Response: 方象遊峺寞　　　　　　　　　　　　　　　　 */ 
/*    卦 指 峙・撹孔・true                                           */ 
/*              払移・fasle　                                        */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxParseAccessStatus(Smart_Handle_t Handle,U8* Response) 
{ 
	U8* pBuf; 
	int len; 
	int expectedlength = 2; 
	Conax_AccessStatus_t ConaxAccessStatus; 
	len = Response[1]; 
 
	if(len < 2) 
		return false; 
	if(len == 2) 
	{ 
		if((Response[2]==0x00) && (Response[3]==0x00)) 
			return false; 
	} 
	 
	pBuf = Response + 2; 
	ConaxAccessStatus.not_sufficient_info	= (pBuf[0] & 0x80) >> 7; 
	ConaxAccessStatus.access_granted	    = (pBuf[0] & 0x40) >> 6; 
	ConaxAccessStatus.order_ppv_event       = (pBuf[0] & 0x20) >> 5; 
	ConaxAccessStatus.geographical_blackout = (pBuf[0] & 0x04) >> 2; 
	ConaxAccessStatus.maturity_rating		= (pBuf[0] & 0x02) >> 1; 
	ConaxAccessStatus.accept_viewing		= pBuf[0] & 0x01; 
	ConaxAccessStatus.ppv_preview			= (pBuf[1] & 0x80) >> 7; 
	ConaxAccessStatus.ppv_expried			= (pBuf[1] & 0x40) >> 6; 
	ConaxAccessStatus.no_access_to_network	= (pBuf[1] & 0x20) >> 5; 
 
	pBuf += 2; 
	if(ConaxAccessStatus.maturity_rating) 
	{ 
		ConaxAccessStatus.current_mat_rat_value = pBuf[0]; 
		pBuf++; 
		expectedlength++; 
	} 
	if(ConaxAccessStatus.accept_viewing) 
	{ 
		ConaxAccessStatus.minutes_left = (pBuf[0] << 8) | pBuf[1]; 
		pBuf += 2; 
		expectedlength += 2; 
	} 
	if(ConaxAccessStatus.order_ppv_event ||  
	   ConaxAccessStatus.accept_viewing) 
	{ 
		memcpy(ConaxAccessStatus.product_ref,pBuf,6); 
		expectedlength += 6; 
	} 
 
	if(len < expectedlength) 
		return false; 
	return true; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxCAStatusCOM                                     */ 
/*    痕方孔嬬・侃尖CA_STATUS_COM凋綜                                */ 
/*    歌    方・Handle: SmartCard鞘凹　　　　　　　　　　　　　　　  */ 
/*              sid   : 氏三催　　　　　　　　　　　　　　　　　　　 */ 
/*              pBuf  : 凋綜遊峺寞　  　　　　　　　　　　　　　　　 */ 
/*              len   : 凋綜海業　　　　　　　　　　　　　　　　　   */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool ConaxCAStatusCom(Smart_Handle_t Handle, 
									   U8				sid, 
									   U8*			    pBuf, 
									   U16			    len) 
{ 
	bool error; 
	U8 ins[] = {0xDD,0xC6,0x00,0xFF,0xFF}; 
	U8 Command[255]; 
 
	ins[3] = sid; 
	ins[4] = (U8)len; 
	memcpy(Command,ins,5); 
	memcpy(Command+5,pBuf,len); 
	error = ConaxWrite(Handle,Command,len+5); 
 
	return error; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxCAStatusSelect                                  */ 
/*    痕方孔嬬・侃尖CA Status Select凋綜                             */ 
/*    歌    方・type  : CA_Status_Type                               */ 
/*                      0x00: Retrieve entire table of subscription  */ 
/*                            status                                 */ 
/*                      0x01: Retrieve entire table of PPV event     */ 
/*                            status                                 */ 
/*                      0x05: Retrieve subscription status for a     */ 
/*                            specified subscription reference       */ 
/*                      0x06: Retrieve PPV event status for a        */ 
/*                            specified event tag                    */ 
/*              buf  : When type=0x05 this is subscription_ref       */ 
/*                     When type=0x06 this is event_tag              */ 
/*                     Leave this NULL otherwise                     */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
bool ConaxCAStatusSelect(Smart_Handle_t   Handle, 
						 U8				  type, 
						 U8*			  buf) 
{ 
	U8 Command[100]; 
	U8 len; 
 
	if(type == 0x05) 
	{ 
		if(buf == NULL) 
		{ 
			printf("ConaxCAStatusSelect : buf should not be NULL !\n"); 
			return false; 
		} 
		len = 5; 
		memset(Command,0,5); 
		Command[1] = 3; 
		memcpy(Command+3,buf,2); 
	} 
	else if(type == 0x06) 
	{ 
		if(buf == NULL) 
		{ 
			printf("ConaxCAStatusSelect : buf should not be NULL !\n"); 
			return false; 
		} 
		len = 6; 
		memset(Command,0,6); 
		Command[1] = 4; 
		memcpy(Command+3,buf,3); 
	} 
	else 
	{ 
		len = 3; 
		memset(Command,0,3); 
		Command[1] = 1; 
	} 
	Command[0] = 0x1C; 
	Command[2] = type; 
	return(ConaxCAStatusCom(Handle,0,Command,len)); 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxSecurityOp                                      */ 
/*    痕方孔嬬・Conax Security OP 凋綜                               */ 
/*    歌    方・Handle: 触鞘凹                                       */ 
/*              sid   :  氏三催                                      */ 
/*              pBuf  :  凋綜遊峺寞                                  */ 
/*              len   :  凋綜海業                                    */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static Smart_ErrorCode_t ConaxSecurityOp(Smart_Handle_t  Handle, 
								      U8				sid, 
								      U8*			    pBuf, 
								      U16			    len) 
{ 
	U8 ins[] = {0xDD,0xC2,0x00,0xFF,0xFF}; 
	U8 Command[50]; 
 	Smart_ErrorCode_t error; 
	ins[3] = sid; 
	ins[4] = (U8)len; 
	memset(Command,0,len+5); 
	memcpy(Command,ins,5); 
	memcpy(Command+5,pBuf,len); 
	error = ConaxWrite(Handle,Command,len+5); 
	return error; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・ConaxReqCardNumber                                   */ 
/*    痕方孔嬬・響函触催凋綜                                         */ 
/*    歌    方・                                                     */ 
/*    卦 指 峙・撹孔・true                                           */ 
/*              払移・fasle                                          */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
bool ConaxReqCardNumber(Smart_Handle_t Handle) 
{ 
	U8 ins[2] = {0x66,0x00}; 
 
	return ConaxSecurityOp(Handle,0,ins,2) != 0; 
} 
 
 
////////////////////////The end-------------