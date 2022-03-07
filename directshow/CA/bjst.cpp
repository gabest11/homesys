
/* 
云旗鷹叙恬葎僥楼冩梢岻朕議聞喘,萩艇噐24弌扮坪徭状繍凪評茅,萩齢哘喘噐斌匍試強賜凪万哺旋來試強嶄・ 
倦夸朔惚徭減・ 
*/ 
 
 
/****************************************************** 
 * 猟周兆・bjst.c 
 * 孔  嬬・侃尖BCA凋綜 
 * 恬  宀・ 
 * 晩  豚・ 
 *****************************************************/ 
 
#include "bjst.h" 
 
Bjst_Info_t* pstBjstInfo = NULL; 
 
static Smart_ErrorCode_t bjst_transfer(Smart_Handle_t Handle, 
								   U8*			    ins, 
								   U16			   NumberToWrite, 
								   U8*			   Response, 
								   U16*			   Read, 
								   U8* Status);//方象勧補 
static bool bjst_begincmd(Smart_Handle_t Handle); 
static bool bjst_get_key(Smart_Handle_t Handle);//響函key・喘恬繍触嶄僕竃議cw盾畜 
static bool bjst_get_sn(Smart_Handle_t Handle);//響函崘嬬触催 
 
static U8 keywords[9] = {0};//響函key・喘恬繍触嶄僕竃議cw盾畜 
 
 
/************************************************************************/ 
/* 兜兵晒, 麼勣垢恬頁響函触坪議児云佚連・泌触催、塰唔斌吉佚連 
    
   補秘     handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 兜兵晒撹孔 false -- 兜兵晒払移 
   凪麿                                                                 */ 
/************************************************************************/ 
bool bjst_init(Smart_Handle_t handle) 
{ 
	pstBjstInfo = &bjst; 
	memset(pstBjstInfo,0,sizeof(Bjst_Info_t)); 
 
	if(!bjst_begincmd(handle)) 
	{ 
		printf("bjst init : get ppua failed !!\n"); 
//		return false; 
	} 
 
 
	if(!bjst_get_sn(handle)) 
	{ 
		printf("bjst init : get sn failed !!\n"); 
//		return false; 
	} 
 
	if(!bjst_get_key(handle)) 
	{ 
		printf("bjst init : get key failed !!\n"); 
		return false; 
	} 
 
	printf("bjst Init OK !!\n"); 
	return true; 
} 
 
 
/************************************************************************/ 
/* 軟兵峺綜,  宸眉倖凋綜壓崘嬬触 Reset 朔駅倬遍枠峇佩・岻朔嘉辛參峇佩凪麿議凋綜  
 
   補秘     Handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 凋綜峇佩撹孔 false -- 凋綜峇佩払移 
   凪麿                                     
                                                                       */ 
/************************************************************************/ 
static bool bjst_begincmd(Smart_Handle_t Handle) 
{ 
	U8	response[50]={0}; 
	U16 cmdlen	=0; 
	U16	resplen =0; 
	U8  pbword[2]={0}; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
	int i = 0; 
 
	U8 cmd1[100] = {0x00, 0x84, 0x00, 0x00, 0x10}; 
	U8 cmd2[100] = {0x00, 0x0C, 0x00, 0x00, 0x10}; 
	U8 cmd3[100] = {0x00, 0x0B, 0x00, 0x00, 0x10}; 
 
	cmdlen = 5; 
	bresult=bjst_transfer(Handle,cmd1,cmdlen,response,&resplen,pbword); 
 
	if(bresult!=SMC_NO_ERROR) 
	{ 
		bresult=bjst_transfer(Handle,cmd1,cmdlen,response,&resplen,pbword); 
	} 
 
	memcpy(cmd2+5,response,8); 
    cmd2[13] = 0x56; 
 
    cmdlen = 14; 
 
	bresult=bjst_transfer(Handle,cmd2,cmdlen,response,&resplen,pbword); 
 
	if(bresult!=SMC_NO_ERROR) 
	{ 
		bresult=bjst_transfer(Handle,cmd2,cmdlen,response,&resplen,pbword); 
		return false; 
	} 
	 
	memcpy(cmd3+5,response,8); 
    cmd3[13] = 0x56; 
 
    cmdlen = 14; 
	bresult=bjst_transfer(Handle,cmd3,cmdlen,response,&resplen,pbword); 
 
	if(bresult!=SMC_NO_ERROR) 
	{ 
		bresult=bjst_transfer(Handle,cmd3,cmdlen,response,&resplen,pbword); 
		return false; 
	} 
	 
	return true; 
} 
 
