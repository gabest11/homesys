#pragma once

#include "../3rdparty/tinyxml/tinyxml.h"
#include "../3rdparty/rtmp/amf.h"
#include "../util/String.h"

namespace Strobe
{
	struct Metadata
	{
		double duration;
		int width;
		int height;
		std::string videocodecid;
		std::string audiocodecid;
		int avcprofile;
		int avclevel;
		double aacaot;
		double videoframerate;
		int audiosamplerate;
		int audiochannels;
		// trackinfo
		// custdef

		struct Metadata()
		{
			duration = 0;
			width = height = 0;
			avcprofile = avclevel = 0;
			aacaot = videoframerate = 0;
			audiosamplerate = audiochannels = 0;
		}
	};

	class Media
	{
	public:
		std::string m_streamId;
		std::string m_url;
		int m_bitrate;
		Metadata m_metadata;
		std::string m_bootstrapInfoId;
		std::string m_bootstrapInfo;

	public:
		Media(int bitrate = 0)
		{
			m_bitrate = bitrate;
		}

		bool Parse(TiXmlElement* node)
		{
			if(node == NULL) return false;

			if(node->QueryValueAttribute("streamId", &m_streamId) != TIXML_SUCCESS) return false;
			if(node->QueryValueAttribute("url", &m_url) != TIXML_SUCCESS) return false;
			if(node->QueryValueAttribute("bootstrapInfoId", &m_bootstrapInfoId) != TIXML_SUCCESS) return false;

			if(m_bitrate == 0)
			{
				if(node->QueryIntAttribute("bitrate", &m_bitrate) != TIXML_SUCCESS) return false;
			}

			if(TiXmlElement* n = node->FirstChildElement("metadata"))
			{
				std::string buff = Util::Base64Decode(std::string(n->GetText()));

				AMFObject obj;

				if(obj.Decode(buff.data(), buff.size()) >= 0)
				{
					if(obj.GetProperty(0).GetString() == "onMetaData")
					{
						AMFObjectProperty prop;

						if(obj.FindFirstMatchingProperty("duration", prop)) m_metadata.duration = prop.GetNumber();
						if(obj.FindFirstMatchingProperty("width", prop)) m_metadata.width = (int)prop.GetNumber();
						if(obj.FindFirstMatchingProperty("height", prop)) m_metadata.height = (int)prop.GetNumber();
						if(obj.FindFirstMatchingProperty("videocodecid", prop)) m_metadata.videocodecid = prop.GetString();
						if(obj.FindFirstMatchingProperty("audiocodecid", prop)) m_metadata.audiocodecid = prop.GetString();
						if(obj.FindFirstMatchingProperty("avcprofile", prop)) m_metadata.avcprofile = (int)prop.GetNumber();
						if(obj.FindFirstMatchingProperty("avclevel", prop)) m_metadata.avclevel = (int)prop.GetNumber();
						if(obj.FindFirstMatchingProperty("aacaot", prop)) m_metadata.aacaot = prop.GetNumber();
						if(obj.FindFirstMatchingProperty("videoframerate", prop)) m_metadata.videoframerate = prop.GetNumber();
						if(obj.FindFirstMatchingProperty("audiosamplerate", prop)) m_metadata.audiosamplerate = (int)prop.GetNumber();
						if(obj.FindFirstMatchingProperty("audiochannels", prop)) m_metadata.audiochannels = (int)prop.GetNumber();
					}
				}
			}

			return true;
		}
	};

	class Manifest
	{
	public:
		std::string m_id;
		std::string m_streamType;
		double m_duration;
		std::list<Media> m_media;

	public:
		Manifest()
		{
			m_duration = 0;
		}

		bool Parse(TiXmlElement* node, int bitrate = 0)
		{
			if(node == NULL) return false;

			if(TiXmlElement* n = node->FirstChildElement("id"))
			{
				m_id = Util::Trim(std::string(n->GetText()));
			}
			else
			{
				return false;
			}

			if(TiXmlElement* n = node->FirstChildElement("streamType"))
			{
				m_streamType = Util::Trim(std::string(n->GetText()));
			}
			else
			{
				return false;
			}

			if(TiXmlElement* n = node->FirstChildElement("duration"))
			{
				std::string duration = Util::Trim(std::string(n->GetText()));

				m_duration = atof(duration.c_str());
			}
			else
			{
				return false;
			}

			for(TiXmlElement* n = node->FirstChildElement("media"); 
				n != NULL; 
				n = n->NextSiblingElement("media"))
			{
				Media m(bitrate);

				if(m.Parse(n))
				{
					m_media.push_back(m);
				}
			}

			for(TiXmlElement* n = node->FirstChildElement("bootstrapInfo"); 
				n != NULL; 
				n = n->NextSiblingElement("bootstrapInfo"))
			{
				std::string id;

				if(n->QueryValueAttribute("id", &id) == TIXML_SUCCESS)
				{
					for(auto i = m_media.begin(); i != m_media.end(); i++)
					{
						if(i->m_bootstrapInfoId == id)
						{
							i->m_bootstrapInfo = Util::Base64Decode(std::string(n->GetText()));
						}
					}
				}
			}

			return true;
		}
	};
}