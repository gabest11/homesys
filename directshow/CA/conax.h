
/* 
云旗鷹叙恬葎僥楼冩梢岻朕議聞喘,萩艇噐24弌扮坪徭状繍凪評茅,萩齢哘喘噐斌匍試強賜凪万哺旋來試強嶄・ 
倦夸朔惚徭減・ 
*/ 
 
 
/****************************************************** 
 * 猟周兆・conax.h 
 * 孔  嬬・侃尖BCA凋綜 
 * 恬  宀・ 
 * 晩  豚・ 
 *****************************************************/ 
 
 
 
#ifndef _conax_h_ 
#define _conax_h_ 
 
#include "sc_def.h" 
 
typedef struct  
{ 
	U8  Version;					/* 井云催 */ 
	U16 CaSysId;					/* CA狼由ID */ 
	U16 CountryIndicatorValue;		/* 忽社炎崗 */ 
	U8  MatRat;						/* 定槍吉雫 */ 
	U8  MaxSessionNum;				/* 恷寄氏三倖方 */ 
}Conax_Param_t;		/* Conax触歌方 */ 
 
 
typedef struct 
{ 
	int							iCardNumber; 
	Conax_Param_t				stParam; 
	U8							auEMMAddress[10][7]; 
	int							iEMMAddressCount;	 
}Conax_Info_t; 
 
 
Conax_Info_t  conax; /*畠蕉延楚・壓崘嬬触兜兵晒朔辛參岷俊聞喘万議匯乂佚連阻*/ 
 
/* 兜兵晒, 麼勣垢恬頁響函触坪議児云佚連・泌触催、塰唔斌吉佚連・壓斤触reset朔距喘   
   補秘     handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 兜兵晒撹孔 false -- 兜兵晒払移 
   凪麿                                                                  
*/ 
 
bool conax_init(Smart_Handle_t handle); 
 
/* 兜兵晒, 麼勣垢恬頁響函触坪議児云佚連・泌触催、塰唔斌吉佚連・壓斤触reset朔距喘   
   補秘     handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 兜兵晒撹孔 false -- 兜兵晒払移 
   凪麿                                                                  
*/ 
 
bool conax_init(Smart_Handle_t handle); 
 
 
/* 侃尖盾裂CAT燕・壓辺欺CAT朔距喘・宸泣conax曳熟蒙艶亜,麼勣頁頼撹匯乂丕刮. 
 
   補秘     Handle -- smart card 鞘凹  buf -- CAT方象 len -- 方象海業 
   補竃     涙 
   卦指峙   涙 
   凪麿        
*/ 
 
void conax_parse_cat(Smart_Handle_t Handle,U8* pbuf,U16 len); 
 
/* 侃尖盾裂ECM・誼欺CW・誼欺万厘断祥辛參心准朕阻・壓辺欺ECM朔距喘・辺函ECM議扮昨譜崔filter 
議及匯倖忖准0x80/0x81祥ok阻 
   補秘     Handle -- smart card 鞘凹  buf -- ECM佚連・貫0x80/0x81蝕兵 
   補竃     pucCW  -- 祥頁cw晴・16倖忖准・音頁謎甜・祥頁謎甜・功象秤趨低徭失編刮 
   卦指峙   true -- 盾裂ECM撹孔 false --盾裂ECM払移 
   凪麿        
*/ 
 
bool conax_parse_ecm(Smart_Handle_t Handle,U8* pbuf,U8* pucCW); 
 
/* 侃尖EMM・麼勣祥頁頼撹斤触娩幡阻。辺函EMM議扮昨譜崔filter議及匯倖忖准0x82・ 
5・6・7・8倖忖准祥頁触催阻。辛參叙譜崔及匯倖忖准・謹辺叱倖EMM冩梢冩梢填・  
 
   補秘     Handle -- smart card 鞘凹  data -- EMM佚連・len -- 方象海業 
   補竃     涙 
   卦指峙   true -- 盾裂EMM撹孔 false --盾裂EMM払移 
   凪麿                                                                    
*/ 
 
bool conax_parse_emm(Smart_Handle_t Handle,U8* pbuf,U16 len);//盾裂EMM・頼撹娩幡吉 
 
#endif /* _conax_h_ */ 
