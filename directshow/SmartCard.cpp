#include "stdafx.h"
#include "SmartCard.h"

SmartCard::SmartCard(SmartCardReader* reader)
	: m_reader(reader)
	, m_system_id(0xffff)
{
}

SmartCard::~SmartCard()
{
}

//

ConaxCard::ConaxCard(SmartCardReader* reader)
	: SmartCard(reader)
{
}

bool ConaxCard::Init()
{
	memset(&m_conax, 0, sizeof(m_conax)); 
	memset(&m_access, 0, sizeof(m_access));
 
	if(!InitCASS()) 
	{ 
		printf("Conax Init : Init CASS failed !!\n");

		return false; 
	} 
   
	if(!ResetSESS(0)) 
	{ 
		printf("Conax Init : Reset SESS failed !!\n"); 

		return false; 
	} 

 	if(!CAStatusSelect(0x00, NULL)) 
 	{ 
 		printf("Conax Init : Select Status 0 failed !!\n"); 

 		return false; 
 	} 

 	if(!CAStatusSelect(0x01, NULL)) 
 	{ 
 		printf("Conax Init : Select Status 1 failed !!\n"); 

 		return false; 
 	} 

	if(!ReqCardNumber()) 
	{ 
		printf("Conax Init : Read Card Number failed !!\n"); 

		return false; 
	} 

	printf("Conax CA init OK!\n"); 

	return true; 
}

std::wstring ConaxCard::GetSerial() const 
{
	return Util::Format(L"%d", m_conax.iCardNumber);
}

std::wstring ConaxCard::GetSystemName() const 
{
	return L"Conax";
}

void ConaxCard::GetSubscriptions(std::list<SmartCardSubscription>& subscriptions)
{
	subscriptions.clear();

	for(auto i = m_subscription.begin(); i != m_subscription.end(); i++)
	{
		SmartCardSubscription s;

		s.id = i->second.id;
		s.name = std::wstring(i->second.name.begin(), i->second.name.end());

		for(int index = 0; index < 4; index++)
		{
			CTime t(i->second.GetYear(index), i->second.GetMonth(index), i->second.GetDay(index), 0, 0, 0);

			s.date.push_back(t);
		}

		subscriptions.push_back(s);
	}
}

bool ConaxCard::ParseCAT(const BYTE* pBuf) 
{ 
	int len = ((pBuf[1] & 0x0F) << 8) + pBuf[2] + 3; 

	if(len > 187) return false;
	
	BYTE Command[255] = {0xDD, 0x82, 0x00, 0x00, len + 2, 0x11, len}; 

	memcpy(Command + 7, pBuf, len); 
	
	return TransferWrite(Command, len + 7); 
} 

