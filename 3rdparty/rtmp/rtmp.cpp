#include "stdafx.h"
#include "rtmp.h"
#include "amf.h"
#include "bytes.h"
#include "../tinyxml/tinyxml.h"
#include <algorithm>

using namespace std;

#define RTMP_SIG_SIZE 1536
#define RTMP_LARGE_HEADER_SIZE 12
#define RTMP_BUFFER_CACHE_SIZE (16 * 1024) // needs to fit largest number of bytes recv() may return

static const int packetSize[] = {12, 8, 4, 1};

enum
{
	RTMP_PACKET_SIZE_LARGE = 0,
	RTMP_PACKET_SIZE_MEDIUM = 1,
	RTMP_PACKET_SIZE_SMALL = 2,
	RTMP_PACKET_SIZE_MINIMUM = 3,
};

enum
{
	RTMP_PACKET_TYPE_AUDIO = 8,
	RTMP_PACKET_TYPE_VIDEO = 9,
	RTMP_PACKET_TYPE_INFO = 12,
};

// RTMPSocket

RTMPSocket::RTMPSocket()
{
	Close();

	m_pBuffer = new char[RTMP_BUFFER_CACHE_SIZE];
	m_nBufferMS = 300;
	m_start = 0;
	m_duration = 0;
}

RTMPSocket::~RTMPSocket()
{
	Close();

	delete [] m_pBuffer;
}

struct BBCMedia
{
	std::string url;
	std::string app;
	std::string playpath;
	std::string host;
	int port;
	int width;
	int height;
};

