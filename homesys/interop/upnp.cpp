#include "stdafx.h"
#include "upnp.h"
#include "convert.h"
#include "managed_ptr.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::IO;
using namespace System::Net;
using namespace System::Text;
using namespace System::Diagnostics;
using namespace System::Runtime::InteropServices;

namespace Homesys
{
	public ref class ConnectionManager : public UPnP::ConnectionManager
	{
		HWND m_wnd;
		UINT m_message;

	public:
		ConnectionManager(HWND wnd, UINT message)
			: m_wnd(wnd)
			, m_message(message)
		{
		}

		virtual property String^ SourceProtocolInfo
		{
			String^ get() override
			{
				return "";
			}
		}

		virtual property String^ SinkProtocolInfo
		{
			String^ get() override
			{
				// return "http-get:*:*:*";

				return "http-get:*:video/avi:*,http-get:*:video/video/x-msvideo:*,http-get:*:video/divx:*,http-get:*:video/x-matroska:*,http-get:*:video/mpeg:*";
			}
		}

		virtual property String^ CurrentConnectionIDs
		{
			String^ get() override
			{
				return "0";
			}
		}

		virtual void GetCurrentConnectionIDs(String^% ConnectionIDs) override
        {
            ConnectionIDs = CurrentConnectionIDs;
        }

		virtual void GetCurrentConnectionInfo(int ConnectionID, int% RcsID, int% AVTransportID, String^% ProtocolInfo, String^% PeerConnectionManager, int% PeerConnectionID, String^% Direction, String^% Status) override
        {
            RcsID = -1;
			AVTransportID = -1;
			ProtocolInfo = "";
			PeerConnectionManager = "";
			PeerConnectionID = -1;
			Direction = "Input";
			Status = "OK";
			/*
			<RcsID>0</RcsID>
			<AVTransportID>0</AVTransportID>
			<ProtocolInfo>:::</ProtocolInfo>
			<PeerConnectionManager>/</PeerConnectionManager>
			<PeerConnectionID>-1</PeerConnectionID>
			<Direction>Input</Direction>
			<Status>Unknown</Status>
			*/
        }

		virtual void GetProtocolInfo(String^% Source, String^% Sink) override
        {
			Source = SourceProtocolInfo;
			Sink = SinkProtocolInfo;
        }
	};

	struct HMS {int h, m, s;};

	public ref class AVTransport : public UPnP::AVTransport
	{
		HWND m_wnd;
		UINT m_message;

		HMS RT2HMS(__int64 rt)
		{
			HMS hms;

			rt /= 10000000;

			hms.s = (int)(rt % 60);

			rt /= 60;

			hms.m = (int)(rt % 60);
	
			rt /= 60;

			hms.h = (int)(rt);

			return hms;
		}

	public:
		AVTransport(HWND wnd, UINT message)
			: m_wnd(wnd)
			, m_message(message)
		{
		}

		virtual property String^ LastChange
		{
			String^ get() override
			{
				return "";
			}
		}

        virtual void GetPositionInfo(UInt32 InstanceID, UInt32% Track, String^% TrackDuration, String^% TrackMetaData, String^% TrackURI, String^% RelTime, String^% AbsTime, Int32% RelCount, Int32% AbsCount) override
        {
			std::wstring state;

			SendMessage(m_wnd, m_message, ID_UPNP_GET_STATE, (LPARAM)&state);

			if(state != L"NO_MEDIA_PRESENT")
			{
				__int64 p, d;

				SendMessage(m_wnd, m_message, ID_UPNP_GET_POS, (LPARAM)&p);
				SendMessage(m_wnd, m_message, ID_UPNP_GET_DUR, (LPARAM)&d);

				HMS pos = RT2HMS(p);
				HMS dur = RT2HMS(d);

				Track = 1;
				TrackDuration = String::Format("{0:d2}:{1:d2}:{2:d2}", dur.h, dur.m, dur.s);
				TrackMetaData = nullptr; // TODO
				TrackURI = nullptr; // TODO
				RelTime = String::Format("{0:d2}:{1:d2}:{2:d2}", pos.h, pos.m, pos.s);
				AbsTime = "NOT_IMPLEMENTED";
				RelCount = Int32::MaxValue;
				AbsCount = Int32::MaxValue;
			}
			else
			{
				Track = 0;
				TrackDuration = "00:00:00";
				TrackMetaData = nullptr; // TODO
				TrackURI = nullptr; // TODO
				RelTime = "NOT_IMPLEMENTED";
				AbsTime = "NOT_IMPLEMENTED";
				RelCount = Int32::MaxValue;
				AbsCount = Int32::MaxValue;
			}

			/*
			<Track>0</Track>
			<TrackDuration>00:00:00</TrackDuration>
			<TrackMetaData />
			<TrackURI />
			<RelTime>NOT_IMPLEMENTED</RelTime>
			<AbsTime>NOT_IMPLEMENTED</AbsTime>
			<RelCount>2147483647</RelCount>
			<AbsCount>2147483647</AbsCount>

			<Track>1</Track>
			<TrackDuration>00:00:00</TrackDuration>
			<TrackMetaData>&lt;DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/"&gt;
				&lt;item id="UNKNOWN" parentID="UNKNOWN" restricted="1"&gt;
				&lt;dc:title&gt;&lt;/dc:title&gt;
				&lt;upnp:class&gt;object.item&lt;/upnp:class&gt;
				&lt;dc:creator&gt;&lt;/dc:creator&gt;
				&lt;upnp:storageMedium&gt;UNKNOWN&lt;/upnp:storageMedium&gt;
				&lt;upnp:writeStatus&gt;UNKNOWN&lt;/upnp:writeStatus&gt;
				&lt;/item&gt;
				&lt;/DIDL-Lite&gt;</TrackMetaData>
			<TrackURI>http://192.168.0.100:9000/disk/photo/DLNA-PNJPEG_LRG-OP01-FLAGS00f00000/O939524126$1745052793.jpg</TrackURI>
			<RelTime>00:00:00</RelTime>
			<AbsTime>NOT_IMPLEMENTED</AbsTime>
			<RelCount>2147483647</RelCount>
			<AbsCount>2147483647</AbsCount>
			*/
        }

        virtual void GetTransportInfo(UInt32 InstanceID, String^% CurrentTransportState, String^% CurrentTransportStatus, String^% CurrentSpeed) override
        {
			std::wstring state;

			SendMessage(m_wnd, m_message, ID_UPNP_GET_STATE, (LPARAM)&state);

			CurrentTransportState = Convert(state.c_str());
            CurrentTransportStatus = "OK";
			CurrentSpeed = "1";
        }

        virtual void GetTransportSettings(UInt32 InstanceID, String^% PlayMode, String^% RecQualityMode) override
        {
			PlayMode = "NORMAL";
			RecQualityMode = "0:BASIC"; // ?
        }

        virtual void SetAVTransportURI(UInt32 InstanceID, String^ CurrentURI, String^ CurrentURIMetaData) override
        {
			std::wstring url = Convert(CurrentURI);

			SendMessage(m_wnd, m_message, ID_UPNP_OPEN, (LPARAM)url.c_str());
        }

        virtual void Stop(UInt32 InstanceID) override
        {
            PostMessage(m_wnd, m_message, ID_UPNP_STOP, 0);
        }

        virtual void Pause(UInt32 InstanceID) override
        {
             PostMessage(m_wnd, m_message, ID_UPNP_PAUSE, 0);
        }

        virtual void Play(UInt32 InstanceID, String^ Speed) override
        {
			PostMessage(m_wnd, m_message, ID_UPNP_PLAY, 0);
        }

        virtual void Previous(UInt32 InstanceID) override
        {
			PostMessage(m_wnd, m_message, ID_UPNP_PREV, 0);
        }

        virtual void Next(UInt32 InstanceID) override
        {
			PostMessage(m_wnd, m_message, ID_UPNP_NEXT, 0);
        }

        virtual void Seek(UInt32 InstanceID, String^ Unit, String^ Target) override
        {
			if(Unit == "REL_TIME")
			{
				// TODO: parse Target into REFERENCE_TIME

				std::wstring target = Convert(Target);

				SendMessage(m_wnd, m_message, ID_UPNP_SEEK, (LPARAM)target.c_str());

				return;
			}

            throw gcnew NotImplementedException();

			/*
			<allowedValue>TRACK_NR</allowedValue>
			<allowedValue>REL_TIME</allowedValue>
			<allowedValue>ABS_TIME</allowedValue>
			*/
        }
	};

