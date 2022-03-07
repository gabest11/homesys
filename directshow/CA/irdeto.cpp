/* 
云旗鷹叙恬葎僥楼冩梢岻朕議聞喘,萩艇噐24弌扮坪徭状繍凪評茅,萩齢哘喘噐斌匍試強賜凪万哺旋來試強嶄・ 
倦夸朔惚徭減・ 
*/ 
 
 
/****************************************************** 
 * 猟周兆・ird.c 
 * 孔  嬬・侃尖TCA凋綜 
 * 恬  宀・ 
 * 晩  豚・ 
 *****************************************************/ 
 
#include "irdeto.h" 
 
 
static U8 getCardNoSerial[7] = { 0x01,0x02,0x00,0x03,0x00,0x00,0x3F}; 
static U8 getHexSerial[7]   = { 0x01,0x02,0x01,0x03,0x00,0x00,0x3E}; 
static U8 getProvider[7]    = { 0x01,0x02,0x03,0x03,0x00,0x00,0x00}; 
static U8 getChannelID[7]   = { 0x01,0x02,0x04,0x00,0x00,0x01,0x00}; 
static U8 ecmCmd[7]	=	{ 0x01,0x05,0x00,0x00,0x02,0x00}; 
static U8 emmCmd[7] =   { 0x01,0x01,0x00,0x00,0x00,0x00}; 
static U8 getCamKey383C[] = {   0x01,0x02,0x09,0x03,0x00,0x40, 
								0x11,0x22,0x33,0x44,0x55,0x66, 
								0x77,0x88,0x11,0x22,0x33,0x44, 
								0x55,0x66,0x77,0x88,0x11,0x22, 
								0x33,0x44,0x55,0x66,0x77,0x88, 
								0x12,0x34,0x56,0x78,0x90,0xAB, 
								0xCD,0xEF,0xFF,0xFF,0xFF,0xFF, 
								0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 
								0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 
								0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 
								0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 
								0xFF,0xFF,0xFF,0xFF			 }; 
 
 
static const U8 camKey[] =  
{ 
   0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88 
}; 
 
static const U8 cryptTable[256] =  
{ 
	0xDA,0x26,0xE8,0x72,0x11,0x52,0x3E,0x46,0x32,0xFF,0x8C,0x1E,0xA7,0xBE,0x2C,0x29, 
	0x5F,0x86,0x7E,0x75,0x0A,0x08,0xA5,0x21,0x61,0xFB,0x7A,0x58,0x60,0xF7,0x81,0x4F, 
	0xE4,0xFC,0xDF,0xB1,0xBB,0x6A,0x02,0xB3,0x0B,0x6E,0x5D,0x5C,0xD5,0xCF,0xCA,0x2A, 
	0x14,0xB7,0x90,0xF3,0xD9,0x37,0x3A,0x59,0x44,0x69,0xC9,0x78,0x30,0x16,0x39,0x9A, 
	0x0D,0x05,0x1F,0x8B,0x5E,0xEE,0x1B,0xC4,0x76,0x43,0xBD,0xEB,0x42,0xEF,0xF9,0xD0, 
	0x4D,0xE3,0xF4,0x57,0x56,0xA3,0x0F,0xA6,0x50,0xFD,0xDE,0xD2,0x80,0x4C,0xD3,0xCB, 
	0xF8,0x49,0x8F,0x22,0x71,0x84,0x33,0xE0,0x47,0xC2,0x93,0xBC,0x7C,0x3B,0x9C,0x7D, 
	0xEC,0xC3,0xF1,0x89,0xCE,0x98,0xA2,0xE1,0xC1,0xF2,0x27,0x12,0x01,0xEA,0xE5,0x9B, 
	0x25,0x87,0x96,0x7B,0x34,0x45,0xAD,0xD1,0xB5,0xDB,0x83,0x55,0xB0,0x9E,0x19,0xD7, 
	0x17,0xC6,0x35,0xD8,0xF0,0xAE,0xD4,0x2B,0x1D,0xA0,0x99,0x8A,0x15,0x00,0xAF,0x2D, 
	0x09,0xA8,0xF5,0x6C,0xA1,0x63,0x67,0x51,0x3C,0xB2,0xC0,0xED,0x94,0x03,0x6F,0xBA, 
	0x3F,0x4E,0x62,0x92,0x85,0xDD,0xAB,0xFE,0x10,0x2E,0x68,0x65,0xE7,0x04,0xF6,0x0C, 
	0x20,0x1C,0xA9,0x53,0x40,0x77,0x2F,0xA4,0xFA,0x6D,0x73,0x28,0xE2,0xCD,0x79,0xC8, 
	0x97,0x66,0x8E,0x82,0x74,0x06,0xC7,0x88,0x1A,0x4A,0x6B,0xCC,0x41,0xE9,0x9D,0xB8, 
	0x23,0x9F,0x3D,0xBF,0x8D,0x95,0xC5,0x13,0xB9,0x24,0x5A,0xDC,0x64,0x18,0x38,0x91, 
	0x7F,0x5B,0x70,0x54,0x07,0xB6,0x4B,0x0E,0x36,0xAC,0x31,0xE6,0xD6,0x48,0xAA,0xB4 
}; 
 