bool RTMPSocket::Open(const Url& url, double start, std::string* playpath)
{
	std::string host = url.host;
	int port = url.port;

	m_start = start;
	m_duration = 0;

	m_url.clear();
	m_app.clear();
	m_playpath.clear();

	m_swfUrl = "http://" + url.host + "/player.swf"; // "file://C:/FlvPlayer.swf"; // TODO
	m_pageUrl = "http://" + url.host + url.request + ".html"; // "http://somehost/sample.html"; // TODO

	if(url.protocol == "rtmp")
	{
		string::size_type i = url.request.find('/');

		if(i != 0)
		{
			return false;
		}

		i = url.request.find("/mp4:");

		if(i != string::npos)
		{
			string::size_type j = url.request.find('?', i + 1);

			m_url = url.value;
			m_app = url.request.substr(1, i - 1);
			if(j != string::npos) m_app += url.request.substr(j);
			m_playpath = url.request.substr(i + 1, j != string::npos ? (j - 1) - i : string::npos);
		}
		else
		{
			i = url.request.find('/', 1);

			if(i == string::npos)
			{
				return false;
			}

			string::size_type j = url.request.find_first_of("&?", i + 1);

			if(j == string::npos)
			{
				j = url.request.size();
			}

			m_url = url.value;
			m_app = url.request.substr(1, i - 1);
			m_playpath = url.request.substr(i + 1, j - (i + 1));

			std::string::size_type ext = m_playpath.size() - 4;

			if((m_playpath.find(".mp4") == ext || m_playpath.find(".f4v") == ext) && m_playpath.find(':') == std::string::npos)
			{
				m_playpath = "mp4:" + m_playpath;
			}
		}
	}
	else if(url.protocol == "bbc")
	{
		// bbc://groupId/[videoId]

		Socket s;

		std::map<std::string, std::string> headers;

		std::list<std::string> types;

		types.push_back("application/xml");
		types.push_back("text/xml");

		std::string xml;

		std::string id;

		if(url.request == "/")
		{
			headers.clear();

			if(!s.HttpGetXML(("http://www.bbc.co.uk/iplayer/playlist/" + url.host).c_str(), headers, types, xml))
			{
				return false;
			}

			TiXmlDocument doc;

			doc.Parse(xml.c_str(), NULL, TIXML_ENCODING_UTF8);

			if(TiXmlElement* playlist = doc.FirstChildElement("playlist"))
			{
				for(TiXmlElement* n = playlist->FirstChildElement("item"); 
					n != NULL; 
					n = n->NextSiblingElement("item"))
				{
					const char* kind = n->Attribute("kind");

					if(kind != NULL && strcmp(kind, "programme") == 0)
					{
						const char* identifier = n->Attribute("identifier");

						if(identifier != NULL)
						{
							id = identifier;

							break;
						}
					}
				}
			}
		}
		else
		{
			id = url.request.substr(1);
		}

		std::vector<BBCMedia> ml;

		if(!id.empty())
		{
			headers.clear();

			headers["User-Agent"] = "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; WOW64; Trident/4.0; SLCC2; .NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; Media Center PC 6.0; .NET4.0C; .NET4.0E)"; // internal server error if not set

			if(!s.HttpGetXML(("http://www.bbc.co.uk/mediaselector/4/mtis/stream/" + id).c_str(), headers, types, xml))
			{
				return false;
			}

			TiXmlDocument doc;

			doc.Parse(xml.c_str(), NULL, TIXML_ENCODING_UTF8);

			if(TiXmlElement* ms = doc.FirstChildElement("mediaSelection"))
			{
				for(TiXmlElement* media = ms->FirstChildElement("media"); 
					media != NULL; 
					media = media->NextSiblingElement("media"))
				{
					const char* type = media->Attribute("type");

					if(type != NULL && strcmp(type, "video/mp4") == 0)
					{
						for(TiXmlElement* connection = media->FirstChildElement("connection"); 
							connection != NULL; 
							connection = connection->NextSiblingElement("connection"))
						{
							const char* supplier = connection->Attribute("supplier");
							const char* application = connection->Attribute("application");
							const char* authString = connection->Attribute("authString");
							const char* identifier = connection->Attribute("identifier");
							const char* server = connection->Attribute("server");
							const char* protocol = connection->Attribute("protocol");
							
							if(supplier != NULL && protocol != NULL && strcmp(protocol, "rtmp") == 0)
							{
								BBCMedia m;

								m.width = 0;
								m.height = 0;

								media->Attribute("width", &m.width);
								media->Attribute("height", &m.height);

								if(server != NULL && application != NULL && authString != NULL && identifier != NULL && strcmp(supplier, "limelight") == 0)
								{
									m.url = "rtmp://" + std::string(server) + ":1935/" + std::string(application) + "?" + std::string(authString);
									m.app = application;
									m.playpath = identifier;
									m.host = server;
									m.port = 1935;

									ml.push_back(m);
								}
								else if(server != NULL && application != NULL && authString != NULL && identifier != NULL && strcmp(supplier, "akamai") == 0)
								{
									m.url = "rtmp://" + std::string(server) + ":1935/" + std::string(application) + "?" + std::string(authString);
									m.app = application;
									m.playpath = identifier;
									m.host = server;
									m.port = 1935;

									ml.push_back(m);
								}
							}
						}
					}
				}
			}
		}

		std::sort(ml.begin(), ml.end(), [] (const BBCMedia& a, const BBCMedia& b) -> bool
		{
			return a.height > b.height;
		});

		BBCMedia* m = NULL;

		if(playpath != NULL)
		{
			for(auto i = ml.begin(); i != ml.end(); i++)
			{
				if(i->playpath == *playpath)
				{
					m = &*i;

					break;
				}
			}
		}

		if(m == NULL)
		{
			for(auto i = ml.begin(); i != ml.end(); i++)
			{
				//if((i->height & 15) == 0)
				{
					m = &*i;

					break;
				}
			}
		}

		if(m != NULL)
		{
			m_url = m->url;
			m_app = m->app;
			m_playpath = m->playpath;

			host = m->host;
			port = m->port;
		}

		if(playpath != NULL)
		{
			*playpath = m_playpath;
		}

		m_pageUrl = "http://bbc.co.uk/";
		m_swfUrl = "http://www.bbc.co.uk/emp/10player.swf";
	}
	else
	{
		return false;
	}

	if(!__super::Open(host.c_str(), port))
	{
		return false;
	}

	DWORD timeout = 10000;

	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	if(!Connect())
	{
		Close();

		return false;
	}

	return true;
}

void RTMPSocket::Close()
{
	__super::Close();

	m_chunkSize = 128;
	m_nBWCheckCounter = 0;
	m_nBytesIn = 0;
	m_nBytesInSent = 0;

	for(int i = 0; i < 64; i++)
	{
		m_vecChannelsIn[i].Reset();
		m_vecChannelsIn[i].m_nChannel = i;
		m_vecChannelsOut[i].Reset();
		m_vecChannelsOut[i].m_nChannel = i;
	}

	m_bPlaying = false;
	m_nBufferSize = 0;
}