bool ConaxCard::ParseEMM(const BYTE* pBuf) 
{ 
	bool found = false;

	const BYTE* p = &pBuf[3];

	for(int i = 0; i < m_conax.iEMMAddressCount; i++)
	{
		if(memcmp(m_conax.auEMMAddress[i], p, 7) == 0)
		{
			found = true;
		}
	}

	if(0)//!found)
	{
		DWORD serial = 0;

		for(int i = 0; i < 4; i++)
		{
			serial = (serial << 8) | p[i + 3];
		}

		if(serial == m_conax.iCardNumber && serial != (m_conax.iCardNumber >> 9))
		{
			found = true;
		}
	}

	if(!found) return false;

	wprintf(L"EMM %02x%02x%02x%02x%02x%02x%02x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6]);

	int len = ((pBuf[1] & 0x0F) << 8) + pBuf[2] + 3; 

	if(len > 187) return false;

	BYTE Command[300] = {0xDD, 0x84, 0x00, 0, len + 2, 0x12, len}; 
 
	memcpy(Command + 7, pBuf, len); 

	return TransferWrite(Command, len + 7); 
} 

bool ConaxCard::ParseECM(const BYTE* pBuf, BYTE* pucCW, bool& gotcw) 
{
	int len = ((pBuf[1] & 0x0F) << 8) + pBuf[2] + 3; 

	if(len > 187) return false;
	
	/* Pay attention to ECM mode indicate bit! */  

	BYTE Command[300] = {0xDD, 0xA2, 0x00, 0x00, len + 3, 0x14, len + 1, 0x00};

	memcpy(Command + 8, pBuf, len); /* Copy the ECM data to the command */ 

	BYTE Response[255]; 
	WORD	Read; 
	BYTE Status[2]; 

	bool ret = false;

	if(m_reader->Transfer(Command, len + 8, Response, &Read, Status))
	{
		BYTE* buf = Response;

		if(Status[0] = 0x98)  
		{ 
			if(TransferRead(Response, Status[1], &Read, Status, 0)) 
			{
				ret = true;

				int iParsedLength = 0; 

				while(iParsedLength < Read) 
				{ 
					BYTE pi = buf[0]; 
					int iLength = buf[1]; 

					/* Each data have a pi parameter, we use this to identify them */ 

					switch(pi)  
					{ 
						/* The following four objects can be received after ECM send */ 

						case CW_PI: 
							if(iLength < 5) return false; 
							memcpy(pucCW + (buf[4] == 0x00 ? 0 : 8), buf + 7, 8); 
							gotcw = true;
							break; 

						case ACCESS_STATUS_PI: 
							ParseAccessStatus(buf);
							break; 
					} 

					iParsedLength += iLength + 2; 
					buf += iLength + 2; 
				} 
			} 
		} 
	}
	else
	{
		printf("ParseECM ???\n");
	}
	
	return ret; 
} 

bool ConaxCard::TransferRead(BYTE* Buffer, WORD NumberToRead, WORD* Read, BYTE* Status, BYTE sid)
{
	BYTE Command[] = {0xDD, 0xCA, 0x00, sid, (BYTE)NumberToRead}; /* ISO part of Read Command */ 

	return m_reader->Transfer(Command, sizeof(Command), Buffer, Read, Status);
}

bool ConaxCard::TransferWrite(BYTE* Buffer, WORD NumberToWrite) 
{
	BYTE Status[2]; 
	BYTE Response[255]; 
	WORD Read; 

	bool b = false;
	/*
		Buffer[0] == 0xdd && 
		Buffer[1] == 0xc6 &&  
		Buffer[2] == 0x00 && 
		Buffer[3] == 0x00 && 
		Buffer[4] == 0x03 && 
		Buffer[5] == 0x1c && 
		Buffer[6] == 0x01;
	*/

	if(!m_reader->Transfer(Buffer, NumberToWrite, NULL, NULL, Status)) 
	{
		printf("Conax Write Failed !!\n"); 

		return false;
	}

	if(b) printf("* Write Status = %02x %02x\n", Status[0], Status[1]);

	if(Status[0] == 0x90 && Status[1] == 0x00)
	{
		return true;
	}

	while(Status[0] == 0x98)
	{
		// TODO: concat buffers before ParseResp?

		if(!TransferRead(Response, Status[1], &Read, Status, 0)) 
		{
			printf("Conax Read Failed !!\n"); 

			return false;
		}

		if(b) printf("* Read Status = %02x %02x (n = %d)\n", Status[0], Status[1], Read);

		if(!ParseResp(Response, Read))
		{
			printf("Conax Parse Response Failed !!\n"); 

			return false;
		}
	}

	return true; // Status[0] == 0x90 && Status[1] == 0x00;
}

bool ConaxCard::InitCASS()
{
	BYTE Command[] = {0xDD, 0x26, 0x00, 0x00, 0x03, 0x10, 0x01, 0x40};

	return TransferWrite(Command, sizeof(Command)); 
}

bool ConaxCard::ResetSESS(BYTE sid)
{
	BYTE Command[] = {0xDD, 0x26, 0x00, 0x00, 0x03, 0x69, 0x01, sid};

	return TransferWrite(Command, sizeof(Command)); 
}

bool ConaxCard::ParseResp(BYTE* buff, WORD len)
{
	int iLength = 0; 
	WORD iTmpLen = 0; 
	WORD iOctLen = 0; 
	int iParsedLength = 0; 
	Conax_CW ConaxCW; 
 
	while(iParsedLength < len) 
	{ 
		iLength = buff[1]; 
 
		/* Each data have a pi parameter,we use this to identify them */ 

		switch(buff[0])  
		{ 
			/* The following five objects can be received after INIT_CASS command */

			case CASS_VER_PI: 
 				if(iLength != 1) return false; 
				m_conax.stParam.Version = buff[2]; 
				break; 

			case CA_SYS_ID_PI: 
				if(iLength != 2) return false; 
				m_conax.stParam.CaSysId = (buff[2] << 8) | buff[3]; 
				m_system_id = m_conax.stParam.CaSysId;
				break; 

			case COUNTRY_INDICATOR_PI: 
				if(iLength != 2) return false; 
				m_conax.stParam.CountryIndicatorValue = (buff[2] << 8) | buff[3]; 
				break; 

			case MAT_RAT_LEVEL_PI: 
				if(iLength != 1) return false; 
				m_conax.stParam.MatRat = buff[2]; 
				break; 

			case SESSION_INFO_PI: 
				if(iLength != 1) return false; 
				m_conax.stParam.MaxSessionNum = buff[2]; 
				break; 
			 
			/* This Object can be received after EMM or ECM send */

			case HOST_DATA_PI: 
				break; 
 
			/* This Object can be received after CAT send */

			case CA_DESC_EMM_PI: 
				if(!ParseCADescEMM(buff)) return false; 
				break; 
			 
			/* The following four objects can be received after ECM send */ 

			case ACCESS_STATUS_PI: 
				if(!ParseAccessStatus(buff)) return false; 
				break; 

			case CW_PI: 
				if(iLength < 5) return false; 
				memset(&ConaxCW, 0, sizeof(Conax_CW)); 
				ConaxCW.cw_id = (buff[2] << 8) | buff[3]; 
				ConaxCW.odd_even_indicator = buff[4]; 
				ConaxCW.iCWLen = iLength - 5; 
				memset(ConaxCW.aucCWBuf, 0, ConaxCW.iCWLen); 
				memcpy(ConaxCW.aucCWBuf, buff + 7, ConaxCW.iCWLen); 
				break; 
				 
			/* This object can be received when Req Card Number Command is send */ 

			case CARD_NUMBER_PI: 
				if(iLength != 4) return false; 
				m_conax.iCardNumber = (buff[2] << 24) | (buff[3] << 16) | (buff[4] << 8) | buff[5]; 
				break; 

			// ???

			case SUBSCRIPTION_PI: 
				ParseSubscription(&buff[2], iLength);
				break;
		}

		iParsedLength += iLength + 2; 
		buff += iLength + 2; 
	} 

	return true;
}

bool ConaxCard::ParseCADescEMM(BYTE* pBuf)
{
	int length; 
	int iAddressLength; 
	int iDescLength; 
	int iParsedAddLen; 
	BYTE  EMM_Address_TABLE[10][7]; 
	int EMM_Address_Count; 

	length = pBuf[1]; 
	iDescLength = pBuf[3]; 
	iAddressLength = length - iDescLength - 2; 
	pBuf += iDescLength + 4; 
	EMM_Address_Count = 0; 
	iParsedAddLen = 0; 

	while(iParsedAddLen < iAddressLength) 
	{ 
		if(pBuf[0] == EMM_ADDRESS_TAG && pBuf[1] == 7) 
		{ 
			memcpy(EMM_Address_TABLE[EMM_Address_Count], pBuf + 2, 7); 
			EMM_Address_Count++; 
			iParsedAddLen += 9; 
			pBuf += 9; 
		} 
		else 
		{ 
			return false; 
		} 
	} 
 
	m_conax.iEMMAddressCount = EMM_Address_Count; 
	
	for(int i = 0; i < EMM_Address_Count; i++)
	{
		memcpy(m_conax.auEMMAddress[i], EMM_Address_TABLE[i], 7);
	}

	return EMM_Address_Count >= 1;
}

bool ConaxCard::ParseAccessStatus(BYTE* pBuf)
{
	int len = pBuf[1]; 
	int expectedlength = 2; 

	if(len < 2 || len == 2 && (pBuf[2] == 0x00) && (pBuf[3] == 0x00))
	{
		return false; 
	}
	 
	pBuf += 2;

	m_access.not_sufficient_info	= (pBuf[0] & 0x80) >> 7; 
	m_access.access_granted	   		= (pBuf[0] & 0x40) >> 6; 
	m_access.order_ppv_event		= (pBuf[0] & 0x20) >> 5; 
	m_access.geographical_blackout	= (pBuf[0] & 0x04) >> 2; 
	m_access.maturity_rating		= (pBuf[0] & 0x02) >> 1; 
	m_access.accept_viewing			= (pBuf[0] & 0x01) >> 0; 
	m_access.ppv_preview			= (pBuf[1] & 0x80) >> 7; 
	m_access.ppv_expried			= (pBuf[1] & 0x40) >> 6; 
	m_access.no_access_to_network	= (pBuf[1] & 0x20) >> 5; 
 
	pBuf += 2; 

	if(m_access.maturity_rating) 
	{ 
		m_access.current_mat_rat_value = pBuf[0]; 

		pBuf++; 
		expectedlength++; 
	} 
	if(m_access.accept_viewing) 
	{ 
		m_access.minutes_left = (pBuf[0] << 8) | pBuf[1]; 

		pBuf += 2; 
		expectedlength += 2; 
	} 
	if(m_access.order_ppv_event || m_access.accept_viewing) 
	{ 
		memcpy(m_access.product_ref, pBuf, 6); 

		expectedlength += 6; 
	} 
 
	return len >= expectedlength;
}

bool ConaxCard::ParseSubscription(BYTE* p, int len)
{
	Conax_Subscription s;

	s.id = (p[0] << 8) | p[1];

	int pbm = 0;
	int date = 0;

	for(int i = 2; i < len; i += p[i + 1] + 2)
	{
		switch(p[i + 0])
		{
		case 0x01:
			s.name = std::string((char*)&p[i + 2], p[i + 1]);
			break;
		case 0x20:
			if(pbm < 2) s.pbm[pbm++] = (p[i + 2] << 24) | (p[i + 3] << 16) | (p[i + 4] << 8) | p[i + 5];
			break;
		case 0x30:
			if(date < 4) s.date[date++] = (p[i + 2] << 8) | p[i + 3];
			break;
		}
	}

	printf("%s: ", s.name.c_str());

	for(int i = 0; i < date; i++)
	{
		printf("%04d-%02d-%02d ", s.GetYear(i), s.GetMonth(i), s.GetDay(i));
	}

	printf("\n");

	s.name = Util::Trim(s.name);

	m_subscription[s.id] = s;

	return true;
}

bool ConaxCard::ReqCardNumber()
{
	BYTE Command[] = {0xDD, 0xC2, 0x00, 0x00, 0x02, 0x66, 0x00}; 

	return TransferWrite(Command, sizeof(Command)); 
}

bool ConaxCard::CAStatusSelect(BYTE type, BYTE* pBuf)
{
	BYTE Command[100] = {0x1C, 0xFF, type};
	BYTE len; 
 
	if(type == 0x05) 
	{ 
		if(pBuf == NULL) 
		{ 
			printf("ConaxCAStatusSelect : buf should not be NULL !\n"); 

			return false; 
		} 

		len = 5;
		Command[1] = 3;  
		memcpy(Command + 3, pBuf, 2); 
	} 
	else if(type == 0x06) 
	{ 
		if(pBuf == NULL) 
		{ 
			printf("ConaxCAStatusSelect : buf should not be NULL !\n"); 

			return false; 
		}
		
		len = 6;
		Command[1] = 4; 
		memcpy(Command + 3, pBuf, 3); 
	} 
	else 
	{ 
		len = 3;
		Command[1] = 1; 
	}

	return CAStatusCom(0, Command, len); 
}

bool ConaxCard::CAStatusCom(BYTE type, BYTE* pBuf, BYTE len)
{
	BYTE Command[255] = {0xDD, 0xC6, 0x00, 0x00, len}; 

	memcpy(Command + 5, pBuf, len); 
	
	return TransferWrite(Command, len + 5); 
}

//

CryptoWorksCard::CryptoWorksCard(SmartCardReader* reader)
	: SmartCard(reader)
{
	memset(m_serial, 0, sizeof(m_serial));
	memset(&m_issuer, 0, sizeof(m_issuer));
}

std::wstring CryptoWorksCard::GetSerial() const 
{
	return Util::Format(L"%02x%02x%02x%02x%02x", m_serial[0], m_serial[1], m_serial[2], m_serial[3], m_serial[4]);
}

std::wstring CryptoWorksCard::GetSystemName() const 
{
	return L"Cryptoworks";
}

bool CryptoWorksCard::Init()
{
	BYTE buff[256];

	if(!SelectFile(0x2f, 0x01))
	{
		return false;
	}
	
	if(ReadRecord(0xd1, buff) < 4)
	{
		return false;
	}

	m_system_id = (buff[2] << 8) | buff[3];

	if(ReadRecord(0x80, buff) < 7)
	{
		return false;
	}

	memcpy(m_serial, &buff[2], 5);

	if(ReadRecord(0x9f, buff) < 3)
	{
		return false;
	}

	m_issuer.id = buff[2];

	if(ReadRecord(0xc0, buff) < 16)
	{
		return false;
	}

	memcpy(m_issuer.name, &buff[2], 14);

	printf("CryptoWorks CA init OK! (id: %04x, issuer: %s (%02x), serial: %02x%02x%02x%02x%02x)\n", 
		m_system_id,
		m_issuer.name, m_issuer.id,
		m_serial[0], m_serial[1], m_serial[2], m_serial[3], m_serial[4]); 
	
	{
		std::vector<BYTE> providers;

		BYTE insB8[] = {0xA4, 0xB8, 0x00, 0x00, 0x0C};
		WORD	read = 0;
		BYTE status[2]; 

		while(m_reader->Transfer(insB8, sizeof(insB8), buff, &read, status) && status[0] == 0xdf && providers.size() < 16)
		{
			if(buff[0] == 0xdf && buff[1] == 0x0a)
			{
				int fileno = ((buff[4] & 0x3f) << 8) | buff[5];

				if((fileno & 0xff00) == 0x1f00)
				{
					providers.push_back(fileno & 0xff);
				}
			}

			insB8[2] = insB8[3] = 0xFF;
		}

		for(auto i = providers.begin(); i != providers.end(); i++)
		{
			if(SelectFile(0x1f, *i))
			{
				char* name = "(unknown)";

				if(SelectFile(0x0e, 0x11) && ReadRecord(0xd6, buff) >= 18)
				{
					name = (char*)&buff[2];
				}

				printf("%s\n", name);

				BYTE insA2_0[] = {0xA4, 0xA2, 0x01, 0x00, 0x03, 0x83, 0x01, 0x00};
				BYTE insA2_1[] = {0xA4, 0xA2, 0x01, 0x00, 0x05, 0x8C, 0x00, 0x00, 0x00, 0x00};
				BYTE insB2[] = {0xA4, 0xB2, 0x00, 0x00, 0x00};
				BYTE fn[] = {0x00, 0x20, 0x40, 0x60};

				for(int j = 0; j < sizeof(fn) / sizeof(fn[0]); j++)
				{
					if(!SelectFile(0x0f, fn[j])) continue;
					
					insA2_0[7] = *i & 0xff;

					BYTE* cmd = j == 1 ? insA2_1 : insA2_0;
					WORD size = j == 1 ? sizeof(insA2_1) : sizeof(insA2_0);

					if(m_reader->Transfer(cmd, size, NULL, NULL, status) && status[0] == 0x9f)
					{
						insB2[3] = 0;
						insB2[4] = status[1];

						while(m_reader->Transfer(insB2, sizeof(insB2), buff, &read, status) && read > 2)
						{
							struct chid_dat
							{
								DWORD chid, version;
								BYTE id, status;
								char from[16];
								char to[16];
								char name[16];
							} chid;

							memset(&chid, 0, sizeof(chid));

							for(int i = 0; i < read; )
							{
								int n = buff[i + 1];
								BYTE* p = &buff[i + 2];

								switch(buff[i])
								{
								case 0x83:
									chid.id = p[0];
									break;
								case 0x8c:
									chid.status = p[0];
									chid.chid = (p[1] << 8) | p[2];
									break;
								case 0x8d:
									_snprintf(chid.from, sizeof(chid.from), "%02d.%02d.%02d", p[0] & 0x1f, ((p[0] & 1) << 3) + (p[1] >> 5), (1990 + (p[0] >> 1)) % 100);
									_snprintf(chid.to, sizeof(chid.to), "%02d.%02d.%02d", p[2] & 0x1f, ((p[2] & 1) << 3) + (p[3] >> 5), (1990 + (p[2] >> 1)) % 100);
									break;
								case 0xd5:
									_snprintf(chid.name, sizeof(chid.name), "%.12s", &p[0]);
									chid.version = (p[12] << 24) | (p[13] << 16) | (p[14] << 8) | p[15];
									break;
								}

								i += n + 2;
							}

							printf("%02x | %04x |  %02x  | %s-%s | %08x | %.12s\n", chid.id, chid.chid, chid.status, chid.from, chid.to, chid.version, chid.name);

							insB2[3] = 1;
						}
					}
				}
			}
		}
	}

	return true; 
}

bool CryptoWorksCard::ParseCAT(const BYTE* pBuf) 
{ 
	int len = ((pBuf[1] & 0x0F) << 8) + pBuf[2] + 3; 

	if(len > 187) return false;
	/*
	BYTE Command[255] = {0xDD, 0x82, 0x00, 0x00, len + 2, 0x11, len}; 

	memcpy(Command + 7, pBuf, len); 
	
	return TransferWrite(Command, len + 7); 
	*/
	return false;
} 

bool CryptoWorksCard::ParseEMM(const BYTE* pBuf) 
{
	BYTE cmd[256 + 5] = {0xA4, 0x00, 0x00, 0x00, 0x00};
	BYTE status[2];

	const BYTE* p = pBuf;

	while(p - pBuf < 188)
	{
		switch(p[0])
		{
		case 0x82: // UA
			if(p[3] != 0xa9 || p[4] != 0xff || p[12] != p[2] - 10 || p[13] != 0x80 || p[14] != 0x05) return false;
			cmd[1] = 0x42;
			cmd[4] = p[2] - 7;
			memcpy(&cmd[5], &p[10], cmd[4]);
			if(CheckSerial(&p[5]))
			if(!m_reader->Transfer(cmd, cmd[4] + 5, NULL, NULL, status) || status[0] != 0x90 || status[1] != 0x00) return false;
			break;
		case 0x84: // SA
			if(p[3] != 0xa9 || p[4] != 0xff || p[11] != p[2] - 9 || p[12] != 0x80 || p[13] != 0x04) return false;
			cmd[1] = 0x48;
			cmd[4] = p[2] - 6;
			memcpy(&cmd[5], &p[9], cmd[4]);
			if(CheckSerial(&p[5]))
			if(!m_reader->Transfer(cmd, cmd[4] + 5, NULL, NULL, status) || status[0] != 0x90 || status[1] != 0x00) return false;
			break;
		default:
			return false;
		}

		p += p[2];
	}

	return true;
} 

bool CryptoWorksCard::ParseECM(const BYTE* pBuf, BYTE* pucCW, bool& gotcw) 
{
	int len = ((pBuf[1] & 0x0F) << 8) + pBuf[2] + 3; 

	if(len > 187) return false;

	pBuf += 5;
	len -= 5;
	
	BYTE ins4C[300] = {0xA4, 0x4C, 0x00, 0x00, len};

	memcpy(ins4C + 5, pBuf, len);

	ins4C[5] = 0x28; // ???

	BYTE status[2]; 

	if(!m_reader->Transfer(ins4C, len + 5, NULL, NULL, status) || status[0] != 0x9f)
	{
		return false;
	}

	BYTE insC0[] = {0xA4, 0xC0, 0x00, 0x00, status[1]};

	BYTE buff[255]; 
	WORD	read = 0;

	memset(buff, 0, sizeof(buff));

	if(!m_reader->Transfer(insC0, sizeof(insC0), buff, &read, status))
	{
		return false;
	}

	gotcw = false;

	for(int i = 0; i < read; )
	{
		int n = buff[i + 1];

		switch(buff[i])
		{
		case 0xDB: // CW

			if(n == 16) 
			{
				memcpy(pucCW, &buff[i + 2], 16); 
				/*
				memcpy(pucCW, &buff[i + 2], 8); 
				memcpy(pucCW + 8, &buff[i + 2 + 8], 8); 
				*/
				/*
				for(int j = 0; j < 8; j++)
				{
					pucCW[j + 8] = buff[i + 2 + 7 - j];
					pucCW[j] = buff[i + 2 + 8 + 7 - j];
				}
				*/
				
				for(int j = 0; j < 8; j++) printf("%02x", pucCW[j]);
				printf(" ");
				for(int j = 0; j < 8; j++) printf("%02x", pucCW[j + 8]);
				printf("\n");

				gotcw = true;
			}

			break;

		case 0xDF: // signature

			if(n == 8 && (buff[i + 2] & 0x50) == 0x50 && (buff[i + 3] & 0x01) == 0 && (buff[i + 5] & 0x80) != 0)
			{
				bool gotsig = true;
			}

			break;
		}
		
		i += n + 2;
	}

	return gotcw; // && gotsig
} 

bool CryptoWorksCard::SelectFile(BYTE b0, BYTE b1)
{
	BYTE insA4[] = {0xA4, 0xA4, 0x00, 0x00, 0x02, b0, b1};

	BYTE status[2];

	m_reader->Transfer(insA4, sizeof(insA4), NULL, NULL, status);
  
	return status[0] == 0x9f && status[1] == 0x11;
}

int CryptoWorksCard::ReadRecord(BYTE rec, BYTE* buff)
{
	BYTE insA2[] = {0xA4, 0xA2, 0x00, 0x00, 0x01, rec};

	BYTE status[2];

	if(!m_reader->Transfer(insA2, sizeof(insA2), NULL, NULL, status) || status[0] != 0x9f)
	{
		return -1;
	}

	BYTE insB2[] = {0xA4, 0xB2, 0x00, 0x00, status[1]};

	WORD read = 0;

	if(!m_reader->Transfer(insB2, sizeof(insB2), buff, &read, status) || buff[read - 2] != 0x90 || buff[read - 1] != 0)
	{
		return -1;
	}

	return read - 2;
}

bool CryptoWorksCard::CheckSerial(const BYTE* p)
{
	return memcmp(p, m_serial, 5) == 0;
}

//

SmartCardReader::SmartCardReader() 
	: m_sc(NULL) 
	, m_last_connect(0)
{
}

SmartCardReader::~SmartCardReader()
{
	delete m_sc;
}

bool SmartCardReader::Connect()
{
	delete m_sc;

	m_sc = NULL;

	int size = m_atr.size();
	BYTE* buff = m_atr.data();

	SmartCard* sc = NULL;

	if(size == 7 && buff[0] == 0x3B && buff[1] == 0x24
	|| size == 8 && buff[0] == 0x3B && buff[1] == 0x34) 
	{ 
		sc = new ConaxCard(this);
	}
	else if(size >= 11 && buff[6] == 0xC4 && buff[9] == 0x8F && buff[10] == 0xF1)
	{
		sc = new CryptoWorksCard(this);
	}
	/*
	else if(size == 12 && buff[0] == 0x3F && buff[1] == 0x77 && buff[10] == 0x90 && buff[11] == 0x00
	|| size == 11 && buff[0] == 0x3F && buff[1] == 0x67 && buff[9] == 0x90 && buff[10] == 0x00) 
	{
		sc = new ViaccessCard(this);
	} 
	else if(size == 20 && buff[0] == 0x3B && buff[1] == 0x9F) 
	{ 
		sc = new IrdetoCard(this);
	}
    else if(size == 8 && buff[0] == 0x3B && buff[1] == 0x64
	|| size == 16 && buff[0] == 0x3B && buff[1] == 0x6C)  
	{ 
		sc = new YxtfCard(this);
	} 
	else if(size == 18 && buff[0] == 0x3B && buff[1] == 0xE9) 
	{ 
		sc = new BjstCard(this);
	} 
	else if(size == 4 && buff[0] == 0x3B && buff[1] == 0x02) 
	{ 
		sc = new SmsxCard(this);
	}
	*/

	if(sc == NULL || !sc->Init())
	{
		delete sc;

		return false;
	}

	m_sc = sc;

	return true;
}

void SmartCardReader::Disconnect()
{
	delete m_sc;

	m_sc = NULL;
}

bool SmartCardReader::HasCard()
{
	return m_sc != NULL;
}

bool SmartCardReader::ParseCAT(SmartCardPacket* packet)
{
	return m_sc != NULL && m_sc->ParseCAT(packet->buff.data());
}

bool SmartCardReader::ParseEMM(SmartCardPacket* packet)
{
	return m_sc != NULL && m_sc->ParseEMM(packet->buff.data());
}

bool SmartCardReader::ParseECM(SmartCardPacket* packet, bool& gotcw)
{
	return m_sc != NULL && m_sc->ParseECM(packet->buff.data(), packet->cw, gotcw);
}

bool SmartCardReader::ParseDCW(SmartCardPacket* packet)
{
	return m_sc != NULL && m_sc->ParseDCW(packet->buff.data());
}

//

WinSCardReader::WinSCardReader()
	: m_context(NULL)
	, m_card(NULL)
{
}

WinSCardReader::~WinSCardReader()
{
	if(m_card) SCardDisconnect(m_card, SCARD_RESET_CARD);
	if(m_context) SCardReleaseContext(m_context);
}

bool WinSCardReader::Create(int index)
{
	if(m_card) {SCardDisconnect(m_card, SCARD_RESET_CARD); m_card = NULL;}
	if(m_context) {SCardReleaseContext(m_context); m_context = NULL;}	

	SCARDCONTEXT context;

	if(SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &context) != SCARD_S_SUCCESS)
	{
		return false;
	}

	LPWSTR readers = NULL;
	DWORD size = SCARD_AUTOALLOCATE;

	if(SCardListReaders(context, NULL, (LPWSTR)&readers, &size) != SCARD_S_SUCCESS)
	{
		SCardReleaseContext(context);

		return false;
	}

	m_name.clear();

	for(LPCWSTR s = readers; *s; s += wcslen(s) + 1)
	{
		if(index-- == 0)
		{
			m_name = s;

			break;
		}
	}
	
	SCardFreeMemory(context, readers);

	if(m_name.empty())
	{
		SCardReleaseContext(context);

		return false;
	}

	m_context = context;

	return true;
}