typedef struct  
{ 
	U8 uPBWords[2]; 
	bool		  bResult; 
	char		  sComment[100]; 
}IrdStatusMsg_t; 
 
static const IrdStatusMsg_t IrdetoStatusMsgs[] =  
{ 
	{ { 0x00,0x00 }, true, "Instruction executed without error" }, 
	{ { 0x55,0x00 }, true, "Instruction executed without error" }, 
	{ { 0x9D,0x00 }, true, "Decoding successfull" }, 
	{ { 0x90,0x00 }, false, "ChID missing. Not subscribed?" }, 
	{ { 0x93,0x00 }, false, "ChID out of date. Subscription expired?" }, 
	{ { 0x9C,0x00 }, false, "Master key error" }, 
	{ { 0x9E,0x00 }, false, "Wrong decryption key" }, 
	{ { 0x9F,0x00 }, false, "Missing key" }, 
	{ { 0x70,0x00 }, false, "Wrong hex serial" }, 
	{ { 0x71,0x00 }, false, "Wrong provider" }, 
	{ { 0x72,0x00 }, false, "Wrong provider group" }, 
	{ { 0x73,0x00 }, false, "Wrong provider group" }, 
	{ { 0x7C,0x00 }, false, "Wrong signature" }, 
	{ { 0x7D,0x00 }, false, "Masterkey missing" }, 
	{ { 0x7E,0x00 }, false, "Wrong provider identifier" }, 
	{ { 0x7F,0x00 }, false, "Invalid nano" }, 
	{ { 0x54,0x00 }, true, "No more ChID's" }, 
}; 
 
 
Irdeto_Info_t*	   pstIrdetoInfo; 
 
 
static bool IrdetoCheckStatus(U8* PB0); 
static U8 GetXorSum(const U8 *mem, int len); 
static int IrdetoWrite(Smart_Handle_t Handle, 
					   U8* cmd, 
					   U8* Response, 
					   U16 goodSB, 
					   U8* IrdetoStatus); 
static void SetProviderIrdeto(U8 pb, const U8 *pi,int iNum); 
static void RotateLeft8Byte(U8 *key); 
static void RevCamCrypt(const U8 *key, U8 *data); 
 
 
/************************************************************************/ 
/* Irdeto購噐channel議侃尖・麼勣恬喘祥頁亅廁繍崘嬬触嶄channel・購議佚連贋刈噐 
潤更悶pstIrdetoInfo嶄  
 
   補秘     Handle -- smart card 鞘凹 
   補竃     繍宥儷狛殻嶄誼欺議channel佚連耶紗欺潤更悶pstIrdetoInfo嶄 
   卦指峙   涙 
   凪麿                                     
                                                          */ 