int RTMPSocket::Read(void* buff, int len)
{
	len = __super::Read(buff, len);

	if(len > 0 && m_bPlaying)
	{
		m_nBytesIn += len;
		
		if(m_nBytesIn > m_nBytesInSent + 600 * 1024) // report every 600K
		{
			SendBytesReceived();
		}
	}

	return len;
}

void RTMPSocket::SetBufferMS(int size)
{
	m_nBufferMS = size;
}

void RTMPSocket::UpdateBufferMS()
{
	SendPing(3, 1, m_nBufferMS);
}

bool RTMPSocket::GetNextMediaPacket(RTMPPacket& packet)
{
	bool bHasMediaPacket = false;

	while(!bHasMediaPacket && IsOpen() && ReadPacket(packet))
	{
		if(!packet.IsReady())
		{
			packet.FreePacket();
			//Sleep(5000); // 5ms
			continue;
		}

		switch(packet.m_packetType)
		{
		case 0x01: // chunk size
			HandleChangeChunkSize(packet);
			break;

		case 0x03: // bytes read report
			break;

		case 0x04: // ping
			HandlePing(packet);
			break;

		case 0x05: // server bw
			break;

		case 0x06: // client bw
			break;

		case 0x08: // audio data
			HandleAudio(packet);
			bHasMediaPacket = true;
			break;

		case 0x09: // video data
			HandleVideo(packet);
			bHasMediaPacket = true;
			break;

		case 0x0F: // flex stream send
			break;

		case 0x10: // flex shared object
			break;

		case 0x11: // flex message
			HandleInvoke(packet.m_body + 1, packet.m_nBodySize - 1);
			break;

		case 0x12: // metadata (notify)
			HandleMetadata(packet.m_body, packet.m_nBodySize);
			bHasMediaPacket = true;
			break;

		case 0x13:
			break;

		case 0x14: // invoke
			HandleInvoke(packet.m_body, packet.m_nBodySize);
			break;

		case 0x16: // go through FLV packets and handle metadata packets

			for(int pos = 0; pos + 11 < packet.m_nBodySize; )
			{
				int dataSize = RTMP::ReadInt24(&packet.m_body[pos + 1]); // size without header (11) and prevTagSize (4)

				int next = pos + 11 + dataSize + 4;

				if(next > packet.m_nBodySize)
				{
					break;
				}

				if(packet.m_body[pos] == 0x12)
				{
					HandleMetadata(packet.m_body + pos + 11, dataSize);
				}

				pos = next;
			}

			bHasMediaPacket = true;

			break;

		default:
			break;
		}

		if(!bHasMediaPacket)
		{ 
			packet.FreePacket();
		}
	}

	if(bHasMediaPacket)
	{
		m_bPlaying = true;
	}

	return bHasMediaPacket;
}