/************************************************************************/ 
/* 響函崘嬬触催・祥頁st触頭貧・幣議方忖・匆喘栖恂娩幡儖峽 
 
   補秘     Handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 資函触催撹孔 false -- 資函触催払移 
   凪麿     誼欺議触催贋刈噐畠蕉延楚pstBjstInfo嶄・辛喘噐OSD・幣                                
 
宥儷幣箭: 
 
 
00 00 05 81 D4 00 01 05 54  
00 00 07 00 xx xx xx xx 90 00   //触催 xx xx xx xx (噴鎗序崙)                     */ 
/************************************************************************/ 
 
 
static bool bjst_get_sn(Smart_Handle_t Handle) 
{ 
	U8	cmd[]={0x81,0xD4,0x00,0x01,0x05}; 
	U8	response[100]; 
	U8	pbword[2]={0}; 
	U16	cmdlen=0; 
	U16	replen=0; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
	 
	cmdlen = 5; 
 
	bresult=bjst_transfer(Handle,cmd,cmdlen,response,&replen,pbword); 
 
	if((bresult!=SMC_NO_ERROR)&&(replen<5)) 
	{ 
		printf("方象危列 \n"); 
		return false; 
	} 
	 
	pstBjstInfo->uCardNumber=(response[1]<<24)+(response[2]<<16)+(response[3]<<8)+response[4]; 
	printf("card number = %8d\n", pstBjstInfo->uCardNumber); 
	return true; 
} 
 
 
 
/************************************************************************/ 
/* 響函紗畜cw議畜埒・壓繍ECM勧公崘嬬触・崘嬬触盾竃CW勧公字競歳議扮昨祥頁喘 
宸怏方恂議紗畜(埋隼麻隈曳熟酒汽・徽頁崛富頁嗤宸倖吭紛議・侭參載栄篇tf才sm議 
恂隈・珊欺侃傚剌焚担芦畠來・涙囂...),凪糞輝扮的宸倖仇圭珊頁雑継阻音富扮寂議・ 
 
   補秘     Handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 響函方象撹孔 false -- 響函方象払移 
   凪麿     誼欺議佚連贋刈噐畠蕉延楚keywords嶄                                
 
宥儷幣箭: 
 
 
00 00 05 81 D0 00 01 08 5D  
00 00 0A xx xx xx xx xx xx xx xx 90 00                      */ 
/************************************************************************/ 
 
bool bjst_get_key(Smart_Handle_t Handle) 
{ 
	U8	cmd[]={0x81,0xD0,0x00,0x01,0x08}; 
	U8	response[100]; 
	U8	pbword[2]={0}; 
	U16	cmdlen=0; 
	U16	replen=0; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
	 
	int i = 0; 
 
	cmdlen = 5; 
 
	bresult=bjst_transfer(Handle,cmd,cmdlen,response,&replen,pbword); 
 
	if((bresult!=SMC_NO_ERROR)&&(replen<5)) 
	{ 
		printf("方象危列 \n"); 
		return false; 
	} 
	 
	memcpy(keywords, response, 8); 
 
	return true; 
} 
 