bool WinSCardReader::Connect()
{
	if(clock() - m_last_connect < 1500)
	{
		return false;
	}

	m_last_connect = clock();

	DWORD protocol;

	if(m_card == NULL)
	{
		SCARDHANDLE card;

		if(SCardConnect(m_context, m_name.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, &card, &protocol) != SCARD_S_SUCCESS)
		{
			return false;
		}

		m_card = card;
	}
	else
	{
		if(SCardReconnect(m_card, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, SCARD_RESET_CARD, &protocol) != SCARD_S_SUCCESS)
		{
			return false;
		}
	}

	if(protocol != SCARD_PROTOCOL_T0)
	{
		return false;
	}

	m_atr.clear();

	BYTE buff[256];
	DWORD size = sizeof(buff);

	if(SCardGetAttrib(m_card, SCARD_ATTR_ATR_STRING, buff, &size) != SCARD_S_SUCCESS)
	{
		return false;
	}

	m_atr.resize(size);

	memcpy(m_atr.data(), buff, size);

	return __super::Connect();
}

void WinSCardReader::Disconnect()
{
	if(m_card != NULL)
	{
		SCardDisconnect(m_card, SCARD_RESET_CARD); // SCARD_LEAVE_CARD? (reconnect will reset, too)
		
		m_card = NULL;
	}

	__super::Disconnect();
}