bool RTMPSocket::Connect()
{
	if(!HandShake())
	{
		return false;
	}

	RTMPPacket packet;

	packet.m_nChannel = 0x03; // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_packetType = 0x14; // INVOKE
	packet.AllocPacket(4096);

	char* enc = packet.m_body;
	
	enc += RTMP::EncodeString(enc, "connect");
	enc += RTMP::EncodeNumber(enc, 1.0);
	*enc++ = 0x03; // Object Datatype
	enc += RTMP::EncodeString(enc, "app", m_app.c_str());
	enc += RTMP::EncodeString(enc, "flashVer", "FMSc/1.0");
	enc += RTMP::EncodeString(enc, "swfUrl", m_swfUrl.c_str());
	enc += RTMP::EncodeString(enc, "tcUrl", m_url.c_str());
	enc += RTMP::EncodeBoolean(enc, "fpad", false);
	enc += RTMP::EncodeNumber(enc, "capabilities", 15.0);
	enc += RTMP::EncodeNumber(enc, "audioCodecs", 0x405); // pcm mp3 aac
	enc += RTMP::EncodeNumber(enc, "videoCodecs", 0xb4); // vp6 vp6a h264 h263
	enc += RTMP::EncodeNumber(enc, "videoFunction", 1.0);
	enc += RTMP::EncodeString(enc, "pageUrl", m_pageUrl.c_str());
	//
	enc += 2; // end of object - 0x00 0x00 0x09
	*enc++ = 0x09;
	//enc += RTMP::EncodeString(enc, "user"); // DEBUG, REMOVE!!!
	//*enc++ = 0x05;
	//enc += RTMP::EncodeString(enc, "tvmanele1"); // DEBUG, REMOVE!!
/*
	enc += RTMP::EncodeString(enc, "connect");
	enc += RTMP::EncodeNumber(enc, 1.0);
	*enc++ = 0x03; // Object Datatype
	enc += RTMP::EncodeString(enc, "app", "a1414/e3");
	enc += RTMP::EncodeString(enc, "flashVer", "FMSc/1.0");//WIN 10,3,183,10");
	enc += RTMP::EncodeString(enc, "swfUrl", "http://www.bbc.co.uk/emp/10player.swf");
	enc += RTMP::EncodeString(enc, "tcUrl", "rtmp://bbcmedia.fcod.llnwd.net:1935/a1414/e3?as=adobe-hmac-sha256&av=1&te=connect&mp=iplayerstream/secure_auth/480kbps/MP/b015j0kk_1317661142.mp4,iplayerstream/secure_auth/800kbps/MP/b015j0kk_1317661142.mp4,iplayerstream/secure_auth/400kbps/b015j0kk_1317661136.mp4,iplayerstream/secure_auth/1500kbps/MP/b015j0kk_1317659895.mp4&et=1317839470&fmta-token=ab8aba3be574cd5e59b85e7c42400bc7f722ca640bb0a8422ce42840d3b340e1");
	enc += RTMP::EncodeBoolean(enc, "fpad", false);
	enc += RTMP::EncodeNumber(enc, "capabilities", 15.0);
	enc += RTMP::EncodeNumber(enc, "audioCodecs", 0x405); // pcm mp3 aac
	enc += RTMP::EncodeNumber(enc, "videoCodecs", 0xb4); // vp6 vp6a h264 h263
	enc += RTMP::EncodeNumber(enc, "videoFunction", 1.0);
	enc += RTMP::EncodeString(enc, "pageUrl", "http://bbc.co.uk/");

	//
	enc += 2; // end of object - 0x00 0x00 0x09
	*enc++ = 0x09;
	//enc += RTMP::EncodeString(enc, "user"); // DEBUG, REMOVE!!!
	//*enc++ = 0x05;
	//enc += RTMP::EncodeString(enc, "tvmanele1"); // DEBUG, REMOVE!!

	m_playpath = "mp4:iplayerstream/secure_auth/1500kbps/MP/b015j0kk_1317659895.mp4";
*/
/*
	enc += RTMP::EncodeString(enc, "connect");
	enc += RTMP::EncodeNumber(enc, 1.0);
	*enc++ = 0x03; // Object Datatype
	enc += RTMP::EncodeString(enc, "app", "ct-vod/_definst_?id=HRqgL70nNFgHb2a-02&publisher=lss");
	enc += RTMP::EncodeString(enc, "flashVer", "FMSc/1.0");
	enc += RTMP::EncodeString(enc, "swfUrl", m_swfUrl.c_str());
	enc += RTMP::EncodeString(enc, "tcUrl", m_url.c_str());
	enc += RTMP::EncodeBoolean(enc, "fpad", false);
	enc += RTMP::EncodeNumber(enc, "capabilities", 15.0);
	enc += RTMP::EncodeNumber(enc, "audioCodecs", 0x405); // pcm mp3 aac
	enc += RTMP::EncodeNumber(enc, "videoCodecs", 0xb4); // vp6 vp6a h264 h263
	enc += RTMP::EncodeNumber(enc, "videoFunction", 1.0);
	enc += RTMP::EncodeString(enc, "pageUrl", m_pageUrl.c_str());

	//
	enc += 2; // end of object - 0x00 0x00 0x09
	*enc++ = 0x09;
	//enc += RTMP::EncodeString(enc, "user"); // DEBUG, REMOVE!!!
	//*enc++ = 0x05;
	//enc += RTMP::EncodeString(enc, "tvmanele1"); // DEBUG, REMOVE!!

	m_playpath = "mp4:ct/iVysilani/2010/06/08/100LetCeskehoVysCT4-080610_1000.mp4";
*/

	packet.m_nBodySize = enc - packet.m_body;

	return SendPacket(packet);
}

