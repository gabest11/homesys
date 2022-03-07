#pragma once

#include "../3rdparty/tinyxml/tinyxml.h"
#include <list>
#include <string>
#include "../util/String.h"

namespace Silverlight
{
	class QualityLevel
	{
	public:
		// codec

		int m_bitrate;
		std::string m_fourcc;
		std::string m_private;

		// video

		int m_width;
		int m_heigth;

		// audio 

		int m_freq;
		int m_channels;
		int m_bps;
		int m_packetsize;
		int m_tag;

	public:
		QualityLevel()
		{
			m_bitrate = 0;
			m_width = 0;
			m_heigth = 0;
			m_freq = 0;
			m_channels = 0;
			m_bps = 0;
			m_packetsize = 0;
			m_tag = 0;
		}

		bool Parse(TiXmlElement* node, const std::string& type)
		{
			if(node == NULL) return false;

			if(node->QueryIntAttribute("Bitrate", &m_bitrate) != TIXML_SUCCESS) return false;
			if(node->QueryValueAttribute("FourCC", &m_fourcc) != TIXML_SUCCESS) return false;
			if(node->QueryValueAttribute("CodecPrivateData", &m_private) != TIXML_SUCCESS) return false;

			if(type == "video")
			{
				if(node->QueryIntAttribute("MaxWidth", &m_width) != TIXML_SUCCESS) return false;
				if(node->QueryIntAttribute("MaxHeight", &m_heigth) != TIXML_SUCCESS) return false;
			}
			else if(type == "audio")
			{
				if(node->QueryIntAttribute("SamplingRate", &m_freq) != TIXML_SUCCESS) return false;
				if(node->QueryIntAttribute("Channels", &m_channels) != TIXML_SUCCESS) return false;
				if(node->QueryIntAttribute("BitsPerSample", &m_bps) != TIXML_SUCCESS) return false;
				if(node->QueryIntAttribute("PacketSize", &m_packetsize) != TIXML_SUCCESS) return false;
				if(node->QueryIntAttribute("AudioTag", &m_tag) != TIXML_SUCCESS) return false;
			}
			else
			{
				return false;
			}

			return true;
		}

		DWORD GetFOURCC() const
		{
			DWORD fcc = 0;

			if(m_fourcc.size() == 4)
			{
				for(int i = 3; i >= 0; i--)
				{
					fcc = (fcc << 8) | (BYTE)m_fourcc[i];
				}
			}

			return fcc;
		}
	};

	class StreamIndex
	{
	public:
		std::string m_type;
		std::string m_url;
		std::list<QualityLevel> m_levels;
		std::list<__int64> m_chunks;
		__int64 m_start;

	public:
		StreamIndex() 
			: m_start(0) 
		{
		}

		bool Parse(TiXmlElement* node)
		{
			if(node == NULL) return false;

			if(node->QueryValueAttribute("Type", &m_type) != TIXML_SUCCESS) return false;
			if(node->QueryValueAttribute("Url", &m_url) != TIXML_SUCCESS) return false;

			for(TiXmlElement* n = node->FirstChildElement("QualityLevel"); 
				n != NULL; 
				n = n->NextSiblingElement("QualityLevel"))
			{
				QualityLevel l;

				if(l.Parse(n, m_type))
				{
					m_levels.push_back(l);
				}
			}

			__int64 t0 = 0;

			for(TiXmlElement* n = node->FirstChildElement("c"); 
				n != NULL; 
				n = n->NextSiblingElement("c"))
			{
				__int64 t, d;

				if(n->QueryInt64Attribute("t", &t) == TIXML_SUCCESS) 
				{
					m_chunks.push_back(t);
				}
				else if(n->QueryInt64Attribute("d", &d) == TIXML_SUCCESS) 
				{
					m_chunks.push_back(t0);

					t0 += d;
				}
				else
				{
					return !m_chunks.empty();
				}
			}

			return true;
		}
	};

