
/* 
云旗鷹叙恬葎僥楼冩梢岻朕議聞喘,萩艇噐24弌扮坪徭状繍凪評茅,萩齢哘喘噐斌匍試強賜凪万哺旋來試強嶄・ 
倦夸朔惚徭減・ 
*/ 
 
 
/****************************************************** 
 * 猟周兆・viaccess.c 
 * 孔  嬬・侃尖VCA凋綜 
 * 恬  宀・ 
 * 晩  豚・ 
 *****************************************************/ 
 
 
 
#include "viaccess.h" 
 
static unsigned char aucViaEMMBuf8E[188]; 
static unsigned char aucViaEMMBuf8C[188]; 
 
static Viaccess_Info_t* pstViaInfo; 
 
 
 
static Smart_ErrorCode_t via_transfer(Smart_Handle_t Handle, 
								   U8*			    ins, 
								   U16			   NumberToWrite, 
								   U8*			   Response, 
								   U16*			   Read, 
								   U8* Status); 
static bool via_get_ppua(Smart_Handle_t Handle); 
static bool via_get_sn(Smart_Handle_t Handle); 
static bool via_get_prov(Smart_Handle_t Handle); 
static Smart_ErrorCode_t via_get_sa(Smart_Handle_t Handle); 
static int via_get_cw(Smart_Handle_t Handle, 
  			         U8*			  pECM, 
			         U8*            pCW  ); 
static bool via_check_status(U8* pb); 
 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_init                                             */ 