bool RTMPSocket::HandShake()
{
	vector<char> clientsig(RTMP_SIG_SIZE + 1);
	vector<char> serversig(RTMP_SIG_SIZE);

	clientsig[0] = 0x03; // not encrypted

	int* t = (int*)&clientsig[1];

	t[0] = htonl(timeGetTime());
	t[1] = 0;

	for(int i = 9; i <= RTMP_SIG_SIZE; i++)
	{
		clientsig[i] = i;
	}

	if(!Write(&clientsig[0], RTMP_SIG_SIZE + 1))
	{
		return false;
	}

	char type;

	if(!ReadAll(&type, 1) || !ReadAll(&serversig[0], RTMP_SIG_SIZE)) // type == 0x03 or 0x06
	{
		return false;
	}

	if(type != clientsig[0])
	{
		// Log(LOGWARNING, "%s: Type mismatch: client sent %d, server answered %d", __FUNCTION__, clientsig[0], type);
	}

	// decode server response

	unsigned int suptime = ntohl(*(int*)&serversig[0]);

	// 2nd part of handshake

	char resp[RTMP_SIG_SIZE];

	if(!ReadAll(resp, RTMP_SIG_SIZE))
	{
		return false;
	}

	if(memcmp(resp, &clientsig[1], RTMP_SIG_SIZE) != 0)
	{
		// Log(LOGWARNING, "%s, client signiture does not match!",__FUNCTION__);
	}

	if(!Write(&serversig[0], RTMP_SIG_SIZE))
	{
		return false;
	}

	return true;
}

bool RTMPSocket::SendServerBW()
{
	RTMPPacket packet;

	packet.m_nChannel = 0x02; // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_packetType = 0x05; // Server BW

	packet.AllocPacket(4);

	//RTMP::EncodeInt32(packet.m_body, 1250000); // hard coded for now
	RTMP::EncodeInt32(packet.m_body, 2500000); // hard coded for now

	packet.m_nBodySize = 4;

	return SendPacket(packet);
}

bool RTMPSocket::SendCheckBW()
{
	RTMPPacket packet;

	packet.m_nChannel = 0x03; // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_packetType = 0x14; // INVOKE
	packet.m_nInfoField1 = timeGetTime();

	packet.AllocPacket(256); // should be enough

	char *enc = packet.m_body;

	enc += RTMP::EncodeString(enc, "_checkbw");
	enc += RTMP::EncodeNumber(enc, 0x00);
	*enc++ = 0x05; // NULL

	packet.m_nBodySize = enc - packet.m_body;

	return SendPacket(packet);
}

bool RTMPSocket::SendCheckBWResult()
{
	RTMPPacket packet;

	packet.m_nChannel = 0x03; // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet.m_packetType = 0x14; // INVOKE
	packet.m_nInfoField1 = 0x16 * m_nBWCheckCounter; // temp inc value. till we figure it out.

	packet.AllocPacket(256); // should be enough

	char *enc = packet.m_body;
	enc += RTMP::EncodeString(enc, "_result");
	enc += RTMP::EncodeNumber(enc, (double)timeGetTime()); // temp
	*enc++ = 0x05; // NULL
	enc += RTMP::EncodeNumber(enc, (double)m_nBWCheckCounter++); 

	packet.m_nBodySize = enc - packet.m_body;

	return SendPacket(packet);
}

bool RTMPSocket::SendPing(short nType, unsigned int nObject, unsigned int nTime)
{
	RTMPPacket packet; 

	packet.m_nChannel = 0x02; // control channel (ping)
	packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet.m_packetType = 0x04; // ping
	packet.m_nInfoField1 = timeGetTime();

	int nSize = (nType == 0x03 ? 10 : 6); // type 3 is the buffer time and requires all 3 parameters. all in all 10 bytes.
	
	if(nType == 0x1B) 
	{
		nSize = 44;
	}

	packet.AllocPacket(nSize);

	packet.m_nBodySize = nSize;

	char *buf = packet.m_body;

	buf += RTMP::EncodeInt16(buf, nType);

	if(nType == 0x1B)
	{
		// ?
	}
	else
	{
		if(nSize > 2)
		{
			buf += RTMP::EncodeInt32(buf, nObject);
		}

		if(nSize > 6)
		{
			buf += RTMP::EncodeInt32(buf, nTime);
		}
	}

	return SendPacket(packet);
}

bool RTMPSocket::SendBGHasStream(double dId, const char* playpath)
{
	RTMPPacket packet;

	packet.m_nChannel = 0x03; // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet.m_packetType = 0x14; // INVOKE

	packet.AllocPacket(256); // should be enough

	char *enc = packet.m_body;

	enc += RTMP::EncodeString(enc, "bgHasStream");
	enc += RTMP::EncodeNumber(enc, dId);
	*enc++ = 0x05; // NULL
	enc += RTMP::EncodeString(enc, playpath);

	packet.m_nBodySize = enc - packet.m_body;

	return SendPacket(packet);
}

