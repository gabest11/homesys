#pragma once

#include <atltime.h>
#include <winscard.h>
#include "ThreadSafeQueue.h"
#include "../util/String.h"
#include "../util/Socket.h"
#include "../3rdparty/anysee/anyseeSC.h"

#pragma comment(lib, "winscard.lib")

struct SmartCardSubscription
{
	WORD id;
	std::wstring name;
	std::list<CTime> date;
};

struct SmartCardDevice
{
	std::wstring name;
	bool inserted;
    std::wstring serial;
	struct {WORD id; std::wstring name;} system;
	std::list<SmartCardSubscription> subscriptions;
};

[uuid("0B33B3E1-E026-41E2-A039-18D823C9D992")]
interface ISmartCardClient : public IUnknown
{
	STDMETHOD(OnDCW)(WORD pid, const BYTE* cw) = 0;
};

[uuid("379AA274-3C56-4267-843F-6BB8C49C0F09")]
interface ISmartCardServer : public IUnknown
{
	enum {CAT = 1, EMM, ECM, DCW, PING};

	STDMETHOD(Process)(DWORD cookie, int type, const BYTE* buff, int len, WORD pid, WORD system_id, bool required) = 0;
	STDMETHOD(Register)(ISmartCardClient* scc, DWORD* cookie) = 0;
	STDMETHOD(Unregister)(DWORD cookie) = 0;
	STDMETHOD(GetDevices)(std::vector<SmartCardDevice>& devs) = 0;
	STDMETHOD(CheckSystemId)(WORD system_id) = 0;
};

class SmartCardPacket
{
public:
	DWORD cookie;
	int type;
	std::vector<BYTE> buff;
	WORD pid;
	WORD system_id;
	bool required;
	clock_t created;
	DWORD hash;
	DWORD hash_size;
	BYTE cw[16];
	bool cw_netcard;
	HANDLE ready;

	SmartCardPacket()
	{
		ready = NULL;
	}

	virtual ~SmartCardPacket()
	{
		if(ready != NULL) CloseHandle(ready);
	}
};

class SmartCardReader;

class SmartCard
{
protected:
	SmartCardReader* m_reader;
	WORD m_system_id;

public:
	SmartCard(SmartCardReader* reader);
	virtual ~SmartCard();

	virtual bool Init() = 0;
	virtual bool ParseCAT(const BYTE* pBuf) = 0;
	virtual bool ParseEMM(const BYTE* pBuf) = 0;
	virtual bool ParseECM(const BYTE* pBuf, BYTE* pucCW, bool& gotcw) = 0;
	virtual bool ParseDCW(const BYTE* pBuf) {return true;}

	virtual std::wstring GetSerial() const {return L"";}
	virtual std::wstring GetSystemName() const {return L"";}
	virtual void GetSubscriptions(std::list<SmartCardSubscription>& subscriptions) {}

	WORD GetSystemId() const {return m_system_id;}
	bool CheckSystemId(WORD system_id) const {return system_id == 0 || m_system_id == 0 || system_id == m_system_id;}
};

class ConaxCard : public SmartCard
{
	#define ASCII_TXT_TAG			0x01 
	#define OCTET_TAG				0x20 
	#define TIME_TAG				0x30 
	#define PRODUCT_ID_TAG			0x46 
	#define PRICE_TAG				0x47 
	#define EMM_ADDRESS_TAG			0x23 
	 
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
	#define SUBSCRIPTION_PI			0x32 
	 
	struct Conax_Param
	{ 
		BYTE Version;					/* 井云催 */ 
		WORD CaSysId;				/* CA狼由ID */ 
		WORD CountryIndicatorValue;	/* 忽社炎崗 */ 
		BYTE MatRat;					/* 定槍吉雫 */ 
		BYTE MaxSessionNum;			/* 恷寄氏三倖方 */ 
	}; /* Conax触歌方 */ 
	 
	struct Conax_Info
	{ 
		int iCardNumber; 
		Conax_Param stParam; 
		BYTE auEMMAddress[10][7]; 
		int iEMMAddressCount;	 
	}; 

	struct Conax_CW
	{ 
		WORD		cw_id;					/* CW ID (not used) */ 
		BYTE		odd_even_indicator;		/* ODD EVEN Indicator */ 
		BYTE		aucCWBuf[8];			/* CW buf */ 
		BYTE		iCWLen;					/* CW length */ 
	};	/* CW歌方 */ 
 
