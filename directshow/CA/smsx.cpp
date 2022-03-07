
 
/* 
云旗鷹叙恬葎僥楼冩梢議朕議聞喘,萩艇噐24弌扮坪徭状繍凪評茅,萩齢哘喘噐斌匍試強賜凪万哺旋來試強嶄・ 
倦夸朔惚徭減・ 
*/ 
 
/****************************************************** 
 * 猟周兆・smsx.c 
 * 孔  嬬・侃尖SCA凋綜 
 * 恬  宀・ 
 * 晩  豚・ 
 *****************************************************/ 
 
#include "smsx.h" 
 
Smsx_Info_t* pstSmsxInfo; 
 
static Smart_ErrorCode_t smsx_transfer( 
								   Smart_Handle_t Handle, 
								   U8*			  ins, 
								   U16			  NumberToWrite, 
								   U8*			  Response, 
								   U16*			  Read, 
								   U8*            Status);//方象勧補 
bool smsx_readdata(Smart_Handle_t Handle,U8 len,U8 *outbuf); 
static bool smsx_begincmd(Smart_Handle_t Handle); 
static bool smsx_get_sn(Smart_Handle_t Handle);//響函崘嬬触催鷹 
 
/************************************************************************/ 
/* 兜兵晒, 麼勣垢恬頁響函触坪議児云佚連・泌触催、塰唔斌吉佚連 
    
   補秘     handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 兜兵晒撹孔 false -- 兜兵晒払移 
   凪麿                                                                 */ 
/************************************************************************/ 
bool smsx_init(Smart_Handle_t handle) 
{ 
	pstSmsxInfo = &smsx; 
	memset(pstSmsxInfo,0,sizeof(Smsx_Info_t)); 
 
	if(!smsx_begincmd(handle)) 
	{ 
		printf("smsx init : begin cmd failed !!\n"); 
//		return false; 
	} 
 
	if(!smsx_get_sn(handle)) 
	{ 
		printf("smsx init : get sn failed !!\n"); 
//		return false; 
	} 
 
	printf("SMSX Init OK !!\n"); 
 
	return true; 
} 
 
 
/************************************************************************/ 
/* 軟兵峺綜,  宸倖凋綜壓崘嬬触 Reset 朔駅倬遍枠峇佩・岻朔嘉辛參峇佩凪麿議凋綜  
 
   補秘     Handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 凋綜峇佩撹孔 false -- 凋綜峇佩払移 
   凪麿                                     
 
宥儷幣箭: 
 
00 A4 04 00 02 A4 3F 00  
90 00  
00 A4 04 00 02 A4 4A 00  
90 00  
                                                                       */ 
/************************************************************************/ 
static bool smsx_begincmd(Smart_Handle_t Handle) 
{ 
	U8	cmd[16]={0x00,0xa4,0x04,0x00,0x02,0x3F,0x00}; 
	U8	response[10]; 
	U8	pbword[2]={0}; 
	U16	cmdlen=0; 
	U16	replen=0; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
	 
	cmd[5] = 0x3F; 
	cmd[6] = 0x00; 
	cmdlen=cmd[4]+5; 
	bresult=smsx_transfer(Handle,cmd,cmdlen,response,&replen,pbword); 
 
//	if(bresult!=SMC_NO_ERROR) 
//	{ 
//		return false; 
//	} 
 
	if((pbword[0]!=0x90)||(pbword[1]!=0x00)) 
	{ 
		return false; 
	} 
 
 
	cmd[5] = 0x4A; 
	cmd[6] = 0x00; 
	cmdlen=cmd[4]+5; 
    bresult=smsx_transfer(Handle,cmd,cmdlen,response,&replen,pbword); 
 
//	if(bresult!=SMC_NO_ERROR) 
//	{ 
//		return false; 
//	} 
 
	if((pbword[0]!=0x90)||(pbword[1]!=0x00)) 
	{ 
		return false; 
	} 
 
	return true; 
} 
 
 
/************************************************************************/ 
/* 響函崘嬬触催鷹才崘嬬触ID・触催頁娩幡俶勣喘欺議・ID祥頁触頭貧・幣議匯堪方忖阻 
 
   補秘     Handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 資函触催撹孔 false -- 資函触催払移 
   凪麿     誼欺議触催贋刈噐畠蕉延楚pstYxtfInfo嶄・辛喘噐OSD・幣                                
 
宥儷幣箭: 
 
 
00 B2 00 05 06 B2 00 01 FF 00 01 FF  
61 67  
00 C0 00 00 67 C0  
00 00 00 xx xx xx xx yy yy yy yy yy yy yy yy 00 //触催 xx xx xx xx (噴鎗序崙) ID yy yy yy yy yy yy yy yy(忖憲堪)  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 90 00                        */ 
/************************************************************************/ 
bool smsx_get_sn(Smart_Handle_t Handle) 
{ 
	U8	cmd[]={0x00,0xB2,0x00,0x05,0x06,0x00,0x01,0xFF,0x00,0x01,0xFF}; 
	U8	response[200]; 
	U8	pbword[2]={0}; 
	U16	cmdlen=0; 
	U16	replen=0; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
 
	U8 nextreadsize=0; 
	 
	cmdlen = 5+cmd[4]; 
 
	bresult=smsx_transfer(Handle,cmd,cmdlen,response,&replen,pbword); 
 
//	if(bresult!=SMC_NO_ERROR) 
//	{ 
//		return false; 
//	} 
 
	if((pbword[0]&0xf0)!=0x60) 
	{ 
		return false; 
	} 
	nextreadsize=pbword[1]; 
	 
	if(smsx_readdata(Handle,nextreadsize,response)!=true) 
	{ 
		return false; 
	} 
 
	memcpy(pstSmsxInfo->uCardUA, response+7, 8);//触頭貧・幣議匯堪催鷹 
    pstSmsxInfo->uCardNumber=(response[3]<<24)+(response[4]<<16)+(response[5]<<8)+response[6];//宸嘉頁娩幡喘議催鷹! 
	return true; 
} 
 