/************************************************************************/ 
 
static void Irdetofindchannelindex(U8 *input) 
{ 
	int i; 
	for(i=0;i<pstIrdetoInfo->aucChannelCount;i++) 
	{ 
		if(pstIrdetoInfo->aucChannelId[i]==input[0])  
		{ 
			if ((U32)((input[1]<<16)+(input[2]<<8)) > pstIrdetoInfo->aucDateInfo[i]) 
			{ 
				pstIrdetoInfo->aucDateInfo[i]=((input[1]<<16)+(input[2]<<8)); 
			} 
			 
			return; 
		} 
	} 
	pstIrdetoInfo->aucChannelId[i]  = input[0]; 
	pstIrdetoInfo->aucDateInfo[i]	= ((input[1]<<16)+(input[2]<<8)); 
	pstIrdetoInfo->aucChannelCount ++; 
} 
 
 
/*-------------------------------------------------------------------*/ 
/*    痕 方 兆・irdeto_init                                          */ 
/*    痕方孔嬬・Irdeto CA兜兵晒痕方                                  */ 
/*    歌    方・                                                     */ 
/*    卦 指 峙・                                                     */ 
/*    畠蕉延楚・                                                     */ 
/*    距喘庁翠:                                                      */ 
/*    姥    廣・                                                     */ 
/*-------------------------------------------------------------------*/ 
bool irdeto_init(Smart_Handle_t Handle) 
{ 
    int r; 
	int rlen; 
	int i,j,k; 
	U8 ucIrdetoStatus[2]; 
	U8 Response[256]; 
 
	U8 cmd[100]={0}; 
 
	pstIrdetoInfo = &irdeto; 
	memset(pstIrdetoInfo,0,sizeof(Irdeto_Info_t)); 
 
 
	/* Get Card Number (Ascii Serial) */ 
	r=IrdetoWrite(Handle,getCardNoSerial,Response,0,ucIrdetoStatus); 
	if((r>0) &&IrdetoCheckStatus(ucIrdetoStatus) && r>=10) 
	{ 
		memcpy(pstIrdetoInfo->acCardNoStr,&Response[8],10); 
		pstIrdetoInfo->acCardNoStr[10]='X'; 
		printf("CardNumber = %s\n",pstIrdetoInfo->acCardNoStr); 
	} 
	else 
	{ 
		/*printf("Get Irdeto Ascii Serial No failed !!\n");*/ 
		return false; 
	} 
 
	/* Get Hex Serial */ 
	r=IrdetoWrite(Handle,getHexSerial,Response,0,ucIrdetoStatus); 
	if(IrdetoCheckStatus(ucIrdetoStatus)&&(r>=25)) 
	{ 
		pstIrdetoInfo->ucNoOfProv = Response[18]; 
		pstIrdetoInfo->ucHexBase = Response[23]; 
		memcpy(pstIrdetoInfo->ucHexSerial,&Response[20],3); 
		printf("smartcardirdeto: Providers: %d HEX Serial: %02X%02X%02X  HEX Base: %02X\n", 
				pstIrdetoInfo->ucNoOfProv,pstIrdetoInfo->ucHexSerial[0],pstIrdetoInfo->ucHexSerial[1], 
				pstIrdetoInfo->ucHexSerial[2],pstIrdetoInfo->ucHexBase); 
	} 
	else 
	{ 
		// printf("Get Irdeto Hex Serial No Failed !!\n"); 
		return false; 
	} 
 
	/* Get Provide Info */ 
	for(i=(pstIrdetoInfo->ucNoOfProv-1); i>=0;i--) 
	{ 
		getProvider[4] = i; 
		r=IrdetoWrite(Handle,getProvider,Response,0,ucIrdetoStatus); 
		if(IrdetoCheckStatus(ucIrdetoStatus) && r>=33) 
		{ 
			SetProviderIrdeto(Response[8],&Response[9],i); 
			printf("Provider%d Info : ProvBase=%02x ProvId=%02x %02x %02x\n",i, 
								pstIrdetoInfo->aucProvBase[i],pstIrdetoInfo->aucProvId[i][0], 
								pstIrdetoInfo->aucProvId[i][1],pstIrdetoInfo->aucProvId[i][2]); 
		} 
		else 
		{ 
			printf("SetProviderIrdeto %d error:  r =%d\n", i, r); 
		} 
	} 
 
	/* Get Channel ID */ 
	pstIrdetoInfo->aucChannelCount = 0; 
	 
	 
	 
	for(i=0;i<pstIrdetoInfo->ucNoOfProv;i++) 
	{ 
		for(j=0;j<MAX_CH_PER_PROV;j++) 
		{ 
			memset(cmd,0,sizeof(cmd)); 
			memcpy(cmd,getChannelID,sizeof(getChannelID)); 
 
			printf("Read Channel Info ...  i=%d j=%d\n",i,j); 
			cmd[4] = i; 
			cmd[6] = j; 
 
			if(pstIrdetoInfo->aucChannelCount>MAX_IRDETO_CHANNEL)  
				break; 
			 
			r = IrdetoWrite(Handle,cmd,Response,0,ucIrdetoStatus); 
			for(k=0;k<Response[7];k+=6) 
			{ 
				if(Response[k+8]==0xFF) 
					continue; 
				Irdetofindchannelindex(&Response[k+9]); 
								 
				printf("ChannelCount=%d ChannelID:%d\n",pstIrdetoInfo->aucChannelCount, pstIrdetoInfo->aucChannelId[pstIrdetoInfo->aucChannelCount-1]); 
			} 
		} 
	} 
 
 
	/* Write Cam Key to Card */ 
	{ 
		r = IrdetoWrite(Handle,getCamKey383C,Response,0x5500,ucIrdetoStatus); 
		rlen = 9; 
	} 
	if(!(r>0 && IrdetoCheckStatus(ucIrdetoStatus) && r==rlen)) 
	{ 
		printf("Irdeto Init : Write CAM ID failed !!\n"); 
		return false; 
	} 
 
	printf("Irdeto Init OK !!\n"); 
    return true; 
} 
 