	class PlayReady
	{
	public:
		enum {ALGORITHM_NONE, ALGORITHM_CTR, ALGORITHM_CBC};

		std::wstring m_hdr; // TODO
		std::wstring m_store; // TODO

		int m_algorithm;
		int m_iv_size;
		unsigned char m_kid[16];

	public:
		PlayReady()
		{
			m_algorithm = ALGORITHM_NONE;
			m_iv_size = 0;
			memset(m_kid, 0, sizeof(m_kid));
		}
	};

	class SmoothStreamingMedia
	{
	public:
		struct {int major, minor;} m_version;
		__int64 m_duration;
		__int64 m_start;
		std::list<StreamIndex> m_streams;
		std::list<__int64> m_markers;
		struct {std::string id, value;} m_protection;
		PlayReady m_playready;

	public:
		SmoothStreamingMedia()
		{
			m_version.major = 0;
			m_version.minor = 0;
			m_duration = 0;
			m_start = 0;
			m_protection.id.clear();
			m_protection.value.clear();
		}

		bool Parse(TiXmlElement* node)
		{
			if(node == NULL) return false;

			if(node->QueryIntAttribute("MajorVersion", &m_version.major) != TIXML_SUCCESS) return false;
			if(node->QueryIntAttribute("MinorVersion", &m_version.minor) != TIXML_SUCCESS) return false;	
			if(node->QueryInt64Attribute("Duration", &m_duration) != TIXML_SUCCESS) return false;

			m_start = _I64_MAX;

			for(TiXmlElement* n = node->FirstChildElement("StreamIndex"); 
				n != NULL; 
				n = n->NextSiblingElement("StreamIndex"))
			{
				StreamIndex s;

				if(s.Parse(n))
				{
					m_streams.push_back(s);

					if(!s.m_chunks.empty())
					{
						m_start = std::min<__int64>(m_start, s.m_chunks.front());
					}
				}
			}

			if(m_start != 0 && m_start != _I64_MAX)
			{
				for(auto j = m_streams.begin(); j != m_streams.end(); j++)
				{
					j->m_start = m_start;

					for(auto i = j->m_chunks.begin(); i != j->m_chunks.end(); i++)
					{
						*i -= m_start;
					}
				}
			}

			if(TiXmlElement* p = node->FirstChildElement("Protection"))
			{
				if(TiXmlElement* ph = p->FirstChildElement("ProtectionHeader"))
				{
					ph->QueryValueAttribute("SystemID", &m_protection.id);

					m_protection.value = ph->GetText();

					if(_stricmp(m_protection.id.c_str(), "9A04F079-9840-4286-AB92-E65BE0885F95") == 0)
					{
						// set default values for PlayReady

						m_playready.m_algorithm = PlayReady::ALGORITHM_CTR;
						m_playready.m_iv_size = 8;

						//

						std::string buff = Util::Base64Decode(m_protection.value);

						const char* ptr = buff.data();
						const char* endptr = ptr + buff.size();

						if(ptr + 6 <= endptr)
						{
							unsigned int size = *(unsigned int*)&ptr[0];
							unsigned short count = *(unsigned short*)&ptr[4];

							ptr += 6;

							for(unsigned short i = 0; i < count && ptr + 4 <= endptr; i++)
							{
								unsigned short rtype = *(unsigned short*)&ptr[0];
								unsigned short rlen = *(unsigned short*)&ptr[2];

								const char* data = (const char*)&ptr[4];

								switch(rtype)
								{
								case 1:
 									m_playready.m_hdr = std::wstring((const wchar_t*)data, rlen / 2); // TODO: process this xml, move values into struct elements
									break;
								case 3:
									m_playready.m_store = std::wstring((const wchar_t*)data, rlen / 2); // TODO: ???
									break;
								}

								ptr += 4 + rlen;
							}
						}

						ASSERT(ptr == endptr);
					}
				}
			}

			return true;
		}
	};
}