	struct Conax_AccessStatus
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
		BYTE		 current_mat_rat_value; 
		WORD		 minutes_left; 
		BYTE		 product_ref[6]; 
	}; /* Conax俊辺彜蓑歌方(PI=0x31) */ 
 
	struct Conax_Subscription
	{
		WORD id;
		std::string name;
		WORD date[4];
		DWORD pbm[2];

		int GetDay(int i) {return (date[i] >> 8) & 0x1f;}
		int GetMonth(int i) {return date[i] & 0x0f;}
		int GetYear(int i) {return 1990 + (((date[i] >> 4) & 0xf) + (date[i] >> 13) * 10);}
	};

	bool TransferRead(BYTE* Buffer, WORD NumberToRead, WORD* Read, BYTE* Status, BYTE sid); 
 	bool TransferWrite(BYTE* Buffer, WORD NumberToWrite);

	bool InitCASS(); 
 	bool ResetSESS(BYTE sid);
 	bool ParseResp(BYTE* Response,WORD len); 
 	bool ParseCADescEMM(BYTE* pBuf); 
 	bool ParseAccessStatus(BYTE* Response); 
	bool ParseSubscription(BYTE* pBuf, int len);
 	bool ReqCardNumber(); 
	bool CAStatusSelect(BYTE type, BYTE* pBuf);
 	bool CAStatusCom(BYTE sid, BYTE* pBuf, BYTE len); 

	Conax_Info m_conax;
	Conax_AccessStatus m_access; 
	std::map<WORD, Conax_Subscription> m_subscription;

protected:
	bool Init();
	bool ParseCAT(const BYTE* pBuf);
	bool ParseEMM(const BYTE* pBuf);
	bool ParseECM(const BYTE* pBuf, BYTE* pucCW, bool& gotcw);

	std::wstring GetSerial() const;
	std::wstring GetSystemName() const;
	void GetSubscriptions(std::list<SmartCardSubscription>& subscriptions);

public:
	ConaxCard(SmartCardReader* reader);
};

class CryptoWorksCard : public SmartCard
{
	BYTE m_serial[5];
	struct {BYTE id; char name[15];} m_issuer;

	bool SelectFile(BYTE b0, BYTE b1);
	int ReadRecord(BYTE rec, BYTE* buff);
	bool CheckSerial(const BYTE* p);

protected:
	bool Init();
	bool ParseCAT(const BYTE* pBuf);
	bool ParseEMM(const BYTE* pBuf);
	bool ParseECM(const BYTE* pBuf, BYTE* pucCW, bool& gotcw);

	std::wstring GetSerial() const;
	std::wstring GetSystemName() const;

public:
	CryptoWorksCard(SmartCardReader* reader);
};

class SmartCardReader
{
protected:
	std::wstring m_name;
	SmartCard* m_sc;
	std::vector<BYTE> m_atr;
	clock_t m_last_connect;

public:
	SmartCardReader();
	virtual ~SmartCardReader();

	LPCWSTR GetName() {return m_name.c_str();}

	virtual bool Create(int index) = 0;
	virtual bool Connect();
	virtual void Disconnect();
	virtual bool HasCard();
	virtual bool CheckPacket(const SmartCardPacket* packet) {return true;}
	virtual std::wstring GetSerial() {return m_sc != NULL ? m_sc->GetSerial() : L"";}
	virtual std::wstring GetSystemName() {return m_sc != NULL ? m_sc->GetSystemName() : L"";}
	virtual WORD GetSystemId() {return m_sc != NULL ? m_sc->GetSystemId() : 0;}
	virtual bool CheckSystemId(WORD system_id) {return m_sc != NULL && m_sc->CheckSystemId(system_id);}
	virtual void GetSubscriptions(std::list<SmartCardSubscription>& subscriptions) {if(m_sc != NULL) m_sc->GetSubscriptions(subscriptions);}
	virtual bool Transfer(BYTE* pucCommand, WORD uNumberToWrite, BYTE* pucResponse, WORD* pusNumberRead, BYTE* pucPBWords) = 0;

	virtual bool ParseCAT(SmartCardPacket* packet);
	virtual bool ParseEMM(SmartCardPacket* packet);
	virtual bool ParseECM(SmartCardPacket* packet, bool& gotcw);
	virtual bool ParseDCW(SmartCardPacket* packet);
};

class WinSCardReader : public SmartCardReader
{
	SCARDCONTEXT m_context;
	SCARDHANDLE m_card;

protected:
	bool Create(int index);
	bool Connect();
	void Disconnect();
	bool HasCard();
	bool Transfer(BYTE* pucCommand, WORD uNumberToWrite, BYTE* pucResponse, WORD* pusNumberRead, BYTE* pucPBWords);

public:
	WinSCardReader();
	virtual ~WinSCardReader();
};

class AnyseeCardReader : public SmartCardReader
{
	CanyseeSC* m_anysee;
	int m_index;

	void Close();

	static DWORD WINAPI AnyseeCardReader::ThreadProc(void* p);

protected:
	bool Create(int index);
	bool Connect();
	void Disconnect();
	bool HasCard();
	bool Transfer(BYTE* pucCommand, WORD uNumberToWrite, BYTE* pucResponse, WORD* pusNumberRead, BYTE* pucPBWords);

public:
	AnyseeCardReader();
	virtual ~AnyseeCardReader();
};