static void IrdetoRefreshProvInfo(Smart_Handle_t Handle, bool bUpdateProvInfo) 
{ 
	int i,j,k,r; 
	U8 ucIrdetoStatus[2]; 
	U8 Response[256]; 
	U8 uLastChannelCount = pstIrdetoInfo->aucChannelCount; 
	U8 cmd[100]={0}; 
 
	 
 
	/* Get Hex Serial */ 
	if(bUpdateProvInfo) 
	{ 
		/* Get Provide Info */ 
		for(i=0;i<pstIrdetoInfo->ucNoOfProv;i++) 
		{ 
			getProvider[4] = i; 
			r=IrdetoWrite(Handle,getProvider,Response,0,ucIrdetoStatus); 
			if(IrdetoCheckStatus(ucIrdetoStatus) && r>=33) 
			{ 
				SetProviderIrdeto(Response[8],&Response[9],i); 
 
			} 
			else 
			{ 
				printf("IrdetoRefreshProvInfo bUpdateProvInfo Error\n"); 
				return; 
			} 
 
		} 
	} 
	else 
	{ 
		/* Get Channel ID */ 
 
		for (i=0; i<pstIrdetoInfo->aucChannelCount;i++) 
		{ 
			pstIrdetoInfo->aucChannelId[i] = 0; 
			pstIrdetoInfo->aucDateInfo[i] = 0; 
		} 
 
		pstIrdetoInfo->aucChannelCount = 0; 
 
		for(i=0;i<pstIrdetoInfo->ucNoOfProv;i++) 
		{ 
			for(j=0;j<MAX_CH_PER_PROV;j++) 
			{ 
				memset(cmd,0,sizeof(cmd)); 
				memcpy(cmd,getChannelID,sizeof(getChannelID)); 
				/*printf("Read Channel Info ...  i=%d j=%d\n",i,j);*/ 
				cmd[4] = i; 
				cmd[6] = j; 
				if(pstIrdetoInfo->aucChannelCount>MAX_IRDETO_CHANNEL)  
					break; 
				r = IrdetoWrite(Handle,cmd,Response,0,ucIrdetoStatus); 
 
 
				if(r<=0) 
				{					 
					continue; 
				} 
 
				for(k=0;k<Response[7];k+=6) 
				{ 
					if(Response[k+8]==0xFF) 
						continue; 
					Irdetofindchannelindex(&Response[k+9]); 
				} 
			} 
		} 
	} 
} 
 
 
static bool IrdetoCheckStatus(U8* PB) 
{ 
	int i; 
	for(i=0;i<17;i++) 
	{ 
		if(PB[0]==IrdetoStatusMsgs[i].uPBWords[0] && (PB[1]==IrdetoStatusMsgs[i].uPBWords[1] || IrdetoStatusMsgs[i].uPBWords[1]==0xFF)) 
			return IrdetoStatusMsgs[i].bResult; 
	} 
	return false; 
} 
 
