#pragma once

#include "../../util/Socket.h"

class RTMPPacket
{
public:
	BYTE m_headerType;
	BYTE m_packetType;
	BYTE m_nChannel;
	int m_nInfoField1; // 3 first bytes
	int m_nInfoField2; // last 4 bytes in a long header
	unsigned int m_nBodySize;
	unsigned int m_nBytesRead;
	unsigned int m_nInternalTimestamp;
	char* m_body;

public:
	RTMPPacket();
	virtual ~RTMPPacket();

	void Reset();
	void AllocPacket(int nSize);
	void FreePacket();
	void FreePacketHeader();
	bool IsReady() const {return m_nBytesRead == m_nBodySize;}
};

class RTMPSocket : public Util::Socket
{
	std::string m_url;
	std::string m_app;
	std::string m_playpath;
	std::string m_swfUrl;
	std::string m_pageUrl;

	int m_chunkSize;
	int m_nBWCheckCounter;
	int m_nBytesIn;
	int m_nBytesInSent;
	bool m_bPlaying;
	int m_nBufferMS;
	int m_stream_id; // returned in _result from invoking createStream

	std::vector<std::string> m_methodCalls; //remote method calls queue

	char* m_pBuffer; // data read from socket
	char* m_pBufferStart; // pointer into m_pBuffer of next byte to process
	int m_nBufferSize; // number of unprocessed bytes in buffer
	RTMPPacket m_vecChannelsIn[64];
	RTMPPacket m_vecChannelsOut[64];

	double m_start;
	double m_duration; // duration of stream in seconds

protected:
	bool Connect();
	bool HandShake();

	bool SendServerBW();
	bool SendCheckBW();
	bool SendCheckBWResult();
	bool SendPing(short nType, unsigned int nObject, unsigned int nTime = 0);
	bool SendBGHasStream(double dId, const char* playpath);
	bool SendCreateStream(double dStreamId);
	bool SendPlay();
	bool SendPause();
	bool SendSeek(double dTime);
	bool SendBytesReceived();

	void HandleInvoke(const char* body, unsigned int nBodySize);
	void HandleMetadata(char* body, unsigned int len);
	void HandleChangeChunkSize(const RTMPPacket& packet);
	void HandleAudio(const RTMPPacket& packet);
	void HandleVideo(const RTMPPacket& packet);
	void HandlePing(const RTMPPacket& packet);

	bool SendPacket(RTMPPacket& packet);
	bool ReadPacket(RTMPPacket& packet);

public:
	RTMPSocket();
	virtual ~RTMPSocket();

	bool Open(const Url& url, double start, std::string* playpath = NULL);
	void Close();

	int Read(void* buff, int len);

	void Seek(double start) {SendSeek(start);}
	double GetDuration() const {return m_duration;}

	void SetBufferMS(int size);
	void UpdateBufferMS();

	bool GetNextMediaPacket(RTMPPacket& packet);
};