bool RTMPSocket::SendCreateStream(double dStreamId)
{
	RTMPPacket packet;

	packet.m_nChannel = 0x03; // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet.m_packetType = 0x14; // INVOKE

	packet.AllocPacket(256); // should be enough

	char *enc = packet.m_body;

	enc += RTMP::EncodeString(enc, "createStream");
	enc += RTMP::EncodeNumber(enc, dStreamId);
	*enc++ = 0x05; // NULL

	packet.m_nBodySize = enc - packet.m_body;

	return SendPacket(packet);
}

bool RTMPSocket::SendPlay()
{
	RTMPPacket packet;

	packet.m_nChannel = 0x08; // we make 8 our stream channel
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_packetType = 0x14; // INVOKE
	packet.m_nInfoField2 = m_stream_id; // 0x01000000;

	packet.AllocPacket(256); // should be enough

	char *enc = packet.m_body;

	enc += RTMP::EncodeString(enc, "play");
	enc += RTMP::EncodeNumber(enc, 0.0);
	*enc++ = 0x05; // NULL
	enc += RTMP::EncodeString(enc, m_playpath.c_str());
	enc += RTMP::EncodeNumber(enc, -2.0); // start (live)
	enc += RTMP::EncodeNumber(enc, -1.0); // duration (inf)

	packet.m_nBodySize = enc - packet.m_body;

	return SendPacket(packet);
}

bool RTMPSocket::SendPause()
{
	RTMPPacket packet;

	packet.m_nChannel = 0x08; // video channel 
	packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet.m_packetType = 0x14; // invoke

	packet.AllocPacket(256); // should be enough

	char *enc = packet.m_body;
	enc += RTMP::EncodeString(enc, "pause");
	enc += RTMP::EncodeNumber(enc, 0);
	*enc++ = 0x05; // NULL
	enc += RTMP::EncodeBoolean(enc, true);
	enc += RTMP::EncodeNumber(enc, 0);

	packet.m_nBodySize = enc - packet.m_body;

	return SendPacket(packet);
}

bool RTMPSocket::SendSeek(double dTime)
{
	RTMPPacket packet;

	packet.m_nChannel = 0x08; // video channel 
	packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet.m_packetType = 0x14; // invoke

	packet.AllocPacket(256); // should be enough

	char *enc = packet.m_body;
	
	enc += RTMP::EncodeString(enc, "seek");
	enc += RTMP::EncodeNumber(enc, 0);
	*enc++ = 0x05; // NULL
	enc += RTMP::EncodeNumber(enc, dTime * 1000);

	packet.m_nBodySize = enc - packet.m_body;

	return SendPacket(packet);
}

bool RTMPSocket::SendBytesReceived()
{
	RTMPPacket packet;

	packet.m_nChannel = 0x02; // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet.m_packetType = 0x03; // bytes in

	packet.AllocPacket(4);

	packet.m_nBodySize = 4;

	RTMP::EncodeInt32(packet.m_body, m_nBytesIn); // hard coded for now

	m_nBytesInSent = m_nBytesIn;

	return SendPacket(packet);
}

void RTMPSocket::HandleInvoke(const char* body, unsigned int nBodySize)
{
	if(body[0] != 0x02) // make sure it is a string method name we start with
	{
		return;
	}

	AMFObject obj;

	if(obj.Decode(body, nBodySize) < 0)
	{
		return;
	}

	std::string method = obj.GetProperty(0).GetString();

	printf("method: %s\n", method.c_str());

	if(method == "_result")
	{
		std::string methodInvoked = m_methodCalls[0];

		m_methodCalls.erase(m_methodCalls.begin());

		printf("methodInvoked: %s\n", methodInvoked.c_str());

		if(methodInvoked == "connect")
		{
			SendServerBW();
			SendPing(3, 0, 300);
			SendCreateStream(2.0);
		}
		else if(methodInvoked == "createStream")
		{
			m_stream_id = (int)obj.GetProperty(3).GetNumber();

			SendPlay();

			if(m_start > 0)
			{
				SendSeek(m_start);
			}

			SendPing(3, 1, m_nBufferMS);
		}
		else if(methodInvoked == "play")
		{
		}
	}
	else if(method == "onBWDone")
	{
		//SendCheckBW();
	}
	else if(method == "_onbwcheck")
	{
		SendCheckBWResult();
	}
	else if(method == "_error")
	{
		// TODO
	}
	else if(method == "close")
	{
		Close();
	}
	else if(method == "onStatus")
	{
		std::string code  = obj.GetProperty(3).GetObject().GetProperty("code").GetString();
		std::string level = obj.GetProperty(3).GetObject().GetProperty("level").GetString();

		if(code == "NetStream.Failed"
		|| code == "NetStream.Play.Failed"
		|| code == "NetStream.Play.Stop"
		|| code == "NetStream.Play.StreamNotFound"
		|| code == "NetConnection.Connect.InvalidApp")
		{
			Close();
		}

		//if (code == "NetStream.Play.Complete")

		/*if(Link.seekTime > 0) {
		if(code == "NetStream.Seek.Notify") { // seeked successfully, can play now!
		bSeekedSuccessfully = true;
		} else if(code == "NetStream.Play.Start" && !bSeekedSuccessfully) { // well, try to seek again
		Log(LOGWARNING, "%s, server ignored seek!", __FUNCTION__);
		}
		}*/
	}
	else
	{

	}
}