static U8 GetXorSum(const U8 *mem, int len) 
{ 
  U8 cs=0; 
  while(len>0) { cs ^= *mem++; len--; } 
  return cs; 
} 
 
#define XOR_START    0x3F /* Start value for xor checksum */ 
static int IrdetoWrite(Smart_Handle_t Handle,U8* cmd,U8* Response,U16 goodSB,U8* Status) 
{ 
	U16 len; 
	int r; 
	U16 Read	=0; 
	Smart_ErrorCode_t error; 
	int iRetries; 
	len = cmd[5]+6; 
	cmd[len] = GetXorSum(cmd,len)^XOR_START; 
	len += 1; 
	r =	0; 
 
	iRetries = 3; 
	do  
	{ 
		error = Smart_Transfer(Handle,cmd,len,Response,0,&Read,Status); 
	} while((Response[0] != 0x01)&&((--iRetries)>0)); 
 
	if(error==SMC_NO_ERROR) 
	{ 
		len = 4; 
		if(Response[0]==cmd[0] && Response[1]==cmd[1]) 
		{ 
			if(Status) 
			{ 
				Status[0] = Response[2]; 
				Status[1] = Response[3]; 
			} 
			if((Response[2]*256+Response[3])==goodSB) 
			{ 
				len += 5; 
				if(Response[7]) 
				{ 
					len += Response[7]; 
					if(GetXorSum(Response,len)==XOR_START) 
						r = len; 
				} 
				else 
				{ 
					r = len; 
				} 
			} 
		} 
		else 
		{ 
			if(Status) 
			{ 
				Status[0] = Response[1]; 
				Status[1] = Response[2]; 
			} 
			r = 3; 
		} 
	} 
	else 
	{ 
		printf("IrdetoWrite Error: %x========= \n", error); 
	} 
 
	 
	return r; 
} 
 
