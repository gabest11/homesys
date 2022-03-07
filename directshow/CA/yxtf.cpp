
/* 
云旗鷹叙恬葎僥楼冩梢岻朕議聞喘,萩艇噐24弌扮坪徭状繍凪評茅,萩齢哘喘噐斌匍試強賜凪万哺旋來試強嶄・ 
倦夸朔惚徭減・ 
*/ 
 
 
/****************************************************** 
 * 猟周兆・yxtf.c 
 * 孔  嬬・侃尖TCA凋綜 
 * 恬  宀・ 
 * 晩  豚・ 
 *****************************************************/ 
 
#include "yxtf.h" 
 
Yxtf_Info_t* pstYxtfInfo = NULL; 
 
static Smart_ErrorCode_t yxtf_transfer(Smart_Handle_t Handle, 
								   U8*			    ins, 
								   U16			    NumberToWrite, 
								   U8*			    Response, 
								   U16*			    Read, 
								   U8* Status);//方象勧補 
static bool yxtf_readdata(Smart_Handle_t Handle,U8 len,U8 *outbuf); 
static bool yxtf_begincmd(Smart_Handle_t Handle); 
static bool yxtf_get_sn(Smart_Handle_t Handle);//響函崘嬬触催鷹 
static bool yxtf_get_prov(Smart_Handle_t Handle);//資函塰唔斌佚連 
static bool yxtf_check_pairing(Smart_Handle_t Handle);//殊臥字触塘斤 
 
 
/************************************************************************/ 
/* 兜兵晒, 麼勣垢恬頁響函触坪議児云佚連・泌触催、塰唔斌吉佚連 
    
   補秘     handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 兜兵晒撹孔 false -- 兜兵晒払移 
   凪麿                                                                 */ 
/************************************************************************/ 
bool yxtf_init(Smart_Handle_t handle) 
{ 
	pstYxtfInfo = &yxtf; 
	memset(pstYxtfInfo,0,sizeof(Yxtf_Info_t)); 
 
	if(!yxtf_begincmd(handle)) 
	{ 
//		return false; 
	} 
 
#if 0 
	if(!yxtf_get_prov(handle)) 
	{ 
		printf("yxtf init : get prog info failed !!\n"); 
		return false; 
	} 
#endif 
 
	if(!yxtf_get_sn(handle)) 
	{ 
		printf("yxtf init : get sn failed !!\n"); 
//		return false; 
	} 
	 
	if(!yxtf_check_pairing(handle)) 
	{ 
		printf("yxtf init : get sn failed !!\n"); 
//		return false; 
	} 
	 
	printf("yxtf Init OK !!\n"); 
	return true; 
} 
 
/************************************************************************/ 
/* 軟兵峺綜,  宸倖凋綜壓崘嬬触 Reset 朔駅倬遍枠峇佩・岻朔嘉辛參峇佩凪麿議凋綜  
 
   補秘     Handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 凋綜峇佩撹孔 false -- 凋綜峇佩払移 
   凪麿                                     
 
宥儷幣箭: 
00 A4 04 00 05 A4 F9 5A 54 00 06  
90 00  
                                                                       */ 