bool WinSCardReader::HasCard()
{
	if(m_card == NULL)
	{
		return false;
	}

	BYTE buff[1];
	DWORD size = sizeof(buff);

	if(SCardGetAttrib(m_card, SCARD_ATTR_ICC_PRESENCE, buff, &size) != SCARD_S_SUCCESS || buff[0] != 2)
	{
		Disconnect();

		return false;
	}

	return __super::HasCard();
}

bool WinSCardReader::Transfer(BYTE* pucCommand, WORD uNumberToWrite, BYTE* pucResponse, WORD* pusNumberRead, BYTE* pucPBWords)
{
	BYTE buff[256];
	DWORD size = sizeof(buff);

	DWORD res = SCardTransmit(m_card, SCARD_PCI_T0, pucCommand, uNumberToWrite, NULL, buff, &size);

	if(res == SCARD_S_SUCCESS && size >= 2)
	{
		if(pusNumberRead)
		{
			*pusNumberRead = (WORD)size;

			memcpy(pucResponse, buff, size);
		}

		pucPBWords[0] = buff[0];
		pucPBWords[1] = buff[1];

		return true;
	}

	if(pusNumberRead)
	{
		*pusNumberRead = 0;
	}

	return false;
}

//

AnyseeCardReader::AnyseeCardReader()
	: m_anysee(NULL)
	, m_index(0)
{
}