static bool IrdetoDecode(Smart_Handle_t Handle,U8* data,U8* cw) 
{ 
	int len; 
	U8 cmd[255]={0}; 
	U8 ucIrdStatus[2]; 
	U8 Response[100]; 
 
	len = data[2]-3; 
 
	memcpy(cmd,ecmCmd,sizeof(ecmCmd)); 
 
	cmd[5] = len; 
	memcpy(cmd+6,&data[6],len); 
 
	len=IrdetoWrite(Handle,cmd,Response,0x9D00,ucIrdStatus); 
	if(len>0) 
	{ 
		if(IrdetoCheckStatus(ucIrdStatus) && (len>=23)) 
		{ 
			if(Response[6]==0x02) 
			{ 
				RevCamCrypt(camKey,&Response[14]); 
				RevCamCrypt(camKey,&Response[22]); 
				memcpy(cw,&Response[14],8); 
				memcpy(cw+8,&Response[22],8); 
			} 
			return true; 
		} 
	} 
	return false; 
} 
 
 
static void SetProviderIrdeto(U8 pb, const U8 *pi,int iNum) 
{ 
	if(iNum>=11) return; 
	if((pi[0]==0x00)&&(pi[1]==0x00)) 
	{ 
	 	pstIrdetoInfo->aucProvId[iNum][0] = 0xFF; 
		pstIrdetoInfo->aucProvId[iNum][1] = 0xFF; 
		pstIrdetoInfo->aucProvId[iNum][2] = 0xFF; 
		pstIrdetoInfo->aucProvBase[iNum] = 0xFF; 
	} 
	else if((pi[0]==0xFF)&&(pi[1]==0xFF)) 
	{ 
		pstIrdetoInfo->aucProvId[iNum][0] = 0xFF; 
		pstIrdetoInfo->aucProvId[iNum][1] = 0xFF; 
		pstIrdetoInfo->aucProvId[iNum][2] = 0xFF; 
		pstIrdetoInfo->aucProvBase[iNum] = 0xFF; 
	} 
	else 
	{ 
		pstIrdetoInfo->aucProvBase[iNum]=pb; 
		memcpy(pstIrdetoInfo->aucProvId[iNum],pi,3); 
	} 
} 
 
static void RotateLeft8Byte(U8 *key) 
{ 
	U8 t1=key[7]; 
	int k; 
	for(k=0 ; k<8 ; k++)  
	{ 
		U8 t2=t1>>7; 
		t1=key[k]; key[k]=(t1<<1) | t2; 
	} 
} 
 
static void RevCamCrypt(const U8 *key, U8 *data) 
{ 
  U8 localKey[8]; 
  int idx1,idx2; 
  memcpy(localKey,key,sizeof(localKey)); 
  for(idx1=0 ; idx1<8 ; idx1++)  
  { 
	  for(idx2=0 ; idx2<8 ; idx2++)  
	  { 
		  const U8 tmp1=cryptTable[data[7] ^ localKey[idx2] ^ idx1]; 
		  const U8 tmp2=data[0]; 
		  data[0]=data[1]; 
		  data[1]=data[2]; 
		  data[2]=data[3]; 
		  data[3]=data[4]; 
		  data[4]=data[5]; 
		  data[5]=data[6] ^ tmp1; 
		  data[6]=data[7]; 
		  data[7]=tmp1 ^ tmp2 ; 
	  } 
	  RotateLeft8Byte(localKey); 
  } 
} 
 
bool irdeto_parse_ecm(Smart_Handle_t Handle,U8* pbuf,U8* pucCW) 
{ 
	U8 cw[16]; 
 
	if(IrdetoDecode(Handle,pbuf,cw)) 
	{ 
		cw[3] = cw[0]+cw[1]+cw[2]; 
		cw[7] = cw[4]+cw[5]+cw[6]; 
		cw[11] = cw[8]+cw[9]+cw[10]; 
		cw[15] = cw[12]+cw[13]+cw[14]; 
		memcpy(pucCW,cw,16); 
		return true; 
	}else 
	{ 
		printf("Irdeto Ecm Error : IrdetoDecode failed !!\n"); 
	} 
 
	return false; 
} 
 
unsigned int AddrLen(const U8 *data) 
{ 
  return data[3] & 0x07; 
} 
 