/************************************************************************/ 
/* 侃尖盾裂ECM・誼欺CW・最最・誼欺万厘断祥辛參心准朕阻・辺函ECM議扮昨譜崔filter 
議及匯倖忖准0x80/0x81祥ok阻 
 
   補秘     Handle -- smart card 鞘凹  buf -- ECM佚連・貫0x80/0x81蝕兵 
   補竃     pucCW  -- 祥頁cw晴・16倖忖准・音頁謎甜・祥頁謎甜・功象秤趨低徭失編刮 
   卦指峙   true -- 盾裂ECM撹孔 false --盾裂ECM払移 
   凪麿        
                                                                        */ 
/************************************************************************/ 
 
bool smsx_parse_ecm(Smart_Handle_t Handle,U8* buf,U8* pucCW) 
{ 
	U8  cmd1[200]={0x80,0x32,0x00,0x00,0x3C}; 
	U8  cmd2[10]={0x00,0xc0,0x00,0x00}; 
	U8	reponse[100]; 
	U16 writelen = 0; 
	U16	replen=0; 
	U8  status[2]; 
	U8* pbuf = buf; 
	int i = 0; 
 
	writelen = pbuf[2]+3+5; 
//	printf("ecm write len = %d\n", writelen); 
	cmd1[4] = pbuf[2]+3; 
	memcpy(cmd1+5,pbuf,writelen-5);//勣斤ECM方象紗倖凋綜遊壅勧公崘嬬触 
	smsx_transfer(Handle,cmd1,writelen,reponse,&replen,status); 
 
	if (replen >= 2) 
	{ 
		cmd2[4] = reponse[1];  
	} 
	else 
	{ 
		if((status[0]&0xf0)==0x60) 
		{ 
			cmd2[4] = status[1]; 
		} 
		else 
		{ 
			return false; 
		}		 
	} 
 
 
	pbuf = reponse; 
	if (replen > 18) 
	{ 
		for (i=0; i<replen; i++) 
		{ 
			if ((pbuf[0]==0x83)&&(pbuf[1]==0x16)) 
			{ 
				break; 
			} 
			else 
			{ 
				pbuf = reponse+i+1; 
			} 
		} 
 
		if (i >= replen) 
		{ 
		    return false; 
		} 
 
		if(buf[0]==0x80)//宸倖仇圭載嗤吭房・泌惚音宸劔侃尖匯和・匯倖忖准岻餓・准朕辛嬬匆慧音竃栖 
		{ 
			memcpy(pucCW, pbuf+6,4); 
			memcpy(pucCW+4, pbuf+6+4+1,4); 
			memcpy(pucCW+8, pbuf+6+8+1,4); 
			memcpy(pucCW+12, pbuf+6+8+1+4+1,4); 
		} 
		else 
		{ 
			memcpy(pucCW, pbuf+6+8+1,4); 
			memcpy(pucCW+4, pbuf+6+8+1+4+1,4); 
	 		memcpy(pucCW+8, pbuf+6,4); 
			memcpy(pucCW+12, pbuf+6+4+1,4); 
		} 
	} 
	return true; 
} 
 