void RTMPSocket::HandleMetadata(char* body, unsigned int len)
{
	AMFObject obj;

	if(obj.Decode(body, len) < 0)
	{
		return;
	}

	std::string metastring = obj.GetProperty(0).GetString();

	if(metastring == "onMetaData")
	{
		AMFObjectProperty prop;

		if(obj.FindFirstMatchingProperty("duration", prop))
		{
			m_duration = prop.GetNumber();
		}
	}
}

void RTMPSocket::HandleChangeChunkSize(const RTMPPacket& packet)
{
	if(packet.m_nBodySize >= 4)
	{
		m_chunkSize = RTMP::ReadInt32(packet.m_body);
	}
}

void RTMPSocket::HandleAudio(const RTMPPacket& packet)
{
}

void RTMPSocket::HandleVideo(const RTMPPacket& packet)
{
}

void RTMPSocket::HandlePing(const RTMPPacket& packet)
{
	short nType = -1;

	if(packet.m_body && packet.m_nBodySize >= 2)
	{
		nType = RTMP::ReadInt16(packet.m_body);
	}

	switch(nType)
	{
	case 0x06:

		if(packet.m_nBodySize >= 6) // server ping. reply with pong.
		{
			unsigned int nTime = RTMP::ReadInt32(packet.m_body + 2);

			SendPing(0x07, nTime);
		}

		break;

	case 0x1a:

		// respond with HMAC SHA256 of decompressed SWF, key is the 30byte player key, also the last 30 bytes of the server handshake are applied
/*
		if(Link.SWFHash)
		{
			SendPing(0x1b, 0, 0);
		}
		else
		{
			Log(LOGWARNING, "%s: Ignoring SWFVerification request, use --swfhash and --swfsize!", __FUNCTION__);
		}
*/
		break;

	case 0x1f:

		// TODO

		break;
	}
}

bool RTMPSocket::SendPacket(RTMPPacket& packet)
{
	const RTMPPacket& prevPacket = m_vecChannelsOut[packet.m_nChannel];

	if(packet.m_headerType != RTMP_PACKET_SIZE_LARGE)
	{
		// compress a bit by using the prev packet's attributes

		if(prevPacket.m_nBodySize == packet.m_nBodySize && packet.m_headerType == RTMP_PACKET_SIZE_MEDIUM) 
		{
			packet.m_headerType = RTMP_PACKET_SIZE_SMALL;
		}

		if(prevPacket.m_nInfoField2 == packet.m_nInfoField2 && packet.m_headerType == RTMP_PACKET_SIZE_SMALL)
		{
			packet.m_headerType = RTMP_PACKET_SIZE_MINIMUM;
		}
	}

	if(packet.m_headerType > 3) // sanity
	{ 
		return false;
	}

	int nSize = packetSize[packet.m_headerType];
	
	char header[RTMP_LARGE_HEADER_SIZE] = {(char)((packet.m_headerType << 6) | packet.m_nChannel)};
	
	if(nSize > 1)
	{
		RTMP::EncodeInt24(header +1, packet.m_nInfoField1);
	}

	if(nSize > 4)
	{
		RTMP::EncodeInt24(header + 4, packet.m_nBodySize);

		header[7] = packet.m_packetType;
	}

	if(nSize > 8)
	{
		RTMP::EncodeInt32LE(header + 8, packet.m_nInfoField2);
	}

	if(!Write(header, nSize))
	{
		return false;
	}

	nSize = packet.m_nBodySize;

	char* buffer = packet.m_body;

	while(nSize)
	{
		int nChunkSize = packet.m_packetType == 0x14 ? m_chunkSize : packet.m_nBodySize;

		if(nSize < m_chunkSize)
		{
			nChunkSize = nSize;
		}

		if(!Write(buffer, nChunkSize))
		{
			return false;
		}

		nSize -= nChunkSize;
		buffer += nChunkSize;

		if(nSize > 0)
		{
			if(!Write(0xc0 | packet.m_nChannel))
			{
				return false;  
			}
		}
	}

	if(packet.m_packetType == 0x14) // we invoked a remote method, keep it in call queue till result arrives
	{
		m_methodCalls.push_back(RTMP::ReadString(packet.m_body + 1));
	}

	m_vecChannelsOut[packet.m_nChannel] = packet;
	m_vecChannelsOut[packet.m_nChannel].m_body = NULL;

	return true;
}