unsigned int AddrBase(const U8 *data) 
{ 
  return data[3] >> 3; 
} 
 
 
bool irdeto_parse_emm(Smart_Handle_t Handle,U8* buf,int length) 
{ 
	int i,len,mod,r; 
	bool accept,bIsHexSerial,bIsProviderUp; 
	U8 aucEmmCmd[255]; 
	U8 ucIrdetoStatus[2]; 
	U8 Response[255]; 
 
	/* Calculate which mode will be used */ 
	len = buf[0]&0x07; 
	mod = buf[0]>>3; 
	accept = false; 
	bIsHexSerial = false; 
	bIsProviderUp = false; 
 
	/* Check EMM Authorize */ 
	if(mod&0x10) 
	{ 
		/* Hex Address Case */ 
		if(mod==pstIrdetoInfo->ucHexBase && (!len || !memcmp(&buf[1],pstIrdetoInfo->ucHexSerial,3))) 
		{ 
			accept = true; 
			bIsHexSerial = true; 
		} 
		else 
		{ 
			printf("mod !=pstIrdetoInfo->ucHexBase %02x: %02x ?  \n", mod, pstIrdetoInfo->ucHexBase); 
			printf("buf %02x %02x %02x \n", buf[1],buf[2],buf[3]); 
			printf("pstIrdetoInfo->ucHexSerial %02x %02x %02x \n", pstIrdetoInfo->ucHexSerial[1], pstIrdetoInfo->ucHexSerial[2], pstIrdetoInfo->ucHexSerial[3]); 
 
		} 
	} 
	else 
	{ 
		/* Provider Address Case */ 
		for(i=0;i<pstIrdetoInfo->ucNoOfProv;i++) 
		{ 
#if 0 
			if((!len || !memcmp(&buf[1],pstIrdetoInfo->aucProvId[i],2))) /* Not 3 !! */ 
#else 
			if(len != 0||!memcmp(&buf[1],pstIrdetoInfo->aucProvId[i],2)||buf[2]==0) 
#endif 
 
			{ 
				accept = true; 
				bIsProviderUp = true; 
				break; 
			} 
		} 
	} 
 
	/* if accept,send emm to card */ 
	if(accept) 
	{ 
		len++; 
		memset(aucEmmCmd,0,sizeof(aucEmmCmd)); 
		memcpy(aucEmmCmd,emmCmd,sizeof(emmCmd)); 
 
		/* HEX Processing */ 
		if(mod&0x10) 
		{ 
			aucEmmCmd[5] = length-1; 
			memcpy(aucEmmCmd+6,buf,4); 
			memcpy(aucEmmCmd+10,buf+6,length-5); 
		} 
		else 
		{ 
			aucEmmCmd[5] = length-1; 
			if(len==3) 
			{ 
				memcpy(aucEmmCmd+6,buf,4); 
				memcpy(aucEmmCmd+10,buf+5,aucEmmCmd[5]-3); 
			} 
			else 
			{ 
				memcpy(aucEmmCmd+6,buf,4); 
				memcpy(aucEmmCmd+10,buf+6,aucEmmCmd[5]-4); 
			} 
 
			/* Adjust length filed, Very important !! */ 
			if(len==3)  
				aucEmmCmd[5]++; 
		} 
 
		/* Send Cmd to Card */ 
		r=IrdetoWrite(Handle,aucEmmCmd,Response,0,ucIrdetoStatus); 
		if((r>0 )&& IrdetoCheckStatus(ucIrdetoStatus))  
		{ 
			if(bIsHexSerial) 
			{ 
				IrdetoRefreshProvInfo(Handle,true); 
 
			} 
			if(bIsProviderUp) 
			{ 
				IrdetoRefreshProvInfo(Handle,false); 
			} 
			return true; 
		} 
		else 
		{ 
			printf("Error r= %d , bIsHexSerial = %d, bIsProviderUp = %d", r, bIsHexSerial, bIsProviderUp); 
			printf("pstIrdetoInfo->aucProvId = %02x %02x %02x", buf[1],buf[2],buf[3]); 
			printf("pstIrdetoInfo->aucProvId = %02x %02x %02x", pstIrdetoInfo->aucProvId[i][0], pstIrdetoInfo->aucProvId[i][1],pstIrdetoInfo->aucProvId[i][2]); 
 
		} 
	} 
	else 
	{ 
		printf("======not accept!!!\n"); 
	} 
	return false; 
} 