/*    痕方孔嬬・viaccess兜兵晒痕方                                   */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・                                                     */ 
/*-------------------------------------------------------------------*/ 
bool via_init(Smart_Handle_t Handle) 
{ 
	pstViaInfo = &viaccess; 
	memset(pstViaInfo,0,sizeof(Viaccess_Info_t)); 
 
	if(!via_get_ppua(Handle)) 
	{ 
		printf("via init : get ppua failed !!\n"); 
		return false; 
	} 
 
	/* 響函猟周佚連(嗤謹富倖猟周,茅麼猟周岻翌耽倖猟周斤哘匯倖捲暦斌) */ 
	if(!via_get_prov(Handle)) 
	{ 
		printf("via init : get prog info failed !!\n"); 
		 
		return false; 
	} 
 
 
	if(!via_get_sn(Handle)) 
	{ 
		printf("via init : get sn failed !!\n"); 
		 
		//return false; 
	} 
	 
 
	printf("Viaccess Init OK !!\n"); 
 
	return true; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_master_file                                      */ 
/*    痕方孔嬬・viaccess痕方・僉夲麼猟周                             */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              Response: ・哘方象産喝曝                             */ 
/*              NumberToRead: 錬李響函誼・哘方象海業                 */ 
/*              Read: 糞縞響函議・哘方象海業                         */ 
/*              Status: 荷恬潤惚(根竃危旗鷹、狛殻忖准吉)             */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_next_file                                        */ 
/*-------------------------------------------------------------------*/ 
static bool via_master_file(Smart_Handle_t Handle, 
									  U8* Response, 
									  U16 NumberToRead, 
									  U16* Read, 
									  U8* Status) 
{ 
	Smart_ErrorCode_t error; 
	U8 ins[5] = {0xca, 0xa4, 0x00, 0x00, 0x00}; 
	pstViaInfo->iPrgIndex = 0; 
	error = via_transfer(Handle,ins,5,Response,Read,Status); 
	if(error == SMC_NO_ERROR) 
		return via_check_status(Status); 
	return false; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_next_file                                        */ 
/*    痕方孔嬬・viaccess ca僉夲和匯倖猟周凋綜                        */ 
/*    歌    方・Handle: SmartCard鞘凹                                */ 
/*              Response_p: 贋慧崘嬬触凋綜・哘議産喝曝遊峺寞         */ 
/*              NumberToRead: 錬李響欺議・哘海業                     */ 
/*              Read: 糞縞響函誼・哘方象海業                         */ 
/*              Status: 荷恬彜蓑                                     */ 
/*    卦 指 峙・撹孔:true                                            */ 
/*              払移:false                                           */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_master_file                                      */ 
/*-------------------------------------------------------------------*/ 
static bool via_next_file(Smart_Handle_t Handle, 
								   U8* Response, 
								   U16 NumberToRead, 
								   U16* Read, 
								   U8* Status) 
{ 
	Smart_ErrorCode_t error; 
	U8 ins[5] = {0xca, 0xa4, 0x02, 0x00, 0x00}; 
	pstViaInfo->iPrgIndex++; 
	error = via_transfer(Handle,ins,5,Response,Read,Status); 
	if(error == SMC_NO_ERROR) 
		return via_check_status(Status); 
	return false; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_select_file                                      */ 
/*    痕方孔嬬・猟周僉夲凋綜                                         */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              file: 猟周園催                                       */ 
/*              Response: ・哘方象産喝曝                             */ 
/*              NumberToRead: 豚李響函議方象海響                     */ 
/*              Read: 糞縞響函議方象海業                             */ 
/*              Status: 荷恬潤惚,(根危列旗鷹才狛殻忖准)              */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_master_file                                      */ 
/*              via_next_file                                        */ 
/*-------------------------------------------------------------------*/ 
static bool via_select_file(Smart_Handle_t Handle,  
									   U8* file, 
									   U8* Response, 
									   U16 NumberToRead, 
									   U16* Read, 
									   U8* Status) 
{ 
	Smart_ErrorCode_t error; 
	U8	ins[8] = {0xca, 0xa4, 0x04, 0x00, 0x03, 0xff, 0xff, 0xff}; 
	int i; 
	for(i=0;i<pstViaInfo->iPrgCount;i++) 
	{ 
		if(pstViaInfo->auPrgInfo[i][0] == file[0]) 
			if(pstViaInfo->auPrgInfo[i][1] == file[1]) 
				if(pstViaInfo->auPrgInfo[i][2] == file[2]) 
					break; 
	} 
	if(i >= pstViaInfo->iPrgCount) 
	{ 
		/* Just use 0xF0 as a tag if file not found */ 
		Status[0] = 0xF0; 
		return false; 
	} 
	if(pstViaInfo->iPrgIndex==i) 
	{ 
		Status[0] = 0xFF; 
		return true; 
	} 
	pstViaInfo->iPrgIndex = i; 
	ins[5] = file[0]; 
	ins[6] = file[1]; 
	ins[7] = file[2]; 
	error = via_transfer(Handle,ins,8,Response,Read,Status); 
	if(error == SMC_NO_ERROR) 
		return via_check_status(Status); 
	return false; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_request_data                                     */ 
/*    痕方孔嬬・響函蒙協海業議方象                                   */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              Response: ・哘方象産喝曝                             */ 
/*              NumberToRead: 豚李響函議方象海響                     */ 
/*              Read: 糞縞響函議方象海業                             */ 
/*              Status: 荷恬潤惚,(根危列旗鷹才狛殻忖准)              */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool via_request_data(Smart_Handle_t Handle, 
									   U8* Response, 
									   U16 NumberToRead, 
									   U16* Read, 
									   U8* Status) 
{ 
	Smart_ErrorCode_t error; 
	U8	ins[5] = {0xca, 0xc0, 0x00, 0x00, 0x1a}; 
	error = via_transfer(Handle, ins,5,Response,Read,Status); 
	if(error == SMC_NO_ERROR) 
		return via_check_status(Status); 
	return false; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_get_consulation                                  */ 
/*    痕方孔嬬・響函方象凋綜,宥狛乎凋綜聞喘音揖議歌方辛參貫触嶄響函  */ 
/*              音揖議佚連・泌惚凋綜屎鳩触繍卦指匯粁方象・凪嶄及2倖  */ 
/*              忖准喘噐公竃補竃方象議海業,宸乂・哘方象辛參宥狛CA B8 */ 
/*              凋綜響函・哘方象                                     */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              Response: ・哘方象産喝曝                             */ 
/*              NumberToRead: 豚李響函議方象海響                     */ 
/*              Read: 糞縞響函議方象海業                             */ 
/*              Status: 荷恬潤惚,(根危列旗鷹才狛殻忖准)              */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_consulation_length                               */ 
/*              via_consulation_answer                               */ 
/*    凋綜傍苧・CA AC A5 00 00: 燕幣響函PPUA                         */ 
/*              CA AC A4 00 00: 燕幣響函触催                         */ 
/*              CA AC A7 00 00: 響函娩幡佚連                         */ 
/*              CA AC 06 00 00: 響函定槍吉雫                         */ 
/*              CA AC A9 00 04: 響函娩幡扮寂                         */ 
/*-------------------------------------------------------------------*/ 
static bool via_get_consultation(Smart_Handle_t Handle,  
										   U8* Response, 
										   U16 NumberToRead, 
										   U16* Read, 
										   U8* Status, 
										   U8 code,  
										   U8 len) 
{ 
	Smart_ErrorCode_t error; 
	U8	ins[5] = {0xCA, 0xAC, 0xFF, 0x00, 0xFF}; 
	ins[2] = code; 
	ins[4] = len; 
	error = via_transfer(Handle,ins,5,Response,Read,Status); 
	if(error == SMC_NO_ERROR) 
		return via_check_status(Status); 
	return false; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_consulation_length                               */ 
/*    痕方孔嬬・響函方象海業                                         */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              Response: ・哘方象産喝曝                             */ 
/*              NumberToRead: 豚李響函議方象海響                     */ 
/*              Read: 糞縞響函議方象海業                             */ 
/*              Status: 荷恬潤惚,(根危列旗鷹才狛殻忖准)              */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_get_consulation                                  */ 
/*              via_consulation_answer                               */ 
/*-------------------------------------------------------------------*/ 
static bool via_consultation_length(Smart_Handle_t Handle, 
											  U8* Response, 
											  U16 NumberToRead, 
											  U16* Read, 
											  U8* Status) 
{ 
	Smart_ErrorCode_t error; 
	U8 ins[5] = {0xca, 0xb8, 0x00, 0x00, 0x02}; 
	error = via_transfer(Handle,ins,5,Response,Read,Status); 
	if(error == SMC_NO_ERROR) 
		return via_check_status(Status); 
	return false; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_consulation_answer                               */ 
/*    痕方孔嬬・響函方象凋綜                                         */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              Response: ・哘方象産喝曝                             */ 
/*              NumberToRead: 豚李響函議方象海響                     */ 
/*              Read: 糞縞響函議方象海業                             */ 
/*              Status: 荷恬潤惚,(根危列旗鷹才狛殻忖准)              */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_consulation_length                               */ 
/*              via_consulation_answer                               */ 
/*-------------------------------------------------------------------*/ 
static bool via_consultation_answer(Smart_Handle_t Handle,  
											  U8* Response, 
											  U16 NumberToRead, 
											  U16* Read, 
											  U8* Status, 
											  U8 length) 
{ 
	Smart_ErrorCode_t error; 
	U8 ins[5] = {0xca, 0xb8, 0x00, 0x00, 0xff}; 
	ins[4] = length; 
	error = via_transfer(Handle,ins,5,Response,Read,Status); 
	if(error == SMC_NO_ERROR) 
		return via_check_status(Status); 
	return false; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_get_prov                                          */ 
/*    痕方孔嬬・響函捲暦斌佚連                                       */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_consulation_length                               */ 
/*              via_consulation_answer                               */ 
/*-------------------------------------------------------------------*/ 
static bool via_get_prov(Smart_Handle_t Handle) 
{ 
	int iProgCount=0; 
	U16 Read; 
	U8 Response[254]; 
	U8 Status[2]; 
	iProgCount = 0; 
	if(!via_master_file(Handle,Response,0,&Read,Status)) 
	{ 
		printf("via get prog info failed : select file failed !!\n"); 
		return false; 
	} 
	while(Status[1] == 0) 
	{ 
		printf("ProgCount = %d\n",pstViaInfo->iPrgCount); 
		if(via_request_data(Handle,Response,0,&Read,Status)) 
		{ 
			printf("Prog %d : %02x %02x %02x\n",iProgCount,Response[0],Response[1],Response[2]); 
			memcpy(pstViaInfo->auPrgInfo[pstViaInfo->iPrgCount],Response,3); 
			pstViaInfo->auPrgInfo[pstViaInfo->iPrgCount][2] &= 0xF0; 
			pstViaInfo->iPrgCount++; 
			iProgCount++; 
			if(pstViaInfo->iPrgCount>MAX_VIA_PROV_COUNT) 
				break; 
		} 
		else 
		{ 
			printf("via get prov info failed !!\n"); 
			return false; 
		} 
 
		if(!via_next_file(Handle,Response,0,&Read,Status)) 
		{ 
			/*printf("select next file failed !!\n");*/ 
			break; 
		} 
	} 
	if(iProgCount<=10) 
		via_get_sa(Handle); 
	printf("ProgCount = %d\n",pstViaInfo->iPrgCount); 
	return true; 
} 
 
static Smart_ErrorCode_t via_get_sa(Smart_Handle_t Handle) 
{ 
	int iProgIndex; 
	U16 Read; 
	U8 Response[254],len; 
	U8 Status[2] = {0,0}; 
	iProgIndex = 0; 
	if(!via_master_file(Handle,Response,0,&Read,Status)) 
	{ 
		printf("via get prog info failed : select file failed !!\n"); 
		return false; 
	} 
	while(Status[1] == 0) 
	{ 
		/* Read SA */ 
		if(via_get_consultation(Handle,Response,0,&Read,Status,0xA5,0x00)) 
		{ 
			if(via_consultation_length(Handle,Response,0,&Read,Status)) 
			{ 
				len = Response[1]; 
				if(via_consultation_answer(Handle,Response,0,&Read,Status,len)) 
				{ 
					printf("SA : %02x %02x %02x %02x\n",Response[0],Response[1],Response[2],Response[3]); 
					memcpy(pstViaInfo->auSA[iProgIndex],Response,4); 
					pstViaInfo->ucViaSCCustWp = Response[5]; 
				} 
			} 
		} 
		iProgIndex++; 
		if(!via_next_file(Handle,Response,0,&Read,Status)) 
			break; 
	} 
	return SMC_NO_ERROR; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_get_df_info                                      */ 
/*    痕方孔嬬・響函蝶倖猟周議蝶倖佚連(距喘CA AC凋綜糞・)            */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              param: 凋綜歌方                                      */ 
/*              Response: ・哘方象産喝曝                             */ 
/*              NumberToRead: 豚李響函議方象海響                     */ 
/*              Read: 糞縞響函議方象海業                             */ 
/*              Status: 荷恬潤惚,(根危列旗鷹才狛殻忖准)              */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・                                                     */ 
/*-------------------------------------------------------------------*/ 
static bool via_get_df_info(Smart_Handle_t Handle, 
									   U8* param, 
									   U8* Response, 
									   U16* Read, 
									   U8* Status) 
{ 
	int length; 
	bool bRet; 
	int tmplen=0; 
	if((param[0]==0)&&(param[1]==0)&&(param[2]==0)) 
	 	bRet = via_master_file(Handle,Response,0,Read,Status); 
	else 
		bRet = via_select_file(Handle,param,Response,0,Read,Status); 
	if(bRet == false) 
	{ 
		printf("via get info failed : select file failed !!\n"); 
		return false; 
	} 
	if(!via_get_consultation(Handle,Response,0,Read,Status,param[3],param[4])) 
	{ 
		printf("via get consulation failed !!\n"); 
		return false; 
	} 
	if(!via_consultation_length(Handle,Response,0,Read,Status)) 
	{ 
		printf("via get consulation length failed !!\n"); 
		return false; 
	} 
	length = Response[1]; 
	tmplen = length; 
	{ 
		if(via_consultation_answer(Handle,Response,0,Read,Status,tmplen-1)) 
		{ 
			if(!via_consultation_answer(Handle,Response+tmplen-1,0,Read,Status,1)) 
			{ 
				printf("via get consulation answer failed !!\n"); 
				return false; 
			} 
		}else 
		{ 
			printf("via get consulation answer failed !!\n"); 
			return false; 
		} 
	} 
	return true; 
} 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_get_sn                                           */ 
/*    痕方孔嬬・響函触催                                             */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_consulation_length                               */ 
/*              via_consulation_answer                               */ 
/*-------------------------------------------------------------------*/ 
bool via_get_sn(Smart_Handle_t Handle) 
{ 
	U8 param[5]; 
	U8 Response[254]; 
	U16 Read; 
	U8 Status[2]; 
	memset(param,0,5); 
	param[3] = 0xA4; 
	if(!via_get_df_info(Handle,param,Response,&Read,Status)) 
	{ 
		printf("via get sn failed !!\n"); 
		return false; 
	} 
	pstViaInfo->uCardNumber = (Response[1] << 24) + (Response[2] << 16) + (Response[3] << 8) + Response[4]; 
	printf("SN = %u\n",pstViaInfo->uCardNumber); 
	return true; 
} 
 
bool via_get_ppua(Smart_Handle_t Handle) 
{ 
	unsigned char insac[] = { 0xca, 0xac, 0x00, 0x00, 0x00 }; /* select data */ 
	unsigned char insb8[] = { 0xca, 0xb8, 0x00, 0x00, 0x02 }; /* read selected data */ 
	unsigned char Response[256]; 
	U16 Read; 
	U8 Status[2] = {0,0}; 
	if(!via_master_file(Handle,Response,0,&Read,Status)) 
	{ 
		printf("get via ppua failed : select file failed !!\n"); 
		return false; 
	} 
	insac[2] = 0xA4; 
	Status[0] = Status[1] = 0; 
	if(via_transfer(Handle,insac,5,Response,&Read,Status) != SMC_NO_ERROR) 
	{ 
		printf("via get ppua send command failed  !!\n"); 
		return false; 
	} 
	if(!via_check_status(Status)) 
	{ 
		printf("PB Words failed !!\n"); 
		return false; 
	} 
	insb8[4] = 0x02; 
	via_transfer(Handle,insb8,5,Response,&Read,Status); 
	if(!via_check_status(Status)) 
	{ 
		printf("PB Words failed !!\n"); 
		return false; 
	} 
	insb8[4] = Response[1]; 
	{ 
		int tmplen=0;		 
		tmplen = Response[1]; 
		insb8[4] = tmplen-1; 
		via_transfer(Handle,insb8,5,Response,&Read,Status); 
		insb8[4] = 1; 
		via_transfer(Handle,insb8,5,Response+tmplen-1,&Read,Status);		 
	} 
	if(!via_check_status(Status)) 
	{ 
		printf("PB Words failed !!\n"); 
		return false; 
	} 
	memcpy(pstViaInfo->uCardUA,Response,5); 
	printf("Card UA : %02x %02x %02x %02x %02x\n",pstViaInfo->uCardUA[0], 
								pstViaInfo->uCardUA[1],pstViaInfo->uCardUA[2], 
								pstViaInfo->uCardUA[3],pstViaInfo->uCardUA[4]); 
	return true; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_get_df_time                                      */ 
/*    痕方孔嬬・響函娩幡扮寂佚連                                     */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              param: 凋綜歌方                                      */ 
/*              Response: ・哘方象産喝曝                             */ 
/*              NumberToRead: 豚李響函議方象海響                     */ 
/*              Read: 糞縞響函議方象海業                             */ 
/*              Status: 荷恬潤惚,(根危列旗鷹才狛殻忖准)              */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_consulation_length                               */ 
/*              via_consulation_answer                               */ 
/*-------------------------------------------------------------------*/ 
static bool via_get_df_time(Smart_Handle_t Handle, 
							 U8* param, 
							 U8* Response, 
							 U16* Read, 
							 U8* Status) 
{ 
	int len; 
	U8 ins[9] = {0xCA,0xAC,0xA9,0x00,0x04,0x00,0x21,0xFF,0x9F}; 
	if(!via_select_file(Handle,param,Response,0,Read,Status)) 
	{ 
		printf("via get df time failed : select file failed !!\n"); 
		return false; 
	} 
	if(via_transfer(Handle,ins,9,Response,Read,Status) != SMC_NO_ERROR) 
	{ 
		printf("via get df time failed : send command failed !!\n"); 
		return false; 
	} 
	if(!via_consultation_length(Handle,Response,2,Read,Status)) 
	{ 
		printf("via get df time failed : get consulation length failed !!\n"); 
		return false; 
	} 
	len = Response[1]; 
	{ 
		int tmplen=len; 
		if(via_consultation_answer(Handle,Response,0,Read,Status,tmplen-1)) 
		{ 
			if(!via_consultation_answer(Handle,Response+tmplen-1,0,Read,Status,1)) 
			{ 
				printf("via get df time failed : get consulation answer failed !!\n"); 
				return false; 
			} 
		}else 
		{ 
			printf("via get df time failed : get consulation answer failed !!\n"); 
			return false; 
		} 
	} 
	return true; 
} 
 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_get_df_des                                       */ 
/*    痕方孔嬬・響函峺協猟周佚連                                     */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              file: 猟周歌方                                       */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*    ・購痕方・via_consulation_length                               */ 
/*              via_consulation_answer                               */ 
/*-------------------------------------------------------------------*/ 
void via_get_df_des(Smart_Handle_t Handle,U8* file) 
{ 
	U8 Response[254]; 
	U16 Read; 
	U8 Status[2]; 
	int len; 
	via_select_file(Handle,file,Response,0,&Read,Status); 
	via_get_consultation(Handle,Response,0,&Read,Status,0xA7,0x00); 
	via_consultation_length(Handle,Response,0,&Read,Status); 
	len = Response[1]; 
 
	{ 
		U8 tmplen=Response[1]; 
		via_consultation_answer(Handle,Response,0,&Read,Status,tmplen-1); 
		via_consultation_answer(Handle,Response+tmplen-1,0,&Read,Status,1); 
	} 
} 
 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_get_cw                                           */ 
/*    痕方孔嬬・窟僕ECM・響函陣崙忖                                  */ 
/*    歌    方・Handle: Smart触鞘凹                                  */ 
/*              pECM  : ECM方象遊峺寞                                */ 
/*              pCW   : 響函議CW方象                                 */ 
/*    卦 指 峙・18    : 函議陣崙忖議海業                             */ 
/*              -1    : 触音屶隔緩准朕工哘斌                         */ 
/*              -2    : 勣箔補秘畜鷹                                 */ 
/*              -3    : ECM厮将窟僕・徽音嬬盾准朕                    */ 
/*              -4    : 窟僕ECM方象・哘佚連竃危                      */ 
/*              -5    : 響函陣崙忖竃危                               */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static int via_get_cw(Smart_Handle_t Handle, 
  			         U8*			 pECM, 
			         U8*             pCW) 
{ 
	int tmp; 
	static U8	ins[5] = {0xca, 0xc0, 0x00, 0x00, 0x12}; 
	int length; 
	U8 Status[2]; 
	U8  file[3]; 
	U8  Response[254]; 
	U16  Read; 
	Smart_ErrorCode_t error; 
 
	/* if Card Not Prepared , return 0 */ 
	length = (pECM[1] & 0x0F << 8) + pECM[2] + 3; 
	file[0] = pECM[6]; 
	file[1] = pECM[7]; 
	file[2] = pECM[8]&0xF0; 
	Status[0] = 0; 
	Status[1] = 0; 
 
	if(!via_select_file(Handle,file,Response,0,&Read,Status)) 
		return -1; 
	pECM[4] = 0xCA; 
	pECM[5] = 0x88; 
	pECM[6] = 0x00; 
	pECM[7] = pECM[8] & 0x0F; 
	pECM[8] = length-9; 
	Status[0] = 0xFF; 
	Status[1] = 0xFF; 
 
	error = via_transfer(Handle, 
					 pECM+4, 
					 length-4, 
					 Response, 
					 &Read, 
					 Status); 
 
	if(error != SMC_NO_ERROR) 
		return -7; 
	if(Status[0] == 0x98) 
	{ 
		return -2; 
	} 
	else if(Status[1] == 0x08) 
	{ 
		return -3; 
	} 
	else if(!((Status[0] == 0x90) && (Status[1] == 0x00))) 
	{ 
		/* if procedure bytes is not 0x90 0x00,just flush the smart card and return -4 */ 
		return -4; 
	} 
	Read = 0; 
	error = via_transfer(Handle, 
						 ins, 
						 5, 
						 Response, 
						 &Read, 
						 Status); 
	if(error !=SMC_NO_ERROR) 
	{ 
		return -7; 
	} 
	if(Read==18) 
	{ 
		memcpy(pCW,Response,Read); 
		 
		/* We have got control words,return the length of CW */ 
		tmp = pCW[2]+pCW[3]+pCW[4]; 
		pCW[5] = tmp&0xFF; 
		tmp = pCW[6]+pCW[7]+pCW[8]; 
		pCW[9] = tmp&0xFF; 
		tmp = pCW[10]+pCW[11]+pCW[12]; 
		pCW[13] = tmp&0xFF; 
		tmp = pCW[14]+pCW[15]+pCW[16]; 
		pCW[17] = tmp&0xFF; 
		return Read; 
	} 
	else 
	{ 
		/* if we can't get CW, return -5 */ 
		return -5; 
	} 
	return -6; 
} 
 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_parse_ecm                                        */ 
/*    痕方孔嬬・侃尖ECM方象                                          */ 
/*    歌    方・pbuf: Ecm方象遊峺寞                                  */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
bool via_parse_ecm(Smart_Handle_t Handle,U8* pbuf,U8* pucCW) 
{ 
	static U8	aucCW[20]; 
	int length; 
	printf("Viaccess Parse ECM...... Handle = %d\n",Handle); 
	 
	memset(aucCW,0,20); 
	length = via_get_cw(Handle,pbuf,aucCW); 
	 
	printf("get cw length = %d\n",length); 
	if(length == -2) 
	{ 
		return false; 
	} 
	else if(length > 1) 
	{ 
		memcpy(pucCW,aucCW+2,16); 
		return true; 
	} 
	return false; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_parse_emm                                        */ 
/*    痕方孔嬬・侃尖EMM方象                                          */ 
/*    歌    方・pbuf: Emm方象遊峺寞                                  */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
void AnalyseEMMSA(unsigned char *data,int index) 
{ 
	memcpy(aucViaEMMBuf8E,&(data[0]),data[2]+3); 
}  
 
/* 
 *	function	: via_parse_emm 
 *	description : Parse Via EMM to update via smart card 
 *	input		: Smart Handle, EMM section , Emm Section Length 
 *	return		: true if upate successful , otherwise false 
 */ 
bool via_parse_emm(Smart_Handle_t Handle,U8* data,U16 len) 
{ 
	unsigned char cmd[5+188]; 
	unsigned char ucDataBuffer[188]; 
	U8 Response[255]; 
	U8 Status[2]; 
	U16 Read; 
	if(data[0] == 0x8C || data[0]== 0x8D) 
	{ 
		memcpy(aucViaEMMBuf8C,data,data[2]+3); 
		if(aucViaEMMBuf8E[0] == 0x8E) 
		{ 
			cmd[0] = 0xCA; 
			cmd[1] = 0xF0; 
			cmd[2] = 0x00; 
			cmd[3] = data[7]&0x0F; 
			cmd[4] = 0x22; 
			ucDataBuffer[0] = 0x9E; 
			ucDataBuffer[1] = 0x20; 
			memcpy(&ucDataBuffer[2],&aucViaEMMBuf8E[7],0x20); 
			memcpy(cmd+5,ucDataBuffer,0x22); 
			 
			via_transfer(Handle,cmd,cmd[4]+5,Response,&Read,Status); 
			if(Status[0]!=0x90) 
			{ 
				printf("EMM Address Not Matched !!\n"); 
				aucViaEMMBuf8E[0] = 0; 
				 
				return false; 
			} 
			printf("EMM Address Matched !!\n"); 
			cmd[0] = 0xCA; 
			cmd[1] = 0x18; 
			cmd[2] = 0x01; 
			cmd[3] = data[7]&0x0F;/*MK index*/ 
			cmd[4] = data[2]+5; 
			memcpy(ucDataBuffer,&data[8],data[2]-5); 
			ucDataBuffer[data[2]-5] = 0xF0;/*fill nano f0 08*/ 
			ucDataBuffer[data[2]-4] = 0x08; 
			memcpy(&ucDataBuffer[data[2]-3],&aucViaEMMBuf8E[aucViaEMMBuf8E[2]-5],8); 
			aucViaEMMBuf8E[0] = 0; 
			memcpy(cmd+5,ucDataBuffer,data[2]+5); 
			via_transfer(Handle,cmd,cmd[4]+5,Response,&Read,Status); 
			 
			if(Status[0]==0x91 && Status[1]==0x00) /* 9100 means subscription updated successful */ 
				return true; 
		} 
	} 
	else if(data[0] == 0x8E) 
	{ 
		AnalyseEMMSA(data,Handle); 
		if(aucViaEMMBuf8C[0] != 0) 
		{ 
			cmd[0] = 0xCA; 
			cmd[1] = 0xF0; 
			cmd[2] = 0x00; 
			cmd[3] = aucViaEMMBuf8C[7]&0x0F; 
			cmd[4] = 0x22; 
			ucDataBuffer[0] = 0x9E; 
			ucDataBuffer[1] = 0x20; 
			memcpy(&ucDataBuffer[2],&data[7],0x20); 
			memcpy(cmd+5,ucDataBuffer,0x22); 
			  
			 
			via_transfer(Handle,cmd,cmd[4]+5,Response,&Read,Status); 
			if(Status[0]!=0x90) 
			{ 
				aucViaEMMBuf8C[0] = 0; 
				 
				/*printf("EMM Address Not Matched !! 22\n");*/ 
				return false; 
			} 
			/*printf("EMM Address Matched !!  22\n");*/ 
			cmd[0] = 0xCA; 
			cmd[1] = 0x18; 
			cmd[2] = 0x01; 
			cmd[3] = aucViaEMMBuf8C[7]&0x0F;/*MK index*/ 
			cmd[4] = aucViaEMMBuf8C[2]+5; 
			memcpy(ucDataBuffer,&aucViaEMMBuf8C[8],aucViaEMMBuf8C[2]-5); 
			ucDataBuffer[aucViaEMMBuf8C[2]-5] = 0xF0;/*fill nano f0 08*/ 
			ucDataBuffer[aucViaEMMBuf8C[2]-4] = 0x08; 
			memcpy(&ucDataBuffer[aucViaEMMBuf8C[2]-3],&data[data[2]-5],8); 
			aucViaEMMBuf8C[0] = 0; 
			memcpy(cmd+5,ucDataBuffer,aucViaEMMBuf8C[2]+5); 
			via_transfer(Handle,cmd,cmd[4]+5,Response,&Read,Status); 
			/*printf("Status : %02x %02x  22\n",Status.StatusBlock.T0.PB[0],Status.StatusBlock.T0.PB[1]);*/ 
			 
			if(Status[0]==0x91 && Status[1]==0x00) /* 9100 means subscription updated successful */ 
				return true; 
		} 
	} 
	else if(data[0] == 0x88) 
	{ 
		cmd[0] = 0xCA; 
		cmd[1] = 0x18; 
		cmd[2] = 0x02; 
		cmd[3] = data[12]&0x0F; 
		cmd[4] = data[2] - 10; 
		memcpy(&ucDataBuffer[0], &data[13], data[2] - 10); 
		memcpy(cmd+5,ucDataBuffer,data[2]-10); 
		  
		 
		via_transfer(Handle,cmd,cmd[4]+5,Response,&Read,Status); 
		aucViaEMMBuf8E[0] = 0; 
		 
		if(Status[0]==0x91 && Status[1] == 0x00) /* 9100 means subscription updated successful */ 
			return true; 
	} 
	return false; 
} 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・via_transfer                                         */ 
/*    痕方孔嬬・Viaccess方象勧補                                     */ 
/*    歌    方・                                                     */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
static Smart_ErrorCode_t via_transfer(Smart_Handle_t Handle, 
								    U8*			 ins, 
								    U16			 NumberToWrite, 
								    U8*			 Response, 
								    U16*		 Read, 
								    U8*			 Status) 
{ 
	Smart_ErrorCode_t error = SMC_NO_ERROR; 
	error = Smart_Transfer(Handle,ins,NumberToWrite,Response,0,Read,Status); 
 
	return error; 
} 
 
/* 
 *	function	: via_check_status 
 *	description	: check viaccess command procedure bytes 
 *	input		: procedure bytes 
 *	return		: true if check passed , otherwise false 
 */ 
static bool via_check_status(U8* pb) 
{ 
	if(pb[0] == 0x6b)  
		return false; 
	else if(pb[0] == 0x6d) 
		return false; 
	else if(pb[0] == 0x90) 
		return true; 
	else if(pb[0] == 0x91) 
		return true; 
	else if(pb[0] == 0x98) 
		return true; 
	else 
	{ 
		printf("Viaccess Check Status failed : %02x %02x\n",pb[0],pb[1]); 
		return false; 
	} 
} 
/////////////////////-------The end --------------