/************************************************************************/ 
/* 侃尖EMM・麼勣祥頁頼撹斤触娩幡阻。辺函EMM議扮昨譜崔filter議及匯倖忖准0x82・ 
5・6・7・8倖忖准祥頁触催阻,廣吭音頁ID亜。辛參叙譜崔及匯倖忖准・謹辺叱倖EMM 
冩梢冩梢填・  
 
   補秘     Handle -- smart card 鞘凹  data -- EMM佚連・len -- 方象海業 
   補竃     涙 
   卦指峙   true -- 盾裂EMM撹孔 false --盾裂EMM払移 
   凪麿                                                                   */ 
/************************************************************************/ 
 
bool smsx_parse_emm(Smart_Handle_t Handle,U8* buf,U16 len) 
{ 
	U8  cmd1[100]={0x80,0x30,0x00,0x00,0x4C}; 
	U8  cmd2[10]={0x00,0xc0,0x00,0x00}; 
	U8	reponse[100]; 
	U16 writelen = 0; 
	U16	replen=0; 
	U8  status[2]; 
 
	writelen = buf[2]+3+5; 
	cmd1[4] = buf[2]+3; 
	memcpy(cmd1+5,buf,writelen-5);//勣斤EMM方象紗倖凋綜遊壅勧公崘嬬触 
	smsx_transfer(Handle,cmd1,writelen,reponse,&replen,status); 
	 
	 
	if (replen > 2) 
	{ 
		return true; 
		 
	}else 
	{ 
		return false; 
	} 
 
	return true; 
} 
/************************************************************************/ 
/* 方象勧補・字競歳才崘嬬触宥儷議俊笥 
 
   補秘     Handle -- smart card 鞘凹・ins -- 勣勧僕議方象・NumberToWrite --  
 勣勧僕議方象海業 
   補竃     Response -- 指哘方象・ Read -- 指哘議方象海業・Status -- 彜蓑忖准  
   卦指峙   方象勧補危列窃侏 SMC_NO_ERROR燕幣涙危列 
   凪麿             
                                                                        */ 
/************************************************************************/ 
static Smart_ErrorCode_t smsx_transfer(Smart_Handle_t Handle, 
								    U8*			 ins, 
								    U16			 NumberToWrite, 
								    U8*			 Response, 
								    U16*		 Read, 
								    U8*			 Status) 
{ 
	Smart_ErrorCode_t error = SMC_NO_ERROR; 
 
	error = Smart_Transfer(Handle,ins,NumberToWrite,Response,0,Read,Status); 
 
//	printf("Status[0]=0x%02x  Status[1]=0x%02x error = %d\n",Status[0],Status[1],error); 
	return error; 
} 
 
/************************************************************************/ 
/*貫触嶄響函峺協海業議方象 
 
   補秘     Handle -- smart card 鞘凹・len -- 勣貫崘嬬触嶄響函議方象海業 
   補竃     outbuf -- 崘嬬触指哘議方象  
   卦指峙   true -- 響函方象撹孔 false -- 響函方象払移 
   凪麿                                                                 */ 
/************************************************************************/ 
bool smsx_readdata(Smart_Handle_t Handle,U8 len,U8 *outbuf) 
{ 
	U8		cmd[]={0x00,0xc0,0x00,0x00,0xff}; 
	U8		reponse[255]; 
	U16		cmdlen=5; 
	U16		replen=0; 
	U8		pbword[2]={0}; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
 
	memset(reponse,0,255); 
	cmd[4]	=	len; 
	bresult=smsx_transfer(Handle,cmd,cmdlen,reponse,&replen,pbword); 
 
//	if(bresult!=SMC_NO_ERROR) 
//	{ 
//		return false; 
//	} 
 
	if((pbword[0]==0x90)&&(pbword[1]==0x00)&&(replen==len)) 
	{ 
		memcpy(outbuf,reponse,replen); 
		return true; 
	} 
	else 
	{ 
		return false; 
	} 
} 
 
///////////---The end---------