	public ref class RenderingControl : public UPnP::RenderingControl
	{
		HWND m_wnd;
		UINT m_message;

	public:
		RenderingControl(HWND wnd, UINT message)
			: m_wnd(wnd)
			, m_message(message)
		{
		}

		virtual property String^ LastChange
		{
			String^ get() override
			{
				return "";
			}
		}

        virtual void GetVolume(UInt32 InstanceID, String^ Channel, UInt16% CurrentVolume) override
        {
			float f = 0;

			SendMessage(m_wnd, m_message, ID_UPNP_GET_VOLUME, (LPARAM)&f);

			CurrentVolume = (UInt16)(f * 100);
        }

        virtual void GetMute(UInt32 InstanceID, String^ Channel, bool% CurrentMute) override
        {
			bool b = false;

			SendMessage(m_wnd, m_message, ID_UPNP_GET_MUTE, (LPARAM)&b);

			CurrentMute = b;
        }

        virtual void SetVolume(UInt32 InstanceID, String^ Channel, UInt16 DesiredVolume) override
        {
			float f = (float)DesiredVolume / 100;

			SendMessage(m_wnd, m_message, ID_UPNP_SET_VOLUME, (LPARAM)&f);
        }

        virtual void SetMute(UInt32 InstanceID, String^ Channel, bool DesiredMute) override
        {
			bool b = DesiredMute;

			SendMessage(m_wnd, m_message, ID_UPNP_SET_MUTE, (LPARAM)&b);
        }
	};

	ref class MediaRenderer : public UPnP::DeviceControl
	{
	public:
		MediaRenderer(HWND wnd, UINT message)
		{
            _services["urn:upnp-org:serviceId:ConnectionManager"] = gcnew ConnectionManager(wnd, message);
            _services["urn:upnp-org:serviceId:AVTransport"] = gcnew AVTransport(wnd, message);
            _services["urn:upnp-org:serviceId:RenderingControl"] = gcnew RenderingControl(wnd, message);

			String^ dir = Path::GetDirectoryName(System::Reflection::Assembly::GetExecutingAssembly()->Location);

            Register(dir + "\\UPnP\\MediaRenderer.xml", true);
		}

		~MediaRenderer()
		{
			this->!MediaRenderer();
		}

		!MediaRenderer()
		{
			Unregister();
		}
	};

	class MediaRendererWrapper::MediaRendererPtr : public managed_ptr<MediaRenderer>
	{
	public:
		MediaRendererPtr(HWND wnd, UINT message)
			: managed_ptr(gcnew MediaRenderer(wnd, message))
		{
		}
	};

	MediaRendererWrapper::MediaRendererWrapper(HWND wnd, UINT message)
	{
		try
		{
			m_mr = new MediaRendererPtr(wnd, message);
		}
		catch(Exception^ e)
		{
			Debug::WriteLine(e);
		}
	}

	MediaRendererWrapper::~MediaRendererWrapper()
	{
		delete m_mr;
	}
}