AnyseeCardReader::~AnyseeCardReader()
{
	Close();
}

void AnyseeCardReader::Close()
{
	if(m_anysee != NULL)
	{
		m_anysee->DeActivation();

		m_anysee->Close(1 + m_index);

		delete m_anysee;

		m_anysee = NULL;
	}
}

DWORD WINAPI AnyseeCardReader::ThreadProc(void* p)
{
	CoInitialize(0);

	AnyseeCardReader* r = (AnyseeCardReader*)p;

	CanyseeSC* anysee = new CanyseeSC();

	if(anysee->GetNumberOfDevices() > r->m_index && anysee->Open(1 + r->m_index) == 0)
	{
		r->m_anysee = anysee;
	}
	else
	{
		delete anysee;
	}

	CoUninitialize();

	return 0;
}

bool AnyseeCardReader::Create(int index)
{
	Close();

	m_name = Util::Format(L"Anysee SmartCard Reader %d", index);
	m_index = index;

	DWORD id;

	WaitForSingleObject(CreateThread(NULL, 0, ThreadProc, this, 0, &id), INFINITE);

	return m_anysee != NULL;
}

bool AnyseeCardReader::Connect()
{
	if(clock() - m_last_connect < 1500)
	{
		return false;
	}

	m_last_connect = clock();

	if(m_anysee == NULL || !m_anysee->IsPresent())
	{
		return false;
	}

	m_atr.clear();

	BYTE buff[256];
	DWORD size = sizeof(buff);

	if(m_anysee->Activation(buff, size) != 0)
	{
		return false;
	}

	m_atr.resize(size);

	memcpy(m_atr.data(), buff, size);

	return __super::Connect();
}