/************************************************************************/ 
/* 侃尖盾裂ECM・誼欺CW・誼欺万厘断祥辛參心准朕阻・辺函ECM議扮昨譜崔filter 
議及匯倖忖准0x80/0x81祥ok阻 
 
   補秘     Handle -- smart card 鞘凹  buf -- ECM佚連・貫0x80/0x81蝕兵 
   補竃     pucCW  -- 祥頁cw晴・16倖忖准・音頁謎甜・祥頁謎甜・功象秤趨低徭失編刮 
   卦指峙   true -- 盾裂ECM撹孔 false --盾裂ECM払移 
   凪麿        
                                                                        */ 
/************************************************************************/ 
bool bjst_parse_ecm(Smart_Handle_t Handle,U8* buf,U8* pucCW) 
{  
	U8	cmd[100]	={0x80,0xEA,0x80,0x00}; 
	U8	cmd2[100]	={0x00,0x84,0x00,0x00,0x10}; 
	U8	response[50]={0}; 
	U16 cmdlen	=0; 
	U16	resplen =0; 
	U8  pbword[2]={0}; 
	Smart_ErrorCode_t bresult=SMC_NO_ERROR; 
	int i = 0; 
	U8  tempcw[16] = {0}; 
 
	memcpy(cmd+4,buf+4,0x43); 
 
	cmd[4+0x43]		= 0x01; 
	cmd[4+0x43+1]	= 0x00; 
	 
	cmdlen			= 0x49; 
 
	bresult=bjst_transfer(Handle,cmd,cmdlen,response,&resplen,pbword); 
 
	if((bresult!=SMC_NO_ERROR)&&(resplen<17)) 
	{ 
		printf("勧僕方象窟伏危列!!!\n"); 
		return false; 
	} 
	else if ((response[0]==0x6B)&&(response[1]==0x01)) 
	{ 
		printf("短嗤娩幡\n"); 
		return false; 
	} 
 
	for (i=0; i<16; i++) 
	{ 
		tempcw[i] = response[i+1]^keywords[i%8]; 
	} 
	 
	memcpy(pucCW,tempcw,16); 
 
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
 
bool bjst_parse_emm(Smart_Handle_t Handle,U8* data,U16 len) 
{ 
	U8  cmd1[100]={0}; 
	U8  cmd2[10]={0x00,0xc0,0x00,0x00}; 
	U8	reponse[100]; 
	U16 writelen = 0; 
	U16	replen=0; 
	U8  status[2]; 
 
	writelen = data[15]+5; 
	memcpy(cmd1,data+11,writelen); 
	bjst_transfer(Handle,cmd1,writelen,reponse,&replen,status); 
	 
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
/* 方象勧補・字競歳才崘嬬触宥儷議俊笥・凪糞匆辛參音喘撃廾・岷俊距喘Smart_Transfer・ 
壓宸麼勣頁紗嬉咫距編喘議 
 
   補秘     Handle -- smart card 鞘凹・ins -- 勣勧僕議方象・NumberToWrite --  
 勣勧僕議方象海業 
   補竃     Response -- 指哘方象・ Read -- 指哘議方象海業・Status -- 彜蓑忖准  
   卦指峙   方象勧補危列窃侏 SMC_NO_ERROR燕幣涙危列 
   凪麿             
                                                                        */ 
/************************************************************************/ 
 
static Smart_ErrorCode_t bjst_transfer(Smart_Handle_t Handle, 
								    U8*			 ins, 
								    U16			 NumberToWrite, 
								    U8*			 Response, 
								    U16*		 Read, 
								    U8*			 Status) 
{ 
	Smart_ErrorCode_t error = SMC_NO_ERROR; 
 
	U8 tempins[256] = {0x42,0x4a,0x53,0x54}; 
 
	memcpy(tempins+4, ins, NumberToWrite); 
	 
	error = Smart_Transfer(Handle,tempins,NumberToWrite,Response,0,Read,Status); 
 
//	printf("Status[0]=0x%02x   Status[1]=0x%02x error = %d\n",Status[0],Status[1],error); 
	return error; 
} 
 
 
/////////////////////----The end ----------------