/************************************************************************/ 
static bool yxtf_begincmd(Smart_Handle_t Handle) 
{ 
	U8	cmd[]={0x00,0xa4,0x04,0x00,0x05,0xf9,0x5a,0x54,0x00,0x06}; 
	U8	response[10]; 
	U8	pbword[2]={0}; 
	U16	cmdlen=0; 
	U16	replen=0; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
	 
	cmdlen=10; 
	bresult=yxtf_transfer(Handle,cmd,cmdlen,response,&replen,pbword); 
 
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
/* 響函塰唔斌ID双燕 
 
   補秘     Handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 資函塰唔斌佚連撹孔 false -- 資函塰唔斌佚連払移 
   凪麿     誼欺議塰唔斌佚連贋刈噐畠蕉延楚pstYxtfInfo嶄・辛喘噐OSD・幣                                
 
宥儷幣箭: 
 
80 44 00 00 08 44  
xx xx 00 00 xx xx 00 00 90 00 //耽曾倖忖准祥頁匯倖塰唔斌議ID・			*/ 
/************************************************************************/ 
 
static bool yxtf_get_prov(Smart_Handle_t Handle) 
{ 
	U8	cmd[5]={0x80,0x44,0x00,0x00,0x08}; 
	U8	response[10]; 
	U8	pbword[2]={0}; 
	int	cmdlen=0; 
	U16	replen=0; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
 
	int provcount=0; 
	int i; 
 
	cmdlen = 5+cmd[4]; 
	bresult = yxtf_transfer(Handle,cmd,cmdlen,response,&replen,pbword); 
//	if(bresult!=SMC_NO_ERROR) 
//	{ 
//		return false; 
//	} 
	/*遍枠殊臥狛殻忖准頁倦屎鳩*/ 
	if((pbword[0]!=0x90)||(pbword[1]!=0x00)) 
	{ 
		return false; 
	} 
	 
	for(i=0;i<MAX_PROV_COUNT;i++)//挫・頁恷謹屶隔4倖 
	{ 
		if((response[i*2]!=0)||(response[i*2+1]!=0)) 
		{ 
			provcount++; 
			pstYxtfInfo->provID[i]=(response[i*2]<<8)|response[i*2+1]; 
			printf("the provid[%d] =0x%04x\n",i,pstYxtfInfo->provID[i]); 
		} 
	} 
} 
 
 
/************************************************************************/ 
/* 響函崘嬬触催・祥頁tf触頭貧・幣議及眉怏方忖・tf匆鎮栖喘恂娩幡儖峽 
 
   補秘     Handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 資函触催撹孔 false -- 資函触催払移 
   凪麿     誼欺議触催贋刈噐畠蕉延楚pstYxtfInfo嶄・辛喘噐OSD・幣                                
 
宥儷幣箭: 
 
 
80 46 00 00 04 46 01 00 00 04  
61 04  
00 C0 00 00 04 C0  
xx xx xx xx 90 00   //触催 xx xx xx xx (噴鎗序崙)                     */ 
/************************************************************************/ 
bool yxtf_get_sn(Smart_Handle_t Handle) 
{ 
	U8	cmd[]={0x80,0x46,0x00,0x00,0x04,0x01,0x00,0x00,0x04}; 
	U8	response[100]; 
	U8	pbword[2]={0}; 
	U16	cmdlen=0; 
	U16	replen=0; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
 
	U8 nextreadsize=0; 
	 
	cmdlen = 5+cmd[4]; 
 
	bresult=yxtf_transfer(Handle,cmd,cmdlen,response,&replen,pbword); 
 
//	if(bresult!=SMC_NO_ERROR) 
//	{ 
//		return false; 
//	} 
 
	if((pbword[0]&0xf0)!=0x60) 
	{ 
		return false; 
	} 
	nextreadsize=pbword[1]; 
	 
	if(yxtf_readdata(Handle,nextreadsize,response)!=true) 
	{ 
		return false; 
	} 
 
	pstYxtfInfo->uCardNumber=(response[0]<<24)+(response[1]<<16) +(response[2]<<8)+response[3];//触催,触頭貧・幣議及眉怏方忖,匆頁娩幡喘議催鷹 
	return true; 
} 
 
 
/************************************************************************/ 
/* 殊臥字触塘斤 
 
   補秘     Handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 殊臥字触塘斤撹孔 false -- 殊臥字触塘斤払移 
   凪麿     誼欺議字触塘斤彜蓑贋刈噐畠蕉延楚pstYxtfInfo嶄・辛喘噐OSD・幣                                
 
宥儷幣箭: 
  
80 4C 00 00 04 4C FF FF FF FF  
94 B1     -- 乎触短嗤才販採字競歳鰯協 
    賜宀 
94 B2     -- 乎触厮将才字競歳鰯協(音匯協頁輝念字競歳填・)                                                                   */ 
/************************************************************************/ 
 
static bool yxtf_check_pairing(Smart_Handle_t Handle) 
{ 
	U8   cmd1[200]= {0x80,0x4c,0x00,0x00,0x04,0xFF,0xFF,0xFF,0xFF}; 
	U8	 reponse[100]; 
	U16  writelen = 0; 
	U16	 replen=0; 
	U8   status[2]; 
	int  i = 0; 
	 
	writelen = cmd1[4]+5; 
	yxtf_transfer(Handle,cmd1,writelen,reponse,&replen,status); 
	 
	if ((status[0] == 0x94)&&(status[1] == 0xB1) ) 
	{ 
		printf("乎触短嗤才販採字競歳鰯協!\n"); 
 
		pstYxtfInfo->paringflag = 0; 
		return true; 
	} 
	else if ((status[0] == 0x94)&&(status[1] == 0xB2) ) 
	{ 
		pstYxtfInfo->paringflag = 1; 
		 
        printf("乎触厮将才字競歳鰯協!\n"); 
 
		return true; 
	} 
	else 
	{ 
		pstYxtfInfo->paringflag = 2; 
		return false; 
	} 
} 
 
/************************************************************************/ 
/* 字触塘斤・匯違秤趨短駅勣喘議・壓嗤乂仇圭・tf壓ECM嶄譜崔阻勣箔崘嬬触塘斤 
嘉嬬盾竃cw,宸倖扮昨・一隈誼欺塘斤佚連・峇佩和中宸倖凋綜・祥辛參頼撹崘嬬触才 
低議字競歳塘斤阻。椎乂參葎字触塘斤阻祥涙隈慌・議・隈哘乎頁爺寔議・崛富斤tf 
栖傍頁宸劔。 
 
   補秘     Handle -- smart card 鞘凹 pairingcode -- 4忖准議塘斤佚連・触才 
斤哘議字匂宥儷狛殻嶄嗤,斤哘議字匂flash嶄匆贋嗤匯倖・諸阿彭触催議佚連 
   補竃     涙 
   卦指峙   true -- 字触塘斤撹孔 false -- 字触塘斤払移 
   凪麿        
 
80 4C 00 00 04 4C xx xx xx xx  
90 00      //輝念字触塘斤 
   賜宀 
94 B2(?)   //短徙聾冩梢醤悶頁焚担・触才凪万字競歳鰯協                    */ 
/************************************************************************/ 
 
bool yxtf_pairing(Smart_Handle_t Handle, U8* pairingcode)// pairingcode 4忖准議塘斤佚連, 
{ 
	U8  cmd1[200]= {0x80,0x4c,0x00,0x00,0x04,0xFF,0xFF,0xFF,0xFF}; 
	U8	reponse[100]; 
	U16 writelen = 0; 
	U16	replen=0; 
	U8  status[2]; 
	int i = 0; 
	 
	memcpy(cmd1+5, pairingcode, 4); 
 
	writelen = cmd1[4]+5;	 
	yxtf_transfer(Handle,cmd1,writelen,reponse,&replen,status); 
 
	if((status[0]==0x90)||(status[1]==0x00)) 
	{ 
	   printf("輝念字触鰯協!\n"); 
	} 
	else 
	{ 
	//	printf("************Status[0]=%02x Status[1]=%02x*********\n",status[0],status[1]); 
		printf("乎触厮鰯協凪万字競歳!\n"); 
 
		return false; 
	}	 
		 
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
 
bool yxtf_parse_ecm(Smart_Handle_t Handle,U8* buf,U8* pucCW) 
{ 
	U8  cmd1[200]={0x80,0x3a,0x00,0x01,0x53}; 
	U8  cmd2[10]={0x00,0xc0,0x00,0x00}; 
	U8	reponse[100]; 
	U16 writelen = 0; 
	U16	replen=0; 
	U8  status[2]; 
	U8* pbuf = buf; 
	int i = 0; 
 
	for (i=0; i<(buf[2]+3); i++)//孀欺寔屎勣吏触戦僕議方象 
	{ 
		if ((pbuf[0]==0x80)&&(pbuf[1]==0x3a)) 
		{ 
			break; 
		} 
		else 
		{ 
			pbuf = buf+i+1; 
		} 
	} 
 
	writelen = pbuf[4]+5; 
//	printf("ecm write len = %d\n", writelen);	 
	memcpy(cmd1,pbuf,writelen); 
	yxtf_transfer(Handle,cmd1,writelen,reponse,&replen,status); 
 
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
	 
	yxtf_transfer(Handle,cmd2,5,reponse,&replen,status); 
 
	//printf("rsp len = %d\n", replen);	 
	if (replen > 23)   
	{//載山夘tf議恂隈・徽頁麿断祥頁宸担恂議・載苧・短嗤紗畜亜・hoho~ ~  
		if(buf[0]==0x80) 
			memcpy(pucCW, reponse+8,16); 
		else 
		{ 
			memcpy(pucCW, reponse+16,8); 
	 		memcpy(pucCW+8,reponse+8,8); 
		} 
	} 
	else 
	{ 
		return false; 
	} 
 
	return true; 
} 
 
/************************************************************************/ 
/* 侃尖EMM・麼勣祥頁頼撹斤触娩幡阻。辺函EMM議扮昨譜崔filter議及匯倖忖准0x82・ 
5・6・7・8倖忖准祥頁触催阻。辛參叙譜崔及匯倖忖准・謹辺叱倖EMM冩梢冩梢填・  
 
   補秘     Handle -- smart card 鞘凹  data -- EMM佚連・len -- 方象海業 
   補竃     涙 
   卦指峙   true -- 盾裂EMM撹孔 false --盾裂EMM払移 
   凪麿                                                                   */ 
/************************************************************************/ 
 
bool yxtf_parse_emm(Smart_Handle_t Handle,U8* data,U16 len) 
{ 
	U8  cmd1[100]={0}; 
	U8  cmd2[10]={0x00,0xc0,0x00,0x00}; 
	U8	reponse[100]; 
	U16 writelen = 0; 
	U16	replen=0; 
	U8  status[2]; 
 
	writelen = data[15]+5; 
	memcpy(cmd1,data+11,writelen);//孀欺寔屎勣吏触戦僕議方象 
	yxtf_transfer(Handle,cmd1,writelen,reponse,&replen,status); 
	 
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
static Smart_ErrorCode_t yxtf_transfer(Smart_Handle_t Handle, 
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
static bool yxtf_readdata(Smart_Handle_t Handle,U8 len,U8 *outbuf) 
{ 
	U8		cmd[]={0x00,0xc0,0x00,0x00,0xff}; 
	U8		reponse[255]; 
	U16		cmdlen=5; 
	U16		replen=0; 
	U8		pbword[2]={0}; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
 
	memset(reponse,0,255); 
	cmd[4]	=	len; 
	bresult=yxtf_transfer(Handle,cmd,cmdlen,reponse,&replen,pbword); 
 
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