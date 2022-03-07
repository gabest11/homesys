
/* 
云旗鷹叙恬葎僥楼冩梢岻朕議聞喘,萩艇噐24弌扮坪徭状繍凪評茅,萩齢哘喘噐斌匍試強賜凪万哺旋來試強嶄・ 
倦夸朔惚徭減・ 
*/ 
 
 
/****************************************************** 
 * 猟周兆・bjst.h 
 * 孔  嬬・侃尖BCA凋綜 
 * 恬  宀・ 
 * 晩  豚・ 
 *****************************************************/ 
 
#ifndef _bjst_h_ 
#define _bjst_h_ 
 
#include "sc_def.h" 
 
#define MAX_PROV_COUNT 4 
 
typedef struct   
{ 
	U32			   uCardNumber;             /*触催*/ 
	U8			   iAgeGrade;               /*定槍吉雫*/ 
	U16            provID[MAX_PROV_COUNT];  /*塰唔斌ID*/ 
}Bjst_Info_t; 
 
 
Bjst_Info_t  bjst;  /*畠蕉延楚・壓崘嬬触兜兵晒朔辛參岷俊聞喘万議匯乂佚連阻*/ 
 
/* 兜兵晒, 麼勣垢恬頁響函触坪議児云佚連・泌触催、塰唔斌吉佚連・壓斤触reset朔距喘   
   補秘     handle -- smart card 鞘凹 
   補竃     涙 
   卦指峙   true -- 兜兵晒撹孔 false -- 兜兵晒払移 
   凪麿                                                                  
*/ 
 
bool bjst_init(Smart_Handle_t handle); 
 
/* 侃尖盾裂ECM・誼欺CW・誼欺万厘断祥辛參心准朕阻・壓辺欺ECM朔距喘・辺函ECM議扮昨譜崔filter 
議及匯倖忖准0x80/0x81祥ok阻 
   補秘     Handle -- smart card 鞘凹  buf -- ECM佚連・貫0x80/0x81蝕兵 
   補竃     pucCW  -- 祥頁cw晴・16倖忖准・音頁謎甜・祥頁謎甜・功象秤趨低徭失編刮 
   卦指峙   true -- 盾裂ECM撹孔 false --盾裂ECM払移 
   凪麿        
*/ 
 
bool bjst_parse_ecm(Smart_Handle_t Handle,U8* pbuf,U8* pucCW); 
 
 
/* 侃尖EMM・麼勣祥頁頼撹斤触娩幡阻,辺欺EMM距喘。辺函EMM議扮昨譜崔filter議及匯倖忖准0x82・ 
5・6・7・8倖忖准祥頁触催阻。辛參叙譜崔及匯倖忖准・謹辺叱倖EMM冩梢冩梢填・  
 
   補秘     Handle -- smart card 鞘凹  pbuf -- EMM佚連・len -- 方象海業 
   補竃     涙 
   卦指峙   true -- 盾裂EMM撹孔 false --盾裂EMM払移 
   凪麿                                                                    
*/ 
bool bjst_parse_emm(Smart_Handle_t Handle,U8* pbuf,U16 len); 
 
#endif /* _bjst_h_ */ 