bool RTMPSocket::ReadPacket(RTMPPacket& packet)
{
	BYTE type;

	if(!ReadAll(&type, 1))
	{
		return false;
	}

	packet.m_nChannel = (type & 0x3f);

	type = (type & 0xc0) >> 6;

	int nSize = packetSize[type];

	if(nSize < RTMP_LARGE_HEADER_SIZE) // using values from the last message of this channel
	{
		packet = m_vecChannelsIn[packet.m_nChannel];
	}

	packet.m_headerType = type;

	nSize--;

	char header[RTMP_LARGE_HEADER_SIZE] = {0};

	if(nSize > 0 && !ReadAll(header, nSize))
	{
		return false;
	}

	if(nSize >= 3)
	{
		packet.m_nInfoField1 = RTMP::ReadInt24(header);
	}

	if(nSize >= 6)
	{
		packet.m_nBodySize = RTMP::ReadInt24(header + 3);
		packet.m_nBytesRead = 0;

		packet.FreePacketHeader(); // new packet body
	}

	if(nSize > 6)
	{
		packet.m_packetType = header[6];
	}

	if(nSize == 11)
	{
		packet.m_nInfoField2 = RTMP::ReadInt32LE(header + 7);
	}

	if(packet.m_nBodySize > 0)
	{
		if(packet.m_body == NULL)
		{
			packet.AllocPacket(packet.m_nBodySize);
		}

		if(packet.m_body == NULL)
		{
			return false;
		}
	}

	int nToRead = packet.m_nBodySize - packet.m_nBytesRead;
	int nChunk = m_chunkSize;

	if(nToRead < nChunk)
	{
		nChunk = nToRead;
	}

	if(!ReadAll(packet.m_body + packet.m_nBytesRead, nChunk))
	{
		packet.m_body = NULL; // we dont want it deleted since its pointed to from the stored packets (m_vecChannelsIn)

		return false;  
	}

	packet.m_nBytesRead += nChunk;

	m_vecChannelsIn[packet.m_nChannel] = packet;

	if(packet.IsReady())
	{
		// reset the data from the stored packet. we keep the header since we may use it later if a new packet for this channel
		// arrives and requests to re-use some info (small packet header)

		m_vecChannelsIn[packet.m_nChannel].m_body = NULL;
		m_vecChannelsIn[packet.m_nChannel].m_nBytesRead = 0;
	}
	else
	{
		packet.m_body = NULL; // so it wont be erased on "free"
	}

	return true;
}

// RTMPPacket

RTMPPacket::RTMPPacket()
{
	Reset();
}

RTMPPacket::~RTMPPacket()
{
	FreePacket();
}

void RTMPPacket::Reset()
{
	m_headerType = 0;
	m_packetType = 0;
	m_nChannel = 0;
	m_nInfoField1 = 0; 
	m_nInfoField2 = 0; 
	m_nBodySize = 0;
	m_nBytesRead = 0;
	m_nInternalTimestamp = 0;
	m_body = NULL;
}

void RTMPPacket::AllocPacket(int nSize)
{
	m_body = new char[nSize];
	memset(m_body, 0, nSize);
	m_nBytesRead = 0;
}

void RTMPPacket::FreePacket()
{
	FreePacketHeader();
	Reset();
}

void RTMPPacket::FreePacketHeader()
{
	delete [] m_body;
	m_body = NULL;
}