class NetCardReader : public SmartCardReader
{
	Util::Socket m_socket;
	std::string m_host;
	DWORD m_port;

protected:
	bool Create(int index);
	bool Connect();
	void Disconnect();
	bool HasCard();
	bool CheckPacket(const SmartCardPacket* packet) {return packet->type == ISmartCardServer::ECM && packet->required || packet->type == ISmartCardServer::DCW;}
	std::wstring GetSerial() {return L"";}
	std::wstring GetSystemName() {return L"NetCard";}
	WORD GetSystemId() {return 0;}
	bool CheckSystemId(WORD system_id) {return true;}
	bool Transfer(BYTE* pucCommand, WORD uNumberToWrite, BYTE* pucResponse, WORD* pusNumberRead, BYTE* pucPBWords);

	bool ParseCAT(SmartCardPacket* packet);
	bool ParseEMM(SmartCardPacket* packet);
	bool ParseECM(SmartCardPacket* packet, bool& gotcw);
	bool ParseDCW(SmartCardPacket* packet);

public:
	NetCardReader();
	virtual ~NetCardReader();
};

class SmartCardServer 
	: public CUnknown
	, public CCritSec
	, public ISmartCardServer
{
	class PacketThread : public CAMThread, public CCritSec
	{
		friend class SmartCardServer;

		struct ecm_t 
		{
			DWORD hash; 
			WORD pid;
			BYTE cw[16];
		};

		SmartCardServer* m_scs;
		SmartCardReader* m_scr;
		CThreadSafeQueue<SmartCardPacket*>* m_queue[4];
		std::list<DWORD> m_emm; 
		std::list<ecm_t*> m_ecm;

		enum {CMD_EXIT};

		DWORD ThreadProc();

		void Process(SmartCardPacket* packet);

	public:
		PacketThread(SmartCardServer* scs, SmartCardReader* scr);
		virtual ~PacketThread();

		void Queue(SmartCardPacket* packet);
	};

	std::list<PacketThread*> m_readers;

	CCritSec m_client_lock;
	std::map<DWORD, ISmartCardClient*> m_clients;
	static DWORD m_client_cookie;

	void OnDCW(SmartCardPacket* packet, bool cached);

	template<class T> void CreateReaders(int n);

public:
	SmartCardServer();
	virtual ~SmartCardServer();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISmartCardServer

	STDMETHODIMP Process(DWORD cookie, int type, const BYTE* buff, int len, WORD pid, WORD system_id, bool required);
	STDMETHODIMP Register(ISmartCardClient* scc, DWORD* cookie);
	STDMETHODIMP Unregister(DWORD cookie);
	STDMETHODIMP GetDevices(std::vector<SmartCardDevice>& devs);
	STDMETHODIMP CheckSystemId(WORD system_id);
};


/* 
************ 辺欺ECM朔,匯倖字競歳屶隔謹倖狼由盾鷹侃尖狛殻議留旗鷹泌和(EMM議侃尖窃貌): 
 
  	switch(CASystemId&0xFF00) 
	{ 
		case 0x0500:		// Viaccess  
			if (字競歳嶄峨秘議葎V触 && 崘嬬触厮将兜兵晒撹孔) 
			{ 
				via_parse_ecm(); 
			}		  
			break; 
		case 0x0600:		// Irdeto  
			if (字競歳嶄峨秘議葎I触 && 崘嬬触厮将兜兵晒撹孔) 
			{ 
				irdeto_parse_ecm(); 
			} 
			break; 
	   case 0x0B00:		// Conax  
		    
		   if (字競歳嶄峨秘議葎I触 && 崘嬬触厮将兜兵晒撹孔) 
		   { 
			   conax_parse_ecm(); 
		   } 
		   break; 
		case 0x4A00: 
		  if (CASystemId>=0x4A00 && CASystemId<=0x4A0F)	// Tongfang 
		  { 
			  if (字競歳嶄峨秘議葎T触 && 崘嬬触厮将兜兵晒撹孔) 
			  { 
				  yxtf_parse_ecm(); 
			  } 
		  } 
		  else if (CASystemId>=0x4AB0 && CASystemId<=0x4ABF)// Suantong 
		  { 
			  if (字競歳嶄峨秘議葎ST触 && 崘嬬触厮将兜兵晒撹孔) 
			  { 
				  bjst_parse_ecm(); 
			  } 
		  } 
		  else if (CASystemId==0x4AD2 || CASystemId<=0x4AD3) // Shuma 
		  { 
			  if (字競歳嶄峨秘議葎SM触 && 崘嬬触厮将兜兵晒撹孔) 
			  { 
				  smsx_parse_ecm(); 
			  } 
		  } 
		  break; 
		case 0x3000:	 
		   if (字競歳嶄峨秘議葎ST触 && 崘嬬触厮将兜兵晒撹孔)// Suantong 
		   { 
				  bjst_parse_ecm(); 
		   } 
		   break; 
		default: 
			break; 
	} 
*/ 