using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;

namespace Homesys
{
    /*
    [ComVisible(true), ClassInterface(ClassInterfaceType.AutoDispatch)]
    public class BaseConnectionManager : ConnectionManager
    {
        public override string CurrentConnectionIDs
        {
            get
            {
                return "0";
            }
        }

        public override int A_ARG_TYPE_AVTransportID
        {
            get
            {
                return 0;
            }
        }

        public override int A_ARG_TYPE_ConnectionID
        {
            get
            {
                return 0;
            }
        }

        public override string A_ARG_TYPE_ConnectionManager
        {
            get
            {
                return "/";
            }
        }

        public override string A_ARG_TYPE_ConnectionStatus
        {
            get
            {
                return "OK";
            }
        }

        public override string A_ARG_TYPE_ProtocolInfo
        {
            get
            {
                return "http-get:*:*:*";
            }
        }

        public override int A_ARG_TYPE_RcsID
        {
            get
            {
                return 0;
            }
        }
    }

    [ComVisible(true), ClassInterface(ClassInterfaceType.AutoDispatch)]
    public class ServerConnectionManager : BaseConnectionManager
    {
        public override string SourceProtocolInfo
        {
            get
            {
                return "*:*:*:*";
            }
        }

        public override string SinkProtocolInfo
        {
            get
            {
                return "*:*:*:*";
            }
        }

        public override string A_ARG_TYPE_Direction
        {
            get
            {
                return "Output";
            }
        }
    }

    [ComVisible(true), ClassInterface(ClassInterfaceType.AutoDispatch)]
    public class RendererConnectionManager : BaseConnectionManager
    {
        public override string SourceProtocolInfo
        {
            get
            {
                return "";
            }
        }

        public override string SinkProtocolInfo
        {
            get
            {
                return "";
            }
        }

        public override string A_ARG_TYPE_Direction
        {
            get
            {
                return "Input";
            }
        }
    }

    [ComVisible(true), ClassInterface(ClassInterfaceType.AutoDispatch)]
    public class HomesysContentDirectory : ContentDirectory
    {
        uint _version = 0;

        public HomesysContentDirectory()
        {
        }

        public override uint A_ARG_TYPE_TransferID
        {
            get
            {
                return 0xffffffff;
            }
        }

        public override string ContainerUpdateIDs
        {
            get
            {
                return "";
            }
        }

        public override string SearchCapabilities
        {
            get
            {
                return "";
            }
        }

        public override string SortCapabilities
        {
            get
            {
                return "";
            }
        }

        public override uint SystemUpdateID
        {
            get
            {
                return _version;
            }
        }

        public override string TransferIDs
        {
            get
            {
                return "";
            }
        }

        public override void Browse(string ObjectID, string BrowseFlag, string Filter, uint StartingIndex, uint RequestedCount, string SortCriteria, out string Result, out uint NumberReturned, out uint TotalMatches, out uint UpdateID)
        {
            Result = 
		        "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">" +
                "<item " +
                " id=\"1\" " +
                " parentID=\"0\" " +
                " childCount=\"0\" " +
                " restricted=\"1\" " +
                " searchable=\"1\" >" +
                "<dc:title>empty</dc:title>" +
                "<res protocolInfo=\"http-get:*:video/mp4:*\">http://tempuri.org</res>" +
                "<upnp:class>object.item.videoItem.movie</upnp:class>" +
                "</item>" +
                "</DIDL-Lite>";

            NumberReturned = 1;
            TotalMatches = 1;

            UpdateID = 1; // TODO
        }

        public override void GetSearchCapabilities(out string SearchCaps)
        {
            SearchCaps = SearchCapabilities;
        }

        public override void GetSortCapabilities(out string SortCaps)
        {
            SortCaps = SortCapabilities;
        }

        public override void GetSystemUpdateID(out uint Id)
        {
            Id = SystemUpdateID;
        }
    }

    [ComVisible(true), ClassInterface(ClassInterfaceType.AutoDispatch)]
    public class HomesysRenderingControl : RenderingControl
    {
        public override string A_ARG_TYPE_Channel
        {
            get
            {
                return "Master";
            }
        }

        public override bool Mute
        {
            get
            {
                return false;
            }
        }

        public override ushort Volume
        {
            get
            {
                return 100;
            }
        }

        public override ushort MasterVolume
        {
            get
            {
                return 100;
            }
        }

        public override void GetMute(uint InstanceID, string Channel, out bool CurrentMute)
        {
            CurrentMute = Mute;
        }

        public override void SetMute(uint InstanceID, string Channel, bool DesiredMute)
        {
            // TODO
        }

        public override void GetVolume(uint InstanceID, string Channel, out ushort CurrentVolume)
        {
            CurrentVolume = Volume;
        }

        public override void SetVolume(uint InstanceID, string Channel, ushort DesiredVolume)
        {
            // TODO
        }

        public override void GetMasterVolume(uint InstanceID, out ushort CurrentVolume)
        {
            CurrentVolume = Volume;
        }

        public override void SetMasterVolume(uint InstanceID, ushort DesiredVolume)
        {
            // TODO
        }
    }

    [ComVisible(true), ClassInterface(ClassInterfaceType.AutoDispatch)]
    public class HomesysAVTransport : AVTransport
    {
        string _uri = "";
        string _metadata = "";
        string _seekmode = "TRACK_NR";
        string _seektarget = "";

        public override string TransportState
        {
            get
            {
                return "NO_MEDIA_PRESENT";
            }
        }

        public override string TransportStatus
        {
            get
            {
                return "OK";
            }
        }

        public override string PlaybackStorageMedium
        {
            get
            {
                return "";
            }
        }

        public override string RecordStorageMedium
        {
            get
            {
                return "";
            }
        }

        public override string PossiblePlaybackStorageMedia
        {
            get
            {
                return "";
            }
        }

        public override string PossibleRecordStorageMedia
        {
            get
            {
                return "";
            }
        }

        public override string CurrentPlayMode
        {
            get
            {
                return "NORMAL";
            }
        }

        public override string TransportPlaySpeed
        {
            get
            {
                return "1";
            }
        }

        public override string RecordMediumWriteStatus
        {
            get
            {
                return "NOT_IMPLEMENTED";
            }
        }

        public override string CurrentRecordQualityMode
        {
            get
            {
                return "NOT_IMPLEMENTED";
            }
        }

        public override string PossibleRecordQualityModes
        {
            get
            {
                return "NOT_IMPLEMENTED";
            }
        }

        public override uint NumberOfTracks
        {
            get
            {
                return 1;
            }
        }

        public override uint CurrentTrack
        {
            get
            {
                return 1;
            }
        }

        public override string CurrentTrackDuration
        {
            get
            {
                return "0:00:00";
            }
        }

        public override string CurrentMediaDuration
        {
            get
            {
                return CurrentTrackDuration;
            }
        }

        public override string CurrentTrackMetaData
        {
            get
            {
                return "NOT_IMPLEMENTED";
            }
        }

        public override string CurrentTrackURI
        {
            get
            {
                return "";
            }
        }

        public override string AVTransportURI
        {
            get
            {
                return _uri;
            }
        }

        public override string AVTransportURIMetaData
        {
            get
            {
                return _metadata;
            }
        }

        public override string NextAVTransportURI
        {
            get
            {
                return "NOT_IMPLEMENTED";
            }
        }

        public override string NextAVTransportURIMetaData
        {
            get
            {
                return "NOT_IMPLEMENTED";
            }
        }

        public override string RelativeTimePosition
        {
            get
            {
                return AbsoluteTimePosition;
            }
        }

        public override string AbsoluteTimePosition
        {
            get
            {
                return "0:00:00";
            }
        }

        public override int RelativeCounterPosition
        {
            get
            {
                return AbsoluteCounterPosition;
            }
        }

        public override int AbsoluteCounterPosition
        {
            get
            {
                return Int32.MaxValue;
            }
        }

        public override string CurrentTransportActions
        {
            get
            {
                return "Play,Stop,Pause,Seek,Next,Previous";
            }
        }

        public override string LastChange
        {
            get
            {
                return String.Format(
                    "<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT_RCS\">" + 
                    "<InstanceID val=\"0\">" +
                    "<TransportState val=\"{0}\" />" + 
                    "</InstanceID>" + 
                    "</Event>",
                    TransportState);
            }
        }

        public override string A_ARG_TYPE_SeekMode
        {
            get
            {
                return _seekmode;
            }
        }

        public override string A_ARG_TYPE_SeekTarget
        {
            get
            {
                return _seektarget;
            }
        }

        public override uint A_ARG_TYPE_InstanceID
        {
            get
            {
                return 0;
            }
        }

        public override void SetAVTransportURI(uint InstanceID, string CurrentURI, string CurrentURIMetaData)
        {
            _uri = CurrentURI;
            _metadata = CurrentURIMetaData;

            // TODO
        }

        public override void SetNextAVTransportURI(uint InstanceID, string NextURI, string NextURIMetaData)
        {
            // TODO
        }

        public override void GetMediaInfo(uint InstanceID, out uint NrTracks, out string MediaDuration, out string CurrentURI, out string CurrentURIMetaData, out string NextURI, out string PlayMedium, out string RecordMedium, out string WriteStatus)
        {
            NrTracks = NumberOfTracks;
            MediaDuration = CurrentMediaDuration;
            CurrentURI = AVTransportURI;
            CurrentURIMetaData = AVTransportURIMetaData;
            NextURI = NextAVTransportURI;
            PlayMedium = PlaybackStorageMedium;
            RecordMedium = RecordStorageMedium;
            WriteStatus = RecordMediumWriteStatus;
        }

        public override void GetTransportInfo(uint InstanceID, out string CurrentTransportState, out string CurrentTransportStatus, out string CurrentSpeed)
        {
            CurrentTransportState = TransportState;
            CurrentTransportStatus = TransportStatus;
            CurrentSpeed = TransportPlaySpeed;
        }

        public override void GetPositionInfo(uint InstanceID, out uint Track, out string TrackDuration, out string TrackMetaData, out string TrackURI, out string RelTime, out string AbsTime, out int RelCount, out int AbsCount)
        {
            Track = CurrentTrack;
            TrackDuration = CurrentTrackDuration;
            TrackMetaData = CurrentTrackMetaData;
            TrackURI = CurrentTrackURI;
            RelTime = RelativeTimePosition;
            AbsTime = AbsoluteTimePosition;
            RelCount = RelativeCounterPosition;
            AbsCount = AbsoluteCounterPosition;
        }

        public override void GetDeviceCapabilities(uint InstanceID, out string PlayMedia, out string RecMedia, out string RecQualityModes)
        {
            PlayMedia = PossiblePlaybackStorageMedia;
            RecMedia = PossibleRecordStorageMedia;
            RecQualityModes = PossibleRecordQualityModes;
        }

        public override void GetTransportSettings(uint InstanceID, out string PlayMode, out string RecQualityMode)
        {
            PlayMode = CurrentPlayMode;
            RecQualityMode = CurrentRecordQualityMode;
        }

        public override void Stop(uint InstanceID)
        {
            // TODO
        }

        public override void Play(uint InstanceID, string Speed)
        {
            // TODO
        }

        public override void Pause(uint InstanceID)
        {
            // TODO
        }

        public override void Record(uint InstanceID)
        {
            // TODO
        }

        public override void Seek(uint InstanceID, string Unit, string Target)
        {
            _seekmode = Unit;
            _seektarget = Target;

            // TODO
        }

        public override void Next(uint InstanceID)
        {
            // TODO
        }

        public override void Previous(uint InstanceID)
        {
            // TODO
        }

        public override void SetPlayMode(uint InstanceID, string NewPlayMode)
        {
            // TODO
        }

        public override void SetRecordQualityMode(uint InstanceID, string NewRecordQualityMode)
        {
            // TODO
        }

        public override void GetCurrentTransportActions(uint InstanceID, out string Actions)
        {
            Actions = ""; // TODO
        }
    }
*/
    public class MediaServer : UPnP.DeviceControl
    {
        public MediaServer()
        {
            string dir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

            Register(dir + @"\UPnP\MediaServer.xml", true);
        }

        public override void Initialize(string bstrXMLDesc, string bstrDeviceIdentifier, string bstrInitString)
        {
            base.Initialize(bstrXMLDesc, bstrDeviceIdentifier, bstrInitString);

            _services["urn:upnp-org:serviceId:ConnectionManager"] = new UPnP.ConnectionManager();
            _services["urn:upnp-org:serviceId:ContentDirectory"] = new UPnP.ContentDirectory();
        }
    }

    public class MediaRenderer : UPnP.DeviceControl
    {
        public MediaRenderer()
        {
            string dir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

            Register(dir + @"\UPnP\MediaRenderer.xml", true);
        }

        public override void Initialize(string bstrXMLDesc, string bstrDeviceIdentifier, string bstrInitString)
        {
            base.Initialize(bstrXMLDesc, bstrDeviceIdentifier, bstrInitString);

            _services["urn:upnp-org:serviceId:ConnectionManager"] = new UPnP.ConnectionManager();
            _services["urn:upnp-org:serviceId:RenderingControl"] = new UPnP.RenderingControl();
            _services["urn:upnp-org:serviceId:AVTransport"] = new UPnP.AVTransport();
        }
    }
    
}