void AnyseeCardReader::Disconnect()
{
	__super::Disconnect();
}

bool AnyseeCardReader::HasCard()
{
	if(m_anysee == NULL || !m_anysee->IsPresent())
	{
		return false;
	}

	return __super::HasCard();
}

bool AnyseeCardReader::Transfer(BYTE* pucCommand, WORD uNumberToWrite, BYTE* pucResponse, WORD* pusNumberRead, BYTE* pucPBWords)
{
	BYTE buff[256];
	WORD size = pucResponse && pucCommand[0] == 0xdd && pucCommand[1] == 0xca ? pucCommand[4] : 0;
	WORD status = 0;

	if(m_anysee->ReadWriteT0(pucCommand, uNumberToWrite, buff, size, status) == 0)
	{
		if(pusNumberRead)
		{
			*pusNumberRead = size;

			memcpy(pucResponse, buff, size);
		}

		pucPBWords[0] = status >> 8;
		pucPBWords[1] = status & 0xff;

		return true;
	}

	if(pusNumberRead)
	{
		*pusNumberRead = 0;
	}

	return false;
}

//

NetCardReader::NetCardReader()
	: m_host("homesyslite.timbuktu.hu")
	, m_port(4896)
{
	CRegKey key;

	if(ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\Homesys\\NetCard", KEY_READ))
	{
		wchar_t buff[256];
		ULONG count = sizeof(buff) / sizeof(buff[0]);
		DWORD port = 0;

		if(ERROR_SUCCESS == key.QueryStringValue(L"host", buff, &count)) 
		{
			std::wstring s(buff);

			m_host = std::string(s.begin(), s.end());
		}

		if(ERROR_SUCCESS == key.QueryDWORDValue(L"port", port))
		{
			m_port = port;
		}
	}
}

NetCardReader::~NetCardReader()
{
}

bool NetCardReader::Create(int index)
{
	if(index > 0) return false;
	
	m_name = Util::Format(L"NetCard Reader %d", index);

	return true;
}

bool NetCardReader::Connect()
{
	if(clock() - m_last_connect < 10000)
	{
		return false;
	}

	m_last_connect = clock();
	
	// printf("NetCard connect\n");

	return m_socket.Open(m_host.c_str(), m_port);
}

void NetCardReader::Disconnect()
{
	// printf("NetCard disconnect\n");

	m_socket.Close();
}

bool NetCardReader::HasCard()
{
	return m_socket.IsOpen();
}

bool NetCardReader::Transfer(BYTE* pucCommand, WORD uNumberToWrite, BYTE* pucResponse, WORD* pusNumberRead, BYTE* pucPBWords)
{
	return false;
}

bool NetCardReader::ParseCAT(SmartCardPacket* packet)
{
	return true;
}

bool NetCardReader::ParseEMM(SmartCardPacket* packet)
{
	return true;
}

bool NetCardReader::ParseECM(SmartCardPacket* packet, bool& gotcw)
{
	if(packet->required)
	{
		clock_t t = clock();

		WORD pid = packet->pid | 0x8000;

		if(!m_socket.Write(&packet->hash, 4)) return false;
		if(!m_socket.Write(&packet->hash_size, 2)) return false;
		if(!m_socket.Write(&pid, 2)) return false;

		WORD size = 0;

		if(m_socket.Read(&size, sizeof(size)) == sizeof(size) && size == 16)
		{
			BYTE* buff = new BYTE[size];

			if(m_socket.Read(packet->cw, size) == size)
			{
				gotcw = true;

				packet->cw_netcard = true;
			}
		}

		// printf("NetCard ECM %d %d %d\n", clock() - t, packet->required, packet->pid);
	}

	return true;
}

bool NetCardReader::ParseDCW(SmartCardPacket* packet)
{
	if(!packet->cw_netcard)
	{
		clock_t t = clock();

		if(!m_socket.Write(&packet->hash, 4)) return false;
		if(!m_socket.Write(&packet->hash_size, 2)) return false;
		if(!m_socket.Write(&packet->pid, 2)) return false;

		WORD size = sizeof(packet->cw);

		if(!m_socket.Write(&size, 2)) return false;
		if(!m_socket.Write(packet->cw, size)) return false;

		// printf("NetCard DCW %d %d\n", clock() - t, packet->pid);
	}

	return true;
}

//

DWORD SmartCardServer::m_client_cookie = 0;

SmartCardServer::SmartCardServer()
	: CUnknown(NAME("SmartCardServer"), NULL)
{
	CreateReaders<WinSCardReader>(8);
	CreateReaders<AnyseeCardReader>(8);
	//CreateReaders<NetCardReader>(1);
}

SmartCardServer::~SmartCardServer()
{
	CAutoLock cAutoLock(this);

	for(auto i = m_readers.begin(); i != m_readers.end(); i++)
	{
		delete *i;
	}
}

STDMETHODIMP SmartCardServer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	return 
		QI(ISmartCardServer)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

template<class T> void SmartCardServer::CreateReaders(int n)
{
	for(int i = 0; i < n; i++)
	{
		SmartCardReader* scr = new T();

		if(scr->Create(i))
		{
			m_readers.push_back(new PacketThread(this, scr));
		}
		else
		{
			delete scr;

			break;
		}
	}
}

void SmartCardServer::OnDCW(SmartCardPacket* packet, bool cached)
{
	{
		CAutoLock cAutoLock(&m_client_lock);

		auto i = m_clients.find(packet->cookie);

		if(i == m_clients.end())
		{
			return;
		}

		i->second->OnDCW(packet->pid, packet->cw);
	}

	if(!cached)
	{
		CAutoLock cAutoLock(this);

		for(auto i = m_readers.begin(); i != m_readers.end(); i++)
		{
			SmartCardPacket* p = new SmartCardPacket(*packet);
			
			p->type = DCW;
			p->created = clock();

			(*i)->Queue(p);
		}
	}
}

// ISmartCardServer

STDMETHODIMP SmartCardServer::Process(DWORD cookie, int type, const BYTE* buff, int len, WORD pid, WORD system_id, bool required)
{
	DWORD hash;
	DWORD hash_size;

	{
		int n = len;

		while(n > 0 && buff[n - 1] == 0xff)
		{
			n--;
		}

		hash = n;
		hash_size = n;

		for(int i = 0, j = n >> 2; i < j; i++) 
		{
			hash ^= ((const DWORD*)buff)[i];
		}
	}

	CAutoLock cAutoLock(this);

	// TODO: discover new readers?

	for(auto i = m_readers.begin(); i != m_readers.end(); i++)
	{
		PacketThread* t = *i;

		SmartCardPacket* p = new SmartCardPacket();

		p->cookie = cookie;
		p->type = type;
		p->pid = pid;
		p->system_id = system_id;
		p->required = required;
		p->buff.resize(len);
		memcpy(p->buff.data(), buff, p->buff.size());
		p->created = clock();
		p->hash = hash;
		p->hash_size = hash_size;
		memset(p->cw, 0, sizeof(p->cw));
		p->cw_netcard = false;

		t->Queue(p);
	}

	return S_OK;
}

STDMETHODIMP SmartCardServer::Register(ISmartCardClient* scc, DWORD* cookie)
{
	CAutoLock cAutoLock(&m_client_lock);

	DWORD next = ++m_client_cookie;

	if(next == 0) next = ++m_client_cookie;

	*cookie = next;

	m_clients[next] = scc;

	return S_OK;
}

STDMETHODIMP SmartCardServer::Unregister(DWORD cookie)
{
	CAutoLock cAutoLock(&m_client_lock);

	auto i = m_clients.find(cookie);

	if(i == m_clients.end())
	{
		return E_INVALIDARG;
	}

	m_clients.erase(i);

	return S_OK;
}

STDMETHODIMP SmartCardServer::GetDevices(std::vector<SmartCardDevice>& devs)
{
	std::vector<HANDLE> events;

	{
		CAutoLock cAutoLock(this);

		for(auto i = m_readers.begin(); i != m_readers.end(); i++)
		{
			SmartCardReader* r = (*i)->m_scr;

			if(dynamic_cast<NetCardReader*>(r))
			{
				continue;
			}

			if(!r->HasCard())
			{
				SmartCardPacket* p = new SmartCardPacket();

				p->cookie = 0;
				p->type = ISmartCardServer::PING;
				p->system_id = 0;
				p->ready = CreateEvent(NULL, FALSE, FALSE, NULL);

				HANDLE ready = NULL;

				DuplicateHandle(GetCurrentProcess(), p->ready, GetCurrentProcess(), &ready, 0, FALSE, DUPLICATE_SAME_ACCESS);

				events.push_back(ready);

				(*i)->Queue(p);
			}
		}
	}

	WaitForMultipleObjects(events.size(), events.data(), TRUE, 10000);

	for(auto i = events.begin(); i != events.end(); i++)
	{
		CloseHandle(*i);
	}

	{
		CAutoLock cAutoLock(this);

		devs.reserve(m_readers.size());

		for(auto i = m_readers.begin(); i != m_readers.end(); i++)
		{
			SmartCardReader* r = (*i)->m_scr;

			if(dynamic_cast<NetCardReader*>(r))
			{
				continue;
			}

			SmartCardDevice dev;

			dev.name = r->GetName();
			dev.inserted = r->HasCard();
			dev.serial = r->GetSerial();
			dev.system.id = r->GetSystemId();
			dev.system.name = r->GetSystemName();
			
			r->GetSubscriptions(dev.subscriptions);

			devs.push_back(dev);
		}
	}

	return S_OK;
}

STDMETHODIMP SmartCardServer::CheckSystemId(WORD system_id)
{
	CAutoLock cAutoLock(this);

	for(auto i = m_readers.begin(); i != m_readers.end(); i++)
	{
		SmartCardReader* r = (*i)->m_scr;

		if(dynamic_cast<NetCardReader*>(r)) // TODO
		{
			continue;
		}

		if(r->HasCard() && r->CheckSystemId(system_id))
		{
			return S_OK;
		}
	}

	return E_FAIL;
}

SmartCardServer::PacketThread::PacketThread(SmartCardServer* scs, SmartCardReader* scr)
	: m_scs(scs)
	, m_scr(scr)
{
	for(int i = 0; i < sizeof(m_queue) / sizeof(m_queue[0]); i++)
	{
		m_queue[i] = new CThreadSafeQueue<SmartCardPacket*>(1000);
	}

	Create();
}

SmartCardServer::PacketThread::~PacketThread()
{
	CAMThread::CallWorker(CMD_EXIT);
	CAMThread::Close();

	for(int i = 0; i < sizeof(m_queue) / sizeof(m_queue[0]); i++)
	{
		while(m_queue[i]->GetCount() > 0)
		{
			delete m_queue[i]->Dequeue();
		}
	}

	m_emm.clear();

	for(auto i = m_ecm.begin(); i != m_ecm.end(); i++)
	{
		delete *i;
	}

	m_ecm.clear();

	delete m_scr;
}

void SmartCardServer::PacketThread::Queue(SmartCardPacket* packet)
{
	if(packet->type != PING && !m_scr->CheckPacket(packet))
	{
		delete packet;

		return;
	}

	int i = 0;

	switch(packet->type)
	{
	case DCW: i = 0; break;
	case ECM: i = packet->required ? 1 : 2; break;
	default: i = 3; break;
	}

	if(m_queue[i]->GetCount() == m_queue[i]->GetMaxCount())
	{
		printf("SmartCardServer queue %d full\n", i);

		delete packet;

		return;
	}

	m_queue[i]->Enqueue(packet);
}

void SmartCardServer::PacketThread::Process(SmartCardPacket* packet)
{
	if(!m_scr->HasCard() && !m_scr->Connect())
	{
		return;
	}

	if(!m_scr->CheckSystemId(packet->system_id))
	{
		return;
	}

	clock_t dt = clock() - packet->created;

	// wprintf(L"[%d] %d %s\n", packet->type, dt, m_scr->GetName());

	if(packet->type == CAT)
	{
		m_scr->ParseCAT(packet);
	}
	else if(packet->type == EMM)
	{
		for(auto i = m_emm.begin(); i != m_emm.end(); i++)
		{
			if(*i == packet->hash)
			{
				return;
			}
		}

		if(m_scr->ParseEMM(packet))
		{
			while(m_emm.size() > 10)
			{
				m_emm.pop_back();
			}

			m_emm.push_front(packet->hash);
		}
	}
	else if(packet->type == ECM)
	{
		bool toolate = false; // dt < 3000;

		if(packet->required || !toolate)
		{
			for(auto i = m_ecm.begin(); i != m_ecm.end(); i++)
			{
				ecm_t* ecm = *i;

				if(ecm->hash == packet->hash)
				{
					memcpy(packet->cw, ecm->cw, 16);

					m_scs->OnDCW(packet, true);

					return;
				}
			}

			bool gotcw = false;

			if(m_scr->ParseECM(packet, gotcw))
			{
				if(gotcw)
				{
					while(m_ecm.size() > 50)
					{
						delete m_ecm.back();

						m_ecm.pop_back();
					}

					ecm_t* ecm = new ecm_t();

					ecm->hash = packet->hash;
					ecm->pid = packet->pid;
					memcpy(ecm->cw, packet->cw, 16);

					m_ecm.push_front(ecm);

					m_scs->OnDCW(packet, false);
				}
			}
			else
			{
				m_scr->Disconnect(); // TODO: retry?
			}
		}
	}
	else if(packet->type == DCW)
	{
		m_scr->ParseDCW(packet);
	}

	if(packet->type >= 3)
	wprintf(L"[%d] %d %d %04d %s\n", packet->type, clock() - packet->created, packet->pid, packet->system_id, m_scr->GetName());
}

DWORD SmartCardServer::PacketThread::ThreadProc()
{
	Util::Socket::Startup();
	
	SmartCardPacket* packet = NULL;

	SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

	HANDLE handles[] = 
	{
		GetRequestHandle(),
		m_queue[0]->GetEnqueueEvent(),
		m_queue[1]->GetEnqueueEvent(),
		m_queue[2]->GetEnqueueEvent(),
		m_queue[3]->GetEnqueueEvent(),
	};

	while(m_hThread != NULL)
	{
		switch(DWORD i = WaitForMultipleObjects(sizeof(handles) / sizeof(handles[0]), handles, FALSE, INFINITE))
		{
		case WAIT_OBJECT_0 + 0: 
			Reply(0);
			m_hThread = NULL;
			break;

		case WAIT_OBJECT_0 + 1: 
		case WAIT_OBJECT_0 + 2: 
		case WAIT_OBJECT_0 + 3: 
		case WAIT_OBJECT_0 + 4: 
			packet = m_queue[i - (WAIT_OBJECT_0 + 1)]->Dequeue();
			Process(packet);
			if(packet->ready != NULL) SetEvent(packet->ready);
			delete packet;
			break;

		default: 
			m_hThread = NULL;
			break;
		}
	}

	Util::Socket::Cleanup();

	return 0;
}
