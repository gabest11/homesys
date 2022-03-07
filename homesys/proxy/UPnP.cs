using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Threading;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using Microsoft.Win32;

namespace Homesys.UPnP
{
    [ComImport, Guid("204810BA-73B2-11D4-BF42-00B0D0118B56"), InterfaceType((short)1)]
    public interface IUPnPDeviceControl
    {
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Initialize([In, MarshalAs(UnmanagedType.BStr)] string bstrXMLDesc, [In, MarshalAs(UnmanagedType.BStr)] string bstrDeviceIdentifier, [In, MarshalAs(UnmanagedType.BStr)] string bstrInitString);
        [return: MarshalAs(UnmanagedType.IDispatch)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        object GetServiceObject([In, MarshalAs(UnmanagedType.BStr)] string bstrUDN, [In, MarshalAs(UnmanagedType.BStr)] string bstrServiceId);
    }

    [ComImport, InterfaceType((short)1), Guid("204810B8-73B2-11D4-BF42-00B0D0118B56")]
    public interface IUPnPDeviceProvider
    {
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Start([In, MarshalAs(UnmanagedType.BStr)] string bstrInitString);
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Stop();
    }

    [ComImport, InterfaceType((short)1), Guid("204810B4-73B2-11D4-BF42-00B0D0118B56")]
    public interface IUPnPEventSink
    {
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime), TypeLibFunc((short)0x40)]
        void OnStateChanged([In] uint cChanges, [In] ref int rgdispidChanges);
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void OnStateChangedSafe([In, MarshalAs(UnmanagedType.Struct)] object varsadispidChanges);
    }

    [ComImport, InterfaceType((short)1), Guid("204810B5-73B2-11D4-BF42-00B0D0118B56")]
    public interface IUPnPEventSource
    {
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Advise([In, MarshalAs(UnmanagedType.Interface)] IUPnPEventSink pesSubscriber);
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Unadvise([In, MarshalAs(UnmanagedType.Interface)] IUPnPEventSink pesSubscriber);
    }

    [ComImport, Guid("204810B6-73B2-11D4-BF42-00B0D0118B56"), InterfaceType((short)1)]
    public interface IUPnPRegistrar
    {
        [return: MarshalAs(UnmanagedType.BStr)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        string RegisterDevice([In, MarshalAs(UnmanagedType.BStr)] string bstrXMLDesc, [In, MarshalAs(UnmanagedType.BStr)] string bstrProgIDDeviceControlClass, [In, MarshalAs(UnmanagedType.BStr)] string bstrInitString, [In, MarshalAs(UnmanagedType.BStr)] string bstrContainerId, [In, MarshalAs(UnmanagedType.BStr)] string bstrResourcePath, [In] int nLifeTime);
        [return: MarshalAs(UnmanagedType.BStr)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        string RegisterRunningDevice([In, MarshalAs(UnmanagedType.BStr)] string bstrXMLDesc, [In, MarshalAs(UnmanagedType.IUnknown)] object punkDeviceControl, [In, MarshalAs(UnmanagedType.BStr)] string bstrInitString, [In, MarshalAs(UnmanagedType.BStr)] string bstrResourcePath, [In] int nLifeTime);
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void RegisterDeviceProvider([In, MarshalAs(UnmanagedType.BStr)] string bstrProviderName, [In, MarshalAs(UnmanagedType.BStr)] string bstrProgIDProviderClass, [In, MarshalAs(UnmanagedType.BStr)] string bstrInitString, [In, MarshalAs(UnmanagedType.BStr)] string bstrContainerId);
        [return: MarshalAs(UnmanagedType.BStr)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        string GetUniqueDeviceName([In, MarshalAs(UnmanagedType.BStr)] string bstrDeviceIdentifier, [In, MarshalAs(UnmanagedType.BStr)] string bstrTemplateUDN);
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void UnregisterDevice([In, MarshalAs(UnmanagedType.BStr)] string bstrDeviceIdentifier, [In] int fPermanent);
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void UnregisterDeviceProvider([In, MarshalAs(UnmanagedType.BStr)] string bstrProviderName);
    }

    [ComImport, Guid("C92EB863-0269-4AFF-9C72-75321BBA2952"), InterfaceType((short)1)]
    public interface IUPnPRemoteEndpointInfo
    {
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void GetDwordValue([In, MarshalAs(UnmanagedType.BStr)] string bstrValueName, out uint pdwValue);
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void GetStringValue([In, MarshalAs(UnmanagedType.BStr)] string bstrValueName, [MarshalAs(UnmanagedType.BStr)] out string pbstrValue);
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void GetGuidValue([In, MarshalAs(UnmanagedType.BStr)] string bstrValueName, out Guid pguidValue);
    }

    [ComImport, InterfaceType((short)1), Guid("204810B7-73B2-11D4-BF42-00B0D0118B56")]
    public interface IUPnPReregistrar
    {
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void ReregisterDevice([In, MarshalAs(UnmanagedType.BStr)] string bstrDeviceIdentifier, [In, MarshalAs(UnmanagedType.BStr)] string bstrXMLDesc, [In, MarshalAs(UnmanagedType.BStr)] string bstrProgIDDeviceControlClass, [In, MarshalAs(UnmanagedType.BStr)] string bstrInitString, [In, MarshalAs(UnmanagedType.BStr)] string bstrContainerId, [In, MarshalAs(UnmanagedType.BStr)] string bstrResourcePath, [In] int nLifeTime);
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void ReregisterRunningDevice([In, MarshalAs(UnmanagedType.BStr)] string bstrDeviceIdentifier, [In, MarshalAs(UnmanagedType.BStr)] string bstrXMLDesc, [In, MarshalAs(UnmanagedType.IUnknown)] object punkDeviceControl, [In, MarshalAs(UnmanagedType.BStr)] string bstrInitString, [In, MarshalAs(UnmanagedType.BStr)] string bstrResourcePath, [In] int nLifeTime);
    }

    [ComImport, CoClass(typeof(UPnPRegistrarClass)), Guid("204810B9-73B2-11D4-BF42-00B0D0118B56")]
    public interface UPnPRegistrar
    {
    }

    [ComImport, TypeLibType((short)2), ClassInterface((short)0), Guid("204810B9-73B2-11D4-BF42-00B0D0118B56")]
    public class UPnPRegistrarClass : UPnPRegistrar
    {
    }

    [ComImport, CoClass(typeof(UPnPRemoteEndpointInfoClass)), Guid("2E5E84E9-4049-4244-B728-2D24227157C7")]
    public interface UPnPRemoteEndpointInfo
    {
    }

    [ComImport, TypeLibType((short)2), ClassInterface((short)0), Guid("2E5E84E9-4049-4244-B728-2D24227157C7")]
    public class UPnPRemoteEndpointInfoClass : UPnPRemoteEndpointInfo
    {
    }

/*
    [XmlRootAttribute("root", Namespace = "urn:schemas-upnp-org:device-1-0")]
    public class ServiceDescription
    {
        [Serializable]
        public class SpecVersion
        {
            public int major = 1;

            public int minor = 0;
        }

        [Serializable]
        public class Service
        {
            public string Optional;
            
            public string serviceType;
            
            public string serviceId;

            public string controlURL;

            public string eventSubURL;

            public string SCPDURL;
        }

        [Serializable]
        public class ServiceList
        {
            [XmlElement("service")]
            public Service[] service;
        }

        [Serializable]
        public class Device
        {
            public string deviceType;
            
            public string friendlyName;
            
            public string manufacturer = "Homesys";
            
            public string manufacturerURL = "http://homesys.tv/";
            
            public string modelURL = "http://homesys.tv/";
            
            public string modelName = "Homesys 4.0";
            
            public string UDN = "uuid:RootDevice";

            public string presentationURL;

            public ServiceList serviceList;
        }

        public SpecVersion specVersion;
        
        public Device device;
    }

    public class StringWriterWithEncoding : StringWriter
    {
        readonly Encoding _encoding;

        public StringWriterWithEncoding(Encoding encoding)
        {
            _encoding = encoding;
        }

        public override Encoding Encoding
        {
            get { return _encoding; }
        }
    }
*/
    public class DeviceControl : IUPnPDeviceControl
    {
        string _path = "";
        string _udn = "";
        string _id = "";
        string _key = "";

        protected Dictionary<string, object> _services = new Dictionary<string, object>();

        Thread _thread;

        public void Register(string path, bool async)
        {
            try
            {
                Unregister();

                _path = path;

                _thread = new Thread(new ThreadStart(ThreadProc));
                _thread.SetApartmentState(ApartmentState.MTA);
                _thread.Start();

                if(!async)
                {
                    _thread.Join();
                    _thread = null;
                }
            }
            catch(Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        public void Unregister()
        {
            if(_id.Length == 0) return;

            try
            {
                if(_thread != null)
                {
                    _thread.Join();
                    _thread = null;
                }

                IUPnPRegistrar upnp = (IUPnPRegistrar)new UPnPRegistrar();

                upnp.UnregisterDevice(_id, 1);

                using(RegistryKey key = Registry.LocalMachine.CreateSubKey(@"Software\Homesys\UPnP"))
                {
                    key.DeleteValue(_key, false);
                }
            }
            catch(Exception e)
            {
                try { Console.WriteLine(e.Message); }
                catch { };
            }
        }

        void ThreadProc()
        {
            try
            {
                IUPnPRegistrar upnp = (IUPnPRegistrar)new UPnPRegistrar();

                string desc = File.ReadAllText(_path);

                // FIXME

                /*
                XmlSerializer s = new XmlSerializer(typeof(ServiceDescription));

                using(XmlTextReader tr = new XmlTextReader(_path))
                {
                    ServiceDescription sd = (ServiceDescription)s.Deserialize(tr);

                    sd.device.friendlyName += " (" + Environment.MachineName + ")";

                    using(StringWriterWithEncoding sw = new StringWriterWithEncoding(Encoding.UTF8))
                    {
                        s.Serialize(sw, sd);

                        desc = sw.ToString();
                    }
                }
                */

                _id = upnp.RegisterRunningDevice(desc, (IUPnPDeviceControl)this, "Hi!", Path.GetDirectoryName(_path), 900);

                using(RegistryKey key = Registry.LocalMachine.CreateSubKey(@"Software\Homesys\UPnP"))
                {
                    _key = Path.GetFileName(_path);

                    key.SetValue(_key, _id);
                }
            }
            catch(Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        public object GetServiceObject(string bstrUDN, string bstrServiceId)
        {
            if(bstrUDN != _udn) throw new COMException();

            object service = null;

            if(!_services.TryGetValue(bstrServiceId, out service))
            {
                throw new COMException();
            }

            return service;
        }

        public virtual void Initialize(string bstrXMLDesc, string bstrDeviceIdentifier, string bstrInitString)
        {
            IUPnPRegistrar upnp = (IUPnPRegistrar)new UPnPRegistrar();

            _udn = upnp.GetUniqueDeviceName(bstrDeviceIdentifier, "uuid:RootDevice");
        }
    }

    //

    [InterfaceType(ComInterfaceType.InterfaceIsDual)]
    public interface IConnectionManager
    {
        string A_ARG_TYPE_ConnectionStatus
        {
            get;
            set;
        }
 
        int A_ARG_TYPE_ConnectionID
        {
            get;
            set;
        }

        string A_ARG_TYPE_Direction
        {
            get;
            set;
        }

        int A_ARG_TYPE_RcsID
        {
            get;
            set;
        }

        string A_ARG_TYPE_ProtocolInfo
        {
            get;
            set;
        }

        string A_ARG_TYPE_ConnectionManager
        {
            get;
            set;
        }

        int A_ARG_TYPE_AVTransportID
        {
            get;
            set;
        }

        string SourceProtocolInfo
        {
            get;
            set;
        }

        string SinkProtocolInfo
        {
            get;
            set;
        }

        string CurrentConnectionIDs
        {
            get;
            set;
        }

        void GetCurrentConnectionIDs(out string ConnectionIDs);

        void GetCurrentConnectionInfo(int ConnectionID, out int RcsID, out int AVTransportID, out string ProtocolInfo, out string PeerConnectionManager, out int PeerConnectionID, out string Direction, out string Status);

        void GetProtocolInfo(out string Source, out string Sink);
    }

    [InterfaceType(ComInterfaceType.InterfaceIsDual)]
    public interface IContentDirectory
    {
        string A_ARG_TYPE_BrowseFlag
        {
            get;
            set;
        }

        uint A_ARG_TYPE_Count
        {
            get;
            set;
        }

        string A_ARG_TYPE_Filter
        {
            get;
            set;
        }

        uint A_ARG_TYPE_Index
        {
            get;
            set;
        }

        string A_ARG_TYPE_ObjectID
        {
            get;
            set;
        }

        string A_ARG_TYPE_Result
        {
            get;
            set;
        }

        string A_ARG_TYPE_SearchCriteria
        {
            get;
            set;
        }

        string A_ARG_TYPE_SortCriteria
        {
            get;
            set;
        }

        string A_ARG_TYPE_TagValueList
        {
            get;
            set;
        }

        uint A_ARG_TYPE_TransferID
        {
            get;
            set;
        }

        string A_ARG_TYPE_TransferLength
        {
            get;
            set;
        }

        string A_ARG_TYPE_TransferStatus
        {
            get;
            set;
        }

        string A_ARG_TYPE_TransferTotal
        {
            get;
            set;
        }

        uint A_ARG_TYPE_UpdateID
        {
            get;
            set;
        }

        Uri A_ARG_TYPE_URI
        {
            get;
            set;
        }

        string SearchCapabilities
        {
            get;
            set;
        }

        string SortCapabilities
        {
            get;
            set;
        }

        string ContainerUpdateIDs
        {
            get;
            set;
        }

        uint SystemUpdateID
        {
            get;
            set;
        }

        string TransferIDs
        {
            get;
            set;
        }

        void GetSearchCapabilities(out string SearchCaps);

        void GetSortCapabilities(out string SortCaps);

        void GetSystemUpdateID(out uint Id);

        void Browse(string ObjectID, string BrowseFlag, string Filter, uint StartingIndex, uint RequestedCount, string SortCriteria, out string Result, out uint NumberReturned, out uint TotalMatches, out uint UpdateID);

        void Search(string ContainerID, string SearchCriteria, string Filter, uint StartingIndex, uint RequestedCount, string SortCriteria, out string Result, out uint NumberReturned, out uint TotalMatches, out uint UpdateID);

        void CreateObject(string ContainerID, string Elements, out string ObjectID, out string Result);

        void DestroyObject(string ObjectID);
    }

    [InterfaceType(ComInterfaceType.InterfaceIsDual)]
    public interface IAVTransport
    {
        string A_ARG_TYPE_SeekMode
        {
            get;
            set;
        }

        string NextAVTransportURI
        {
            get;
            set;
        }

        string PlaybackStorageMedium
        {
            get;
            set;
        }

        string RecordStorageMedium
        {
            get;
            set;
        }

        uint A_ARG_TYPE_InstanceID
        {
            get;
            set;
        }

        uint CurrentTrack
        {
            get;
            set;
        }

        string TransportState
        {
            get;
            set;
        }

        string PossiblePlaybackStorageMedia
        {
            get;
            set;
        }

        string CurrentPlayMode
        {
            get;
            set;
        }

        string TransportStatus
        {
            get;
            set;
        }

        string CurrentRecordQualityMode
        {
            get;
            set;
        }

        string CurrentTrackDuration
        {
            get;
            set;
        }

        string CurrentTrackURI
        {
            get;
            set;
        }

        string AVTransportURI
        {
            get;
            set;
        }

        int AbsoluteCounterPosition
        {
            get;
            set;
        }

        string PossibleRecordQualityModes
        {
            get;
            set;
        }

        string NextAVTransportURIMetaData
        {
            get;
            set;
        }

        string AVTransportURIMetaData
        {
            get;
            set;
        }

        string A_ARG_TYPE_SeekTarget
        {
            get;
            set;
        }

        string AbsoluteTimePosition
        {
            get;
            set;
        }

        string RecordMediumWriteStatus
        {
            get;
            set;
        }

        string CurrentTransportActions
        {
            get;
            set;
        }

        string RelativeTimePosition
        {
            get;
            set;
        }

        string CurrentTrackMetaData
        {
            get;
            set;
        }

        string CurrentMediaDuration
        {
            get;
            set;
        }

        string TransportPlaySpeed
        {
            get;
            set;
        }

        int RelativeCounterPosition
        {
            get;
            set;
        }

        uint NumberOfTracks
        {
            get;
            set;
        }

        string PossibleRecordStorageMedia
        {
            get;
            set;
        }

        string LastChange
        {
            get;
            set;
        }

        void GetPositionInfo(uint InstanceID, out uint Track, out string TrackDuration, out string TrackMetaData, out string TrackURI, out string RelTime, out string AbsTime, out int RelCount, out int AbsCount);

        void GetTransportInfo(uint InstanceID, out string CurrentTransportState, out string CurrentTransportStatus, out string CurrentSpeed);

        void GetTransportSettings(uint InstanceID, out string PlayMode, out string RecQualityMode);

        void GetCurrentTransportActions(uint InstanceID, out string Actions);

        void GetDeviceCapabilities(uint InstanceID, out string PlayMedia, out string RecMedia, out string RecQualityModes);

        void GetMediaInfo(uint InstanceID, out uint NrTracks, out string MediaDuration, out string CurrentURI, out string CurrentURIMetaData, out string NextURI, out string PlayMedium, out string RecordMedium, out string WriteStatus);

        void SetAVTransportURI(uint InstanceID, string CurrentURI, string CurrentURIMetaData);

        void SetPlayMode(uint InstanceID, string NewPlayMode);

        void Stop(uint InstanceID);

        void Pause(uint InstanceID);

        void Play(uint InstanceID, string Speed);

        void Previous(uint InstanceID);

        void Next(uint InstanceID);

        void Seek(uint InstanceID, string Unit, string Target);
    }

    [InterfaceType(ComInterfaceType.InterfaceIsDual)]
    public interface IRenderingControl
    {
        string A_ARG_TYPE_Channel
        {
            get;
            set;
        }

        uint A_ARG_TYPE_InstanceID
        {
            get;
            set;
        }

        bool Mute
        {
            get;
            set;
        }

        UInt16 ColorTemperature
        {
            get;
            set;
        }

        UInt16 Sharpness
        {
            get;
            set;
        }

        string A_ARG_TYPE_PresetName
        {
            get;
            set;
        }

        UInt16 BlueVideoGain
        {
            get;
            set;
        }

        UInt16 RedVideoBlackLevel
        {
            get;
            set;
        }

        Int16 HorizontalKeystone
        {
            get;
            set;
        }

        UInt16 Contrast
        {
            get;
            set;
        }

        UInt16 Brightness
        {
            get;
            set;
        }

        UInt16 Volume
        {
            get;
            set;
        }

        string PresetNameList
        {
            get;
            set;
        }

        UInt16 RedVideoGain
        {
            get;
            set;
        }

        UInt16 BlueVideoBlackLevel
        {
            get;
            set;
        }

        bool Loudness
        {
            get;
            set;
        }

        Int16 VerticalKeystone
        {
            get;
            set;
        }

        UInt16 GreenVideoBlackLevel
        {
            get;
            set;
        }

        UInt16 GreenVideoGain
        {
            get;
            set;
        }

        string LastChange
        {
            get;
            set;
        }

        void GetVolume(uint InstanceID, string Channel, out UInt16 CurrentVolume);

        void GetMute(uint InstanceID, string Channel, out bool CurrentMute);

        void GetRedVideoBlackLevel(uint InstanceID, out UInt16 CurrentRedVideoBlackLevel);

        void GetRedVideoGain(uint InstanceID, out UInt16 CurrentRedVideoGain);

        void GetGreenVideoBlackLevel(uint InstanceID, out UInt16 CurrentGreenVideoBlackLevel);

        void GetGreenVideoGain(uint InstanceID, out UInt16 CurrentGreenVideoGain);

        void GetBlueVideoBlackLevel(uint InstanceID, out UInt16 CurrentBlueVideoBlackLevel);

        void GetBlueVideoGain(uint InstanceID, out UInt16 CurrentBlueVideoGain);

        void GetBrightness(uint InstanceID, out UInt16 CurrentBrightness);

        void GetColorTemperature(uint InstanceID, out UInt16 CurrentColorTemperature);

        void GetContrast(uint InstanceID, out UInt16 CurrentContrast);

        void GetHorizontalKeystone(uint InstanceID, out Int16 CurrentHorizontalKeystone);

        void GetLoudness(uint InstanceID, string Channel, out bool CurrentLoudness);

        void GetSharpness(uint InstanceID, out UInt16 CurrentSharpness);

        void GetVerticalKeystone(uint InstanceID, out Int16 CurrentVerticalKeystone);

        void ListPresets(uint InstanceID, out string CurrentPresetNameList);

        void SelectPreset(uint InstanceID, string PresetName);

        void SetRedVideoBlackLevel(uint InstanceID, UInt16 DesiredRedVideoBlackLevel);

        void SetRedVideoGain(uint InstanceID, UInt16 DesiredRedVideoGain);

        void SetGreenVideoBlackLevel(uint InstanceID, UInt16 DesiredGreenVideoBlackLevel);

        void SetGreenVideoGain(uint InstanceID, UInt16 DesiredGreenVideoGain);

        void SetBlueVideoBlackLevel(uint InstanceID, UInt16 DesiredBlueVideoBlackLevel);

        void SetBlueVideoGain(uint InstanceID, UInt16 DesiredBlueVideoGain);

        void SetBrightness(uint InstanceID, UInt16 DesiredBrightness);

        void SetColorTemperature(uint InstanceID, UInt16 DesiredColorTemperature);

        void SetContrast(uint InstanceID, UInt16 DesiredContrast);

        void SetHorizontalKeystone(uint InstanceID, Int16 DesiredHorizontalKeystone);

        void SetLoudness(uint InstanceID, string Channel, UInt16 DesiredLoudness);

        void SetVolume(uint InstanceID, string Channel, UInt16 DesiredVolume);

        void SetMute(uint InstanceID, string Channel, bool DesiredMute);

        void SetSharpness(uint InstanceID, UInt16 DesiredSharpness);

        void SetVerticalKeystone(uint InstanceID, Int16 DesiredVerticalKeystone);
    }

    //

    [ComVisible(true), ClassInterface(ClassInterfaceType.AutoDispatch)]
    public class EventSource : IUPnPEventSource
    {
        protected IUPnPEventSink _host;

        virtual public void Advise(IUPnPEventSink pesSubscriber)
        {
            _host = (IUPnPEventSink)pesSubscriber;
        }

        virtual public void Unadvise(IUPnPEventSink pesSubscriber)
        {
            _host = null;
        }

        protected void StateChanged(int dispid)
        {
            _host.OnStateChangedSafe(new int[] { dispid });
        }
    }

    [ComVisible(true)]
    public class ConnectionManager : EventSource, IConnectionManager
    {
        public string A_ARG_TYPE_ConnectionStatus
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public int A_ARG_TYPE_ConnectionID
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_Direction
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public int A_ARG_TYPE_RcsID
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_ProtocolInfo
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_ConnectionManager
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public int A_ARG_TYPE_AVTransportID
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        [DispId(1)]
        virtual public string SourceProtocolInfo
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        [DispId(2)]
        virtual public string SinkProtocolInfo
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        [DispId(3)]
        virtual public string CurrentConnectionIDs
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        virtual public void GetCurrentConnectionIDs(out string ConnectionIDs)
        {
            throw new NotImplementedException();
        }

        virtual public void GetCurrentConnectionInfo(int ConnectionID, out int RcsID, out int AVTransportID, out string ProtocolInfo, out string PeerConnectionManager, out int PeerConnectionID, out string Direction, out string Status)
        {
            throw new NotImplementedException();
        }

        virtual public void GetProtocolInfo(out string Source, out string Sink)
        {
            throw new NotImplementedException();
        }
    }

    [ComVisible(true)]
    public class ContentDirectory : IContentDirectory
    {
        public string A_ARG_TYPE_BrowseFlag
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public uint A_ARG_TYPE_Count
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_Filter
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public uint A_ARG_TYPE_Index
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_ObjectID
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_Result
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_SearchCriteria
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_SortCriteria
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_TagValueList
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public uint A_ARG_TYPE_TransferID
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_TransferLength
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_TransferStatus
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_TransferTotal
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public uint A_ARG_TYPE_UpdateID
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public Uri A_ARG_TYPE_URI
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string SearchCapabilities
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string SortCapabilities
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        [DispId(1)]
        virtual public string ContainerUpdateIDs
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        [DispId(2)]
        virtual public uint SystemUpdateID
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        [DispId(3)]
        virtual public string TransferIDs
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        virtual public void GetSearchCapabilities(out string SearchCaps)
        {
            throw new NotImplementedException();
        }

        virtual public void GetSortCapabilities(out string SortCaps)
        {
            throw new NotImplementedException();
        }

        virtual public void GetSystemUpdateID(out uint Id)
        {
            throw new NotImplementedException();
        }

        virtual public void Browse(string ObjectID, string BrowseFlag, string Filter, uint StartingIndex, uint RequestedCount, string SortCriteria, out string Result, out uint NumberReturned, out uint TotalMatches, out uint UpdateID)
        {
            throw new NotImplementedException();
        }

        virtual public void Search(string ContainerID, string SearchCriteria, string Filter, uint StartingIndex, uint RequestedCount, string SortCriteria, out string Result, out uint NumberReturned, out uint TotalMatches, out uint UpdateID)
        {
            throw new NotImplementedException();
        }

        virtual public void CreateObject(string ContainerID, string Elements, out string ObjectID, out string Result)
        {
            throw new NotImplementedException();
        }

        virtual public void DestroyObject(string ObjectID)
        {
            throw new NotImplementedException();
        }
    }

    [ComVisible(true)]
    public class AVTransport : EventSource, IAVTransport
    {
        public string A_ARG_TYPE_SeekMode
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string NextAVTransportURI
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string PlaybackStorageMedium
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string RecordStorageMedium
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public uint A_ARG_TYPE_InstanceID
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public uint CurrentTrack
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string TransportState
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string PossiblePlaybackStorageMedia
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string CurrentPlayMode
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string TransportStatus
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string CurrentRecordQualityMode
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string CurrentTrackDuration
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string CurrentTrackURI
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string AVTransportURI
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public int AbsoluteCounterPosition
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string PossibleRecordQualityModes
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string NextAVTransportURIMetaData
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string AVTransportURIMetaData
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_SeekTarget
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string AbsoluteTimePosition
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string RecordMediumWriteStatus
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string CurrentTransportActions
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string RelativeTimePosition
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string CurrentTrackMetaData
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string CurrentMediaDuration
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string TransportPlaySpeed
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public int RelativeCounterPosition
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public uint NumberOfTracks
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string PossibleRecordStorageMedia
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        [DispId(1)]
        virtual public string LastChange
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        virtual public void GetPositionInfo(uint InstanceID, out uint Track, out string TrackDuration, out string TrackMetaData, out string TrackURI, out string RelTime, out string AbsTime, out int RelCount, out int AbsCount)
        {
            throw new NotImplementedException();
        }

        virtual public void GetTransportInfo(uint InstanceID, out string CurrentTransportState, out string CurrentTransportStatus, out string CurrentSpeed)
        {
            throw new NotImplementedException();
        }

        virtual public void GetTransportSettings(uint InstanceID, out string PlayMode, out string RecQualityMode)
        {
            throw new NotImplementedException();
        }

        virtual public void GetCurrentTransportActions(uint InstanceID, out string Actions)
        {
            throw new NotImplementedException();
        }

        virtual public void GetDeviceCapabilities(uint InstanceID, out string PlayMedia, out string RecMedia, out string RecQualityModes)
        {
            throw new NotImplementedException();
        }

        virtual public void GetMediaInfo(uint InstanceID, out uint NrTracks, out string MediaDuration, out string CurrentURI, out string CurrentURIMetaData, out string NextURI, out string PlayMedium, out string RecordMedium, out string WriteStatus)
        {
            throw new NotImplementedException();
        }

        virtual public void SetAVTransportURI(uint InstanceID, string CurrentURI, string CurrentURIMetaData)
        {
            throw new NotImplementedException();
        }

        virtual public void SetPlayMode(uint InstanceID, string NewPlayMode)
        {
            throw new NotImplementedException();
        }

        virtual public void Stop(uint InstanceID)
        {
            throw new NotImplementedException();
        }

        virtual public void Pause(uint InstanceID)
        {
            throw new NotImplementedException();
        }

        virtual public void Play(uint InstanceID, string Speed)
        {
            throw new NotImplementedException();
        }

        virtual public void Previous(uint InstanceID)
        {
            throw new NotImplementedException();
        }

        virtual public void Next(uint InstanceID)
        {
            throw new NotImplementedException();
        }

        virtual public void Seek(uint InstanceID, string Unit, string Target)
        {
            throw new NotImplementedException();
        }
    }

    [ComVisible(true)]
    public class RenderingControl : EventSource, IRenderingControl
    {
        public string A_ARG_TYPE_Channel
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public uint A_ARG_TYPE_InstanceID
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public bool Mute
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort ColorTemperature
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort Sharpness
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string A_ARG_TYPE_PresetName
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort BlueVideoGain
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort RedVideoBlackLevel
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public short HorizontalKeystone
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort Contrast
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort Brightness
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort Volume
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public string PresetNameList
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort RedVideoGain
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort BlueVideoBlackLevel
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public bool Loudness
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public short VerticalKeystone
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort GreenVideoBlackLevel
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        public ushort GreenVideoGain
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        virtual public string LastChange
        {
            get
            {
                throw new NotImplementedException();
            }
            set
            {
                throw new NotImplementedException();
            }
        }

        virtual public void GetVolume(uint InstanceID, string Channel, out ushort CurrentVolume)
        {
            throw new NotImplementedException();
        }

        virtual public void GetMute(uint InstanceID, string Channel, out bool CurrentMute)
        {
            throw new NotImplementedException();
        }

        virtual public void GetRedVideoBlackLevel(uint InstanceID, out ushort CurrentRedVideoBlackLevel)
        {
            throw new NotImplementedException();
        }

        virtual public void GetRedVideoGain(uint InstanceID, out ushort CurrentRedVideoGain)
        {
            throw new NotImplementedException();
        }

        virtual public void GetGreenVideoBlackLevel(uint InstanceID, out ushort CurrentGreenVideoBlackLevel)
        {
            throw new NotImplementedException();
        }

        virtual public void GetGreenVideoGain(uint InstanceID, out ushort CurrentGreenVideoGain)
        {
            throw new NotImplementedException();
        }

        virtual public void GetBlueVideoBlackLevel(uint InstanceID, out ushort CurrentBlueVideoBlackLevel)
        {
            throw new NotImplementedException();
        }

        virtual public void GetBlueVideoGain(uint InstanceID, out ushort CurrentBlueVideoGain)
        {
            throw new NotImplementedException();
        }

        virtual public void GetBrightness(uint InstanceID, out ushort CurrentBrightness)
        {
            throw new NotImplementedException();
        }

        virtual public void GetColorTemperature(uint InstanceID, out ushort CurrentColorTemperature)
        {
            throw new NotImplementedException();
        }

        virtual public void GetContrast(uint InstanceID, out ushort CurrentContrast)
        {
            throw new NotImplementedException();
        }

        virtual public void GetHorizontalKeystone(uint InstanceID, out short CurrentHorizontalKeystone)
        {
            throw new NotImplementedException();
        }

        virtual public void GetLoudness(uint InstanceID, string Channel, out bool CurrentLoudness)
        {
            throw new NotImplementedException();
        }

        virtual public void GetSharpness(uint InstanceID, out ushort CurrentSharpness)
        {
            throw new NotImplementedException();
        }

        virtual public void GetVerticalKeystone(uint InstanceID, out short CurrentVerticalKeystone)
        {
            throw new NotImplementedException();
        }

        virtual public void ListPresets(uint InstanceID, out string CurrentPresetNameList)
        {
            throw new NotImplementedException();
        }

        virtual public void SelectPreset(uint InstanceID, string PresetName)
        {
            throw new NotImplementedException();
        }

        virtual public void SetVolume(uint InstanceID, string Channel, ushort DesiredVolume)
        {
            throw new NotImplementedException();
        }

        virtual public void SetMute(uint InstanceID, string Channel, bool DesiredMute)
        {
            throw new NotImplementedException();
        }

        virtual public void SetRedVideoBlackLevel(uint InstanceID, ushort DesiredRedVideoBlackLevel)
        {
            throw new NotImplementedException();
        }

        virtual public void SetRedVideoGain(uint InstanceID, ushort DesiredRedVideoGain)
        {
            throw new NotImplementedException();
        }

        virtual public void SetGreenVideoBlackLevel(uint InstanceID, ushort DesiredGreenVideoBlackLevel)
        {
            throw new NotImplementedException();
        }

        virtual public void SetGreenVideoGain(uint InstanceID, ushort DesiredGreenVideoGain)
        {
            throw new NotImplementedException();
        }

        virtual public void SetBlueVideoBlackLevel(uint InstanceID, ushort DesiredBlueVideoBlackLevel)
        {
            throw new NotImplementedException();
        }

        virtual public void SetBlueVideoGain(uint InstanceID, ushort DesiredBlueVideoGain)
        {
            throw new NotImplementedException();
        }

        virtual public void SetBrightness(uint InstanceID, ushort DesiredBrightness)
        {
            throw new NotImplementedException();
        }

        virtual public void SetColorTemperature(uint InstanceID, ushort DesiredColorTemperature)
        {
            throw new NotImplementedException();
        }

        virtual public void SetContrast(uint InstanceID, ushort DesiredContrast)
        {
            throw new NotImplementedException();
        }

        virtual public void SetHorizontalKeystone(uint InstanceID, short DesiredHorizontalKeystone)
        {
            throw new NotImplementedException();
        }

        virtual public void SetLoudness(uint InstanceID, string Channel, ushort DesiredLoudness)
        {
            throw new NotImplementedException();
        }

        virtual public void SetSharpness(uint InstanceID, ushort DesiredSharpness)
        {
            throw new NotImplementedException();
        }

        virtual public void SetVerticalKeystone(uint InstanceID, short DesiredVerticalKeystone)
        {
            throw new NotImplementedException();
        }
    }
    /*
    public class MediaServer : UPnPDeviceControl
    {
        public MediaServer()
        {
            string dir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);

            Register(dir + @"\UPnP\MediaServer.xml", true);
        }
    }
    */
}
