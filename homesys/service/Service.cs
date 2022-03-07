using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.ServiceModel;
using System.ServiceModel.Description;
using System.Net;
using System.Runtime.Serialization;
using System.Data;
using System.Data.SqlServerCe;
using System.Threading;
using System.Runtime.InteropServices;
using System.Xml;
using System.Xml.XPath;
using System.Linq;
using System.Transactions;
using System.Xml.Serialization;
using System.Security.Cryptography;
using System.Data.Linq.SqlClient;
using System.Web;
using System.Runtime.Serialization.Formatters.Binary;
using System.Diagnostics;
using System.ServiceModel.Discovery;
using System.ServiceModel.Web;

namespace Homesys
{
    [ServiceBehavior(InstanceContextMode = InstanceContextMode.Single)]
    public partial class Service : IDisposable, Local.IService
    {
        class Timers
        {
            public Timer online;
            public Timer recording;
            public Timer webupdate;
            public Timer appversion;
            public Timer diskspace;
            public WaitableTimer wakeup;

            public const long OnlineTimeout = 15000;
            public const long RecordingTimeout = 5000;
            public const long WebUpdateTimeout = 60000;
            public const long AppVersionTimeout = 3600000;
            public const long DiskSpaceTimeout = 30000;
        }

        class Cache
        {
            public Dictionary<int, Local.Channel> channel = new Dictionary<int, Local.Channel>();
            public Dictionary<int, Local.Program> program = new Dictionary<int, Local.Program>();
            public Dictionary<int, Local.Episode> episode = new Dictionary<int, Local.Episode>();
            public Dictionary<int, Local.Movie> movie = new Dictionary<int, Local.Movie>();

            public void Clear()
            {
                channel.Clear();
                program.Clear();
                episode.Clear();
                movie.Clear();
            }
        }

        class TunerForRecording
        {
            public int index;
            public Guid tunerId;
            public int presetId;
            public Local.RecordingTimeState state;
            public bool activeAndFree;
        }

        class ScanContext
        {
            public Guid tunerId;
            public Tuner tuner;
            public Thread thread;
            public List<Local.TuneRequest> reqs;
            public List<Local.Preset> presets = new List<Local.Preset>();
            public string provider = "";
            public bool abort;
            public bool done = true;
        }

        Object _sync;
        DB _db;
        Cache _cache = new Cache();
        Dictionary<int, Local.Preset> _presets = new Dictionary<int, Local.Preset>();
        Dictionary<Guid, Tuner> _tuners;
        Guid _tunerId;
        SmartCardServer _cardserver;
        Local.AppVersion _appversion;
        StringLocalizer _localizer;
        FileSystemWatcher _recordingPathWatcher;
        Timers _timer;
        Guide _guide;
        ScanContext _scan;
        volatile bool _online_server = true;
        volatile bool _online_user = false;
        ServiceHost _callback;
        MediaServer _mediaserver;

        static int PhysConn_Video_Tuner = 1;
        static int AnalogVideo_PAL_B = 0x10;

        // TODO
        public const int Start_delay = -300;
        public const int Stop_delay = 900;

        public Service()
        {
            _sync = new Object();
            _db = new DB(AppDataPath + "homesys.sdf");
            _tuners = new Dictionary<Guid, Tuner>();
            _tunerId = Guid.Empty;
            _appversion = new Local.AppVersion();
            _localizer = new StringLocalizer();
            _scan = new ScanContext();
            _callback = CallbackService.Create();
            _callback.Open();
            _mediaserver = new MediaServer();

            RepairTuners();

            _guide = new Guide(this);
            _guide.UpdatedEvent += OnGuideUpdated;

            _timer = new Timers();
            _timer.wakeup = new WaitableTimer();
            _timer.online = new Timer(TimerOnline, null, 15000, Timers.OnlineTimeout);
            _timer.recording = new Timer(TimerRecording, null, 10000, Timers.RecordingTimeout);
            _timer.webupdate = new Timer(TimerWebUpdate, null, 5000, Timers.WebUpdateTimeout);
            _timer.appversion = new Timer(TimerAppVersion, null, 1000, Timers.AppVersionTimeout);
            _timer.diskspace = new Timer(TimerDiskSpace, null, 10000, Timers.DiskSpaceTimeout);

            UpdateRecordingPathWatcher();

            using(var Context = CreateDataContext())
            {
                if(Context.Machine.Count() == 0)
                {
                    Context.Machine.InsertOnSubmit(new Data.Machine()
                    {
                        Id = Guid.NewGuid(),
                        Name = Environment.MachineName,
                        Published = false,
                        Lang = _localizer.Language,
                        Timeshift_dur = 100
                    });
                }

                foreach(Guid id in (from row in Context.Recording where row.Name == "" select row.Id).ToList())
                {
                    UpdateRecordingState(id);
                }

                foreach(Data.Parental parental in (from row in Context.Parental where row.Inactivity > 0 && row.Password != "" select row))
                {
                    parental.Enabled = true;
                }

                Context.SubmitChanges();
            }

            if(_guide.UpdateChannels())
            {
                _guide.UpdateActivePrograms();
            }
        }

        public StringLocalizer Localizer
        {
            get { return _localizer; }
        }

        public static string AppDataPath
        {
            get
            {
                string path = Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData) + Path.DirectorySeparatorChar + "Homesys";

                if(!Directory.Exists(path))
                {
                    Directory.CreateDirectory(path);
                }

                return path + Path.DirectorySeparatorChar;
            }
        }

        public static string DefaultRecordingPath
        {
            get
            {
                string path = AppDataPath + "Recordings";

                if(!Directory.Exists(path))
                {
                    Directory.CreateDirectory(path);
                }

                return path + Path.DirectorySeparatorChar;
            }
        }

        public static string DefaultRecordingFormat
        {
            get
            {
                return ".ts";
            }
        }

        public static string DefaultTimeshiftPath
        {
            get
            {
                return Path.GetTempPath();
            }
        }

        public static string DefaultDownloadPath
        {
            get
            {
                string path = AppDataPath + "Downloads";

                if(!Directory.Exists(path))
                {
                    Directory.CreateDirectory(path);
                }

                return path + Path.DirectorySeparatorChar;
            }
        }

        public static DateTime DateTimeMin
        {
            get { return new DateTime(1980, 1, 1); }
        }

        public bool Run(String[] args)
        {
            if(args.Length >= 2 && args[1] == "enumtuners")
            {
                foreach(Local.TunerDevice td in AllTuners)
                {
                    Log.WriteLine("{0} {1}", td.name, td.type);
                }

                return false;
            }
            else if(args.Length >= 4 && args[1] == "scan")
            {
                foreach(Local.TunerReg t in Tuners)
                {
                    if(args[2] == "analog" && t.type == Local.TunerDeviceType.Analog
                    || args[2] == "dvbs" && t.type == Local.TunerDeviceType.DVBS
                    || args[2] == "dvbt" && t.type == Local.TunerDeviceType.DVBT
                    || args[2] == "dvbc" && t.type == Local.TunerDeviceType.DVBC
                    || args[2] == "dvbf" && t.type == Local.TunerDeviceType.DVBF)
                    {
                        Log.WriteLine("{0}", t.name);

                        foreach(Local.TuneRequestPackage package in GetTuneRequestPackages(t.id))
                        {
                            if(package.name == args[3])
                            {
                                StartTunerScanByPackage(t.id, package.id);

                                while(!IsTunerScanDone())
                                {
                                    Thread.Sleep(100);

                                    if(Console.KeyAvailable)
                                    {
                                        break;
                                    }
                                }

                                StopTunerScan();

                                List<Local.Preset> presets = new List<Local.Preset>(GetTunerScanResult());

                                if(presets != null && presets.Count > 0)
                                {
                                    SaveTunerScanResult(t.id, presets, false);
                                }

                                break;
                            }
                        }

                        break;
                    }
                }

                return false;
            }

            return true;
        }

        public static ServiceHost Create()
        {
            // ModifyDefaultDacl((HANDLE)System.Diagnostics.Process.GetCurrentProcess().Handle);

            Service svc = new Service();

            string host = Dns.GetHostName();

            ServiceHost serviceHost = new ServiceHost(svc, new Uri("http://" + host + ":24935/"));

            // #if DEBUG

            ServiceMetadataBehavior metadataBehavior = serviceHost.Description.Behaviors.Find<ServiceMetadataBehavior>();

            if(metadataBehavior != null)
			{
                metadataBehavior.HttpGetEnabled = true;
			}
			else
			{
                metadataBehavior = new ServiceMetadataBehavior();
                metadataBehavior.HttpGetEnabled = true;
                serviceHost.Description.Behaviors.Add(metadataBehavior);
			}

            // #endif

            ServiceDebugBehavior debugBehavior = serviceHost.Description.Behaviors.Find<ServiceDebugBehavior>();

            if(debugBehavior != null)
            {
                debugBehavior.IncludeExceptionDetailInFaults = true;
            }
            else
            {
                debugBehavior = new ServiceDebugBehavior();
                debugBehavior.IncludeExceptionDetailInFaults = true;
                serviceHost.Description.Behaviors.Add(debugBehavior);
            }

            serviceHost.Description.Behaviors.Add(new ServiceDiscoveryBehavior());

            NetNamedPipeBinding netNamedPipeBinding = new NetNamedPipeBinding();

            serviceHost.AddServiceEndpoint(typeof(Local.IService), netNamedPipeBinding, "net.pipe://localhost/homesys/service");

            BasicHttpBinding basicHttpBinding = new BasicHttpBinding();

            int i = 0;

            foreach(IPAddress ipaddress in Dns.GetHostEntry(host).AddressList.Where(ip => ip.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork))
            {
                serviceHost.AddServiceEndpoint(typeof(Local.IService), basicHttpBinding, String.Format("http://{0}:{1}/homesys{2}", ipaddress, 24935, i++));
            }

            serviceHost.AddServiceEndpoint(typeof(Local.IService), basicHttpBinding, String.Format("http://{0}:{1}/homesys{2}", host, 24935, i++));

            serviceHost.AddServiceEndpoint(new UdpDiscoveryEndpoint());

            return serviceHost;
        }

        public Data.HomesysDB CreateDataContext(bool tracking = true)
        {
            var ctx = new Data.HomesysDB(_db.Connection);

            if(!tracking)
            {
                ctx.ObjectTrackingEnabled = false;
            }

            return ctx;
        }

        public Web.HomesysService CreateWebService(bool auth = false)
        {
            try
            {
                if(!_online_server)
                {
                    throw new Exception("WebService offline");
                }

                Web.HomesysService svc = null;

                Local.MachineReg m = Machine;

                if(auth && m == null)
                {
                    throw new Exception("Machine not registered");
                }

                const int timeout = 15000;

                if(m != null && m.username.Length > 0)
                {
                    svc = new Web.HomesysService(timeout, m.username, m.password, _localizer.Language);
                }
                else
                {
                    svc = new Web.HomesysService(timeout);
                }

                if(auth && svc.SessionId == "")
                {
                    throw new Exception("Could not start session");
                }

                if(svc.SessionId != "")
                {
                    lock(_sync)
                    {
                        if(m != null && !m.published)
                        {
                            Publish(svc, m.name);
                        }
                    }

                    _online_user = true;
                }

                return svc;
            }
            catch(Exception e)
            {
                _online_user = false;

                if(e is WebException)
                {
                    if(((WebException)e).Status == WebExceptionStatus.Timeout)
                    {
                        Console.WriteLine(e.Message);

                        _online_server = false;
                    }
                }

                throw;
            }
        }

        void Publish(Web.HomesysService svc, string name)
        {
            try
            {
                List<Web.TunerData> tdl = new List<Web.TunerData>();

                int i = 0;

                using(var Context = CreateDataContext(false))
                {
                    var webIds = (from row in svc.ListMachines(svc.SessionId) select row.Id).ToList();

                    /*
                    if(Context.Machine.Any(m => m.Web_id.HasValue && webIds.Contains(m.Web_id.Value)))
                    {
                        return; // already published
                    }
                    */

                    Log.WriteLine("Registering machine");

                    foreach(Guid webId in webIds)
                    {
                        svc.UnregisterMachine(svc.SessionId, webId);
                    }

                    foreach(var r in Context.Tuner)
                    {
                        Web.TunerData td = new Web.TunerData();

                        td.SequenceNumber = i++;
                        td.ClientTunerData = r.Id.ToString();
                        td.FriendlyName = r.Friendly_name;
                        td.DisplayName = r.Display_name;

                        tdl.Add(td);
                    }
                }

                Web.MachineRegistrationData mrd = svc.RegisterMachine(svc.SessionId, name, tdl.ToArray());

                using(var Context = CreateDataContext())
                {
                    var r = Context.Machine.First();

                    r.Web_id = mrd.MachineId;
                    r.Published = true;

                    foreach(Web.Tuner t in mrd.TunerList)
                    {
                        var q = from row in tdl where row.SequenceNumber == t.SequenceNumber select row.ClientTunerData;

                        Guid tunerId = new Guid(q.Single());

                        Context.Tuner.Single(row => row.Id == tunerId).Web_id = t.Id;
                    }

                    Context.SubmitChanges();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        void Unpublish()
        {
            using(var Context = CreateDataContext())
            {
                var m = Context.Machine.First();

                m.Web_id = null;
                m.Published = false;

                foreach(var t in Context.Tuner)
                {
                    t.Web_id = null;
                }

                foreach(var r in Context.Recording)
                {
                    r.Web_id = 0;
                }

                Context.SubmitChanges();
            }

            GoOnline();
        }

        public bool OnQuerySuspend()
		{
            lock(_sync)
            {
                try
                {
                    if(_tuners.Values.Any(t => t.IsRecording()))
                    {
                        return false;
                    }
                }
                catch(Exception e)
                {
                    Log.WriteLine(e);
                }
            }

			return true;
		}

        public void OnSuspend()
		{
			DeactivateAll();
		}

        public void OnResumeSuspend()
		{
			// TODO
		}

        void TimerOnline(Object o)
        {
            bool taken = false;

            try
            {
                Monitor.TryEnter(_timer.online, ref taken);

                if(!taken)
                {
                    return;
                }

                if(!_online_server)
                {
                    Web.HomesysService svc = new Web.HomesysService(3000);

                    svc.GetNow(); // TODO: Ping()

                    _online_server = true;

                    Console.WriteLine("WebService online");
                }
            }
            catch(Exception e)
            {
                // Log.WriteLine(e);
            }
            finally
            {
                if(taken)
                {
                    Monitor.Exit(_timer.online);
                }
            }
        }

		void TimerRecording(Object o)
		{
            bool taken = false;

            try
            {
                Monitor.TryEnter(_timer.recording, ref taken);

                if(!taken)
                {
                    return;
                }

                if(_timer.wakeup != null)
                {
                    UpdateScheduled();

                    UpdateInProgress();

                    UpdateIdle();

                    DateTime? start = GetNextRecordingStart();

                    if(start != null)
                    {
                        _timer.wakeup.Set(start.Value - new TimeSpan(0, 1, 0));
                    }
                    else
                    {
                        _timer.wakeup.Reset();
                    }
                }
                else
                {
                    Debug.WriteLine("Wakeup timer gone");
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
            finally
            {
                if(taken)
                {
                    Monitor.Exit(_timer.recording);
                }
            }
		}

        void UpdateScheduled()
        {
            lock(_sync)
            {
                foreach(Local.Recording r in GetRecordings(Local.RecordingState.Scheduled))
                {
                    Local.RecordingTimeState state = r.GetTimeState();

                    if(state == Local.RecordingTimeState.Before)
                    {
                        continue;
                    }

                    if(state == Local.RecordingTimeState.StopDelay || state == Local.RecordingTimeState.After)
                    {
                        UpdateRecording(r.id, Local.RecordingState.Missed);

                        continue;
                    }

                    bool started = false;
                    bool delayed = false;

                    try
                    {
                        int channelId = r.program != null ? r.program.channelId : 0;

                        int presetId = r.preset != null ? r.preset.id : 0;

                        List<TunerForRecording> tps = EnumTunersForRecording(channelId, presetId);

                        if(tps == null)
                        {
                            throw new Exception("Cannot record, no suitable tuner found");
                        }

                        foreach(TunerForRecording tp in tps)
                        {
                            try
                            {
                                if(tp.state != Local.RecordingTimeState.None)
                                {
                                    if(tp.state >= Local.RecordingTimeState.StopDelay && state >= Local.RecordingTimeState.Record)
                                    {
                                        Log.WriteLine("Stopped recording ({0}, {1} - {2}, {3})", r.id, tp.state, state, tp.tunerId);

                                        StopRecording(tp.tunerId, Local.RecordingState.Finished);
                                    }
                                    else
                                    {
                                        Log.WriteLine("Recording delayed ({0} - {1}), tuner still recording ({2})", r.start, r.name, tp.tunerId);

                                        delayed = true;

                                        continue;
                                    }
                                }

                                string fn = GetPreset(tp.presetId).name + " - " + r.name;

                                Activate(tp.tunerId);

                                SetPreset(tp.tunerId, tp.presetId);

                                Thread.Sleep(1000); // ???

                                Tuner t;

                                if(_tuners.TryGetValue(tp.tunerId, out t))
                                {
                                    t.StartRecording(RecordingPath, RecordingFormat, fn, r.id);

                                    UpdateRecording(r.id, Local.RecordingState.InProgress, fn);

                                    PostServiceMessage(_localizer[StringLocalizerType.RecordingStarted]);

                                    // TODO: store presetId to recording

                                    Log.WriteLine("Scheduled recording started ({0})", fn);

                                    started = true;

                                    break;
                                }
                                else
                                {
                                    throw new Exception("Invalid tunerId");
                                }
                            }
                            catch(Exception e)
                            {
                                Log.WriteLine(e);
                            }
                        }
                    }
                    catch(Exception e)
                    {
                        Log.WriteLine(e);
                    }

                    if(state == Local.RecordingTimeState.StartDelay)
                    {
                        continue; // do not mark it failed just yet
                    }

                    if(!started && !delayed)
                    {
                        UpdateRecording(r.id, Local.RecordingState.Error);
                    }
                }
            }
        }

        void UpdateInProgress()
        {
            lock(_sync)
            {
                foreach(Local.Recording r in GetRecordings(Local.RecordingState.InProgress))
                {
                    try
                    {
                        if(r.GetTimeState() == Local.RecordingTimeState.After)
                        {
                            Tuner t = _tuners.Values.FirstOrDefault(t2 => t2.GetRecordingId() == r.id);

                            if(t != null)
                            {
                                t.StopRecording();

                                UpdateRecording(r.id, Local.RecordingState.Finished, t.GetRecordingPath());

                                PostServiceMessage(_localizer[StringLocalizerType.RecordingEnded]);
                            }
                            else
                            {
                                UpdateRecording(r.id, Local.RecordingState.Error);
                            }
                        }
                    }
                    catch(Exception e)
                    {
                        Log.WriteLine(e);
                    }
                }
            }
        }

        void UpdateIdle()
        {
            lock(_sync)
            {
                List<Guid> inactive = new List<Guid>();

                foreach(KeyValuePair<Guid, Tuner> pair in _tuners.Where(p => p.Value.GetIdleTime() >= new TimeSpan(0, 1, 0)))
                {
                    Tuner t = pair.Value;

                    if(t.IsRecording())
                    {
                        Local.Recording r = GetRecording(t.GetRecordingId());

                        if(r != null && r.state != Local.RecordingState.Prompt)
                        {
                            continue;
                        }
                    }

                    inactive.Add(pair.Key);

                    Log.WriteLine("Idle tuner deactivated (tunerId = {0})", pair.Key);
                }

                foreach(Guid tunerId in inactive)
                {
                    Deactivate(tunerId);
                }

                if(_tuners.Count == 0 && _cardserver != null)
                {
                    _cardserver.Dispose();

                    _cardserver = null;

                    Log.WriteLine("Idle card server deactivated");
                }
            }
        }

        void TimerWebUpdate(object o)
        {
            bool taken = false;

            try
            {
                Monitor.TryEnter(_timer.webupdate, ref taken);

                if(!taken)
                {
                    return;
                }

                System.Threading.ThreadPriority oldThreadPriority = System.Threading.Thread.CurrentThread.Priority;
                System.Threading.Thread.CurrentThread.Priority = System.Threading.ThreadPriority.Lowest;

                try
                {
                    if(DateTime.Now - GetWebUpdate(WebUpdateType.Recordings) >= new TimeSpan(0, 5, 0))
                    {
                        List<Guid> deletedIds;

                        UpdateRecordings(out deletedIds);

                        foreach(Guid id in deletedIds)
                        {
                            try
                            {
                                lock(_sync)
                                {
                                    Tuner t = _tuners.Values.FirstOrDefault(t2 => t2.IsRecording() && t2.GetRecordingId() == id);

                                    if(t != null)
                                    {
                                        string fn = t.GetRecordingPath();

                                        t.StopRecording();

                                        File.Delete(fn);
                                    }
                                }
                            }
                            catch(Exception e)
                            {
                                Log.WriteLine(e);
                            }
                        }

                        SetWebUpdate(WebUpdateType.Recordings, DateTime.Now);
                    }
                }
                catch(Exception e)
                {
                    Log.WriteLine(e);
                }

                try
                {
                    if(DateTime.Now - GetWebUpdate(WebUpdateType.OverlappingRecordings) >= new TimeSpan(0, 3, 0))
                    {
                        UpdateOverlappingRecordings();

                        SetWebUpdate(WebUpdateType.OverlappingRecordings, DateTime.Now);
                    }
                }
                catch(Exception e)
                {
                    Log.WriteLine(e);
                }

                System.Threading.Thread.CurrentThread.Priority = oldThreadPriority;
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
            finally
            {
                if(taken)
                {
                    Monitor.Exit(_timer.webupdate);
                }
            }
        }

        void TimerAppVersion(Object o)
        {
            bool taken = false;

            try
            {
                Monitor.TryEnter(_timer.appversion, ref taken);

                if(!taken)
                {
                    return;
                }

                using(var svc = CreateWebService())
                {
                    string name = "homesys";

                    Web.AppVersion av = svc.GetAppVersion(name, System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString());

                    UInt64 version = 0;

                    lock(_sync)
                    {
                        _appversion.version = UInt64.Parse(av.Version, System.Globalization.NumberStyles.HexNumber);
                        _appversion.url = av.Url;
                        _appversion.required = av.Required;

                        version = _appversion.version;
                    }

                    if(InstalledVersion < version)
                    {
                        PostServiceMessage(_localizer[StringLocalizerType.NewVersionAvailable], Local.ServiceMessageType.SoftwareUpdate);
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
            finally
            {
                if(taken)
                {
                    Monitor.Exit(_timer.appversion);
                }
            }
        }
        
        [DllImport("coredll")]
        static extern bool GetDiskFreeSpaceEx(string directoryName, ref long freeBytesAvailable, ref long totalBytes, ref long totalFreeBytes);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool GetDiskFreeSpaceEx(string lpDirectoryName, out ulong lpFreeBytesAvailable, out ulong lpTotalNumberOfBytes, out ulong lpTotalNumberOfFreeBytes);

        void TimerDiskSpace(Object o)
        {
            bool taken = false;

            try
            {
                Monitor.TryEnter(_timer.diskspace, ref taken);

                if(!taken)
                {
                    return;
                }

                foreach(string s in new string[] { RecordingPath, TimeshiftPath })
                {
                    if(s != null)
                    {
                        ulong freeBytesAvailable;
                        ulong totalBytes;
                        ulong totalFreeBytes;

                        if(GetDiskFreeSpaceEx(s, out freeBytesAvailable, out totalBytes, out totalFreeBytes))
                        {
                            if(totalFreeBytes < (2ul << 30))
                            {
                                PostServiceMessage(Localizer[StringLocalizerType.DiskSpaceLow], Local.ServiceMessageType.DiskSpaceLow, new TimeSpan(0, 0, 10), 1, 0);

                                break;
                            }
                        }
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
            finally
            {
                if(taken)
                {
                    Monitor.Exit(_timer.diskspace);
                }
            }
        }

        void OnGuideUpdated()
        {
            lock(_cache)
            {
                _cache.Clear();
            }
        }

        void OnLoginChanged()
        {
            OnRecordingsChanged();
        }

        void OnRecordingsChanged()
        {
            lock(_sync)
            {
                ResetWebUpdate(WebUpdateType.Recordings);
             
                _timer.webupdate.Change(0, Timers.WebUpdateTimeout);
            }
        }

        void UpdateRecordings(out List<Guid> deletedIds)
        {
            deletedIds = new List<Guid>();

            try
            {
                using(var svc = CreateWebService(true))
                {
                    using(var Context = CreateDataContext())
                    {
                        Guid? webId = (from row in Context.Machine select row.Web_id).FirstOrDefault();

                        if(!webId.HasValue)
                        {
                            return; // no tuner, no recordings
                        }

                        // local => web

                        try
                        {
                            List<int> states = new List<int>()
                            {
                                (int)Local.RecordingState.Scheduled, 
                                (int)Local.RecordingState.InProgress, 
                                (int)Local.RecordingState.Aborted, 
                                (int)Local.RecordingState.Missed, 
                                (int)Local.RecordingState.Error, 
                                (int)Local.RecordingState.Finished, 
                                (int)Local.RecordingState.Overlapping
                            };

                            var recs =
                                from row in Context.Recording
                                where row.Program_id != null && row.Web_id == 0 && states.Contains(row.State)
                                select row;

                            foreach(var r in recs)
                            {
                                int programId = r.Program_id.Value;

                                Log.WriteLine("Sending recording (programId = {0})", programId);

                                r.Web_id = svc.AddRecording(svc.SessionId, webId.Value, programId);

                                Context.SubmitChanges();
                            }
                        }
                        catch(Exception e)
                        {
                            Log.WriteLine(e);

                            return;
                        }

                        // web => local

                        Dictionary<int, Web.Recording> webRecordings = new Dictionary<int, Web.Recording>();

                        try
                        {
                            int count = 0;

                            var recordings = svc.ListRecordings(svc.SessionId, false).Where(r => r.MachineId == webId.Value);

                            webRecordings = recordings.ToDictionary(r => r.Id);

                            HashSet<int> webIds = new HashSet<int>(from row in Context.Recording select row.Web_id);

                            foreach(Web.Recording recording in recordings.Where(r => !webIds.Contains(r.Id)))
                            {
                                Guid id = Guid.NewGuid();

                                Local.RecordingState state;

                                switch(recording.RecordingState)
                                {
                                    case Local.RecordingStateString.Scheduled:
                                        state = Local.RecordingState.Scheduled;
                                        break;
                                    case Local.RecordingStateString.InProgress:
                                        state = Local.RecordingState.InProgress;
                                        break;
                                    case Local.RecordingStateString.Aborted:
                                        state = Local.RecordingState.Aborted;
                                        break;
                                    case Local.RecordingStateString.Error:
                                        state = Local.RecordingState.Error;
                                        break;
                                    case Local.RecordingStateString.Finished:
                                        state = Local.RecordingState.Finished;
                                        break;
                                    case Local.RecordingStateString.Deleted:
                                        state = Local.RecordingState.Deleted;
                                        break;
                                    case Local.RecordingStateString.Warning:
                                        state = Local.RecordingState.Warning;
                                        break;
                                    default:
                                        state = Local.RecordingState.Scheduled;
                                        break;
                                }

                                Log.WriteLine("Received recording (channelId = {0}, programId = {1}, state = {2})", recording.ChannelId, recording.ProgramId, state);

                                Context.Recording.InsertOnSubmit(new Data.Recording()
                                {
                                    Id = id,
                                    Web_id = recording.Id,
                                    Name = "",
                                    Start = recording.StartDate,
                                    Stop = recording.StopDate,
                                    State = (int)state,
                                    Preset_id = null,
                                    Program_id = recording.ProgramId,
                                    Channel_id = recording.ChannelId,
                                    Start_delay = Service.Start_delay,
                                    Stop_delay = Service.Stop_delay
                                });

                                Context.SubmitChanges();

                                UpdateRecordingState(id);

                                count++;
                            }

                            if(count > 0)
                            {
                                PostServiceMessage(String.Format(_localizer[StringLocalizerType.IncomingRecording], count));
                            }
                        }
                        catch(Exception e)
                        {
                            Log.WriteLine(e);

                            return;
                        }

                        // sync recording state

                        try
                        {
                            Dictionary<Local.RecordingState, string> map = new Dictionary<Local.RecordingState, string>();

                            map.Add(Local.RecordingState.Scheduled, Local.RecordingStateString.Scheduled);
                            map.Add(Local.RecordingState.InProgress, Local.RecordingStateString.InProgress);
                            map.Add(Local.RecordingState.Aborted, Local.RecordingStateString.Aborted);
                            map.Add(Local.RecordingState.Missed, Local.RecordingStateString.Error);
                            map.Add(Local.RecordingState.Error, Local.RecordingStateString.Error);
                            map.Add(Local.RecordingState.Overlapping, Local.RecordingStateString.Overlapping);
                            map.Add(Local.RecordingState.Finished, Local.RecordingStateString.Finished);
                            map.Add(Local.RecordingState.Warning, Local.RecordingStateString.Warning);

                            List<int> states = new List<int>()
                            {
                                (int)Local.RecordingState.Scheduled, 
                                (int)Local.RecordingState.InProgress, 
                                (int)Local.RecordingState.Aborted, 
                                (int)Local.RecordingState.Missed, 
                                (int)Local.RecordingState.Error, 
                                (int)Local.RecordingState.Finished, 
                                (int)Local.RecordingState.Overlapping, 
                                (int)Local.RecordingState.Deleted, 
                                (int)Local.RecordingState.Warning
                            };

                            var recs =
                                from row in Context.Recording
                                where row.Web_id > 0 && states.Contains(row.State)
                                select row;

                            foreach(var r in recs)
                            {
                                Web.Recording recording;

                                if(webRecordings.TryGetValue(r.Web_id, out recording))
                                {
                                    if(r.State == (int)Local.RecordingState.Deleted || recording.RecordingState == Local.RecordingStateString.Deleted)
                                    {
                                        Log.WriteLine("Deleting recording (channelId = {0}, programId = {1})", recording.ChannelId, recording.ProgramId);

                                        deletedIds.Add(r.Id);

                                        svc.UpdateRecording(svc.SessionId, r.Web_id, Local.RecordingStateString.Deleted);

                                        Context.Recording.DeleteOnSubmit(r);

                                        svc.DeleteRecording(svc.SessionId, r.Web_id);
                                    }
                                    else
                                    {
                                        string state;

                                        if(map.TryGetValue((Local.RecordingState)r.State, out state))
                                        {
                                            if(recording.RecordingState != state)
                                            {
                                                Log.WriteLine("Updating recording (channelId = {0}, programId = {1}, state = {2})", recording.ChannelId, recording.ProgramId, (Local.RecordingState)r.State);

                                                svc.UpdateRecording(svc.SessionId, r.Web_id, state);
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    r.Web_id = 0;
                                }

                                Context.SubmitChanges();
                            }
                        }
                        catch(Exception e)
                        {
                            Log.WriteLine(e);

                            return;
                        }
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e.Message);
            }
        }

        void UpdateRecording(Guid recordingId, Local.RecordingState state, string path = null)
        {
            using(var Context = CreateDataContext())
            {
                var r = Context.Recording.Single(row => row.Id == recordingId);

                r.State = (int)state;

                if(state == Local.RecordingState.InProgress)
                {
                    int rating = 0;

                    Local.Recording rec = GetRecording(recordingId);

                    if(rec.program != null)
                    {
                        rating = Math.Max(rec.program.episode.movie.rating, rating);
                    }

                    if(rec.preset != null)
                    {
                        switch(rec.preset.rating)
                        {
                            case 1: rating = Math.Max(rating, 12); break;
                            case 2: rating = Math.Max(rating, 16); break;
                            case 3: rating = Math.Max(rating, 18); break;
                            case 4: rating = Math.Max(rating, Int32.MaxValue); break;
                        }
                    }

                    r.Start = DateTime.Now;
                    r.Rating = rating;
                }
                else if(state == Local.RecordingState.Aborted || state == Local.RecordingState.Finished) // WHERE stop IS NULL ???
                {
                    r.Stop = DateTime.Now;

                    Local.Recording rec = GetRecording(recordingId);

                    switch(rec.GetTimeState())
                    {
                        case Local.RecordingTimeState.StopDelay:
                        case Local.RecordingTimeState.After:
                            r.State = (int)Local.RecordingState.Finished;
                            break;
                    }

                    if(!r.Program_id.HasValue && r.Channel_id != 0)
                    {
                        TimeSpan tsmax = new TimeSpan(0);

                        foreach(Local.Program p in GetPrograms(Guid.Empty, r.Channel_id, r.Start, r.Stop.Value))
                        {
                            DateTime start = p.start >= r.Start ? p.start : r.Start;
                            DateTime stop = p.stop <= r.Stop.Value ? p.stop : r.Stop.Value;

                            TimeSpan ts = stop - start;

                            if(ts > tsmax)
                            {
                                tsmax = ts;

                                r.Name = p.episode.title.Length > 0 ? p.episode.movie.title + " - " + p.episode.title : p.episode.movie.title;
                            }
                        }
                    }
                }

                if(path != null)
                {
                    r.Path = path;
                }

                Context.SubmitChanges();
            }
        }

        void UpdateOverlappingRecordings()
        {
            try
            {
                int n = 0;

                using(var Context = CreateDataContext(false))
                {
                    n = Context.Recording.Count(r => r.State == (int)Local.RecordingState.Overlapping);
                }

                if(n > 0)
                {
                    PostServiceMessage(String.Format(_localizer[StringLocalizerType.RecordingCollision], n), Local.ServiceMessageType.OverlappingRecording);
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        void UpdateRecordingState(Guid recordingId)
        {
            try
            {
                using(var Context = CreateDataContext())
                {
                    var r = Context.Recording.FirstOrDefault(row => row.Id == recordingId && row.Name == "");

                    if(r != null)
                    {
                        if(r.Program_id.HasValue)
                        {
                            var r2 =
                                from p in Context.Program
                                from e in Context.Episode
                                from m in Context.Movie
                                where p.Id == r.Program_id.Value && p.Episode_id == e.Id && e.Movie_id == m.Id
                                select new { Movie = m.Title, Episode = e.Title };

                            var t = r2.FirstOrDefault();

                            if(t != null)
                            {
                                r.Name = t.Episode.Length > 0 ? t.Movie + " - " + t.Episode : t.Movie;
                            }
                        }
                        else
                        {
                            r.Name = _localizer[StringLocalizerType.ManualRecording];

                            if(r.Preset_id.HasValue)
                            {
                                r.Name += " - " + Context.Preset.Single(p => p.Id == r.Preset_id.Value).Name;
                            }
                        }
                    }

                    Context.SubmitChanges();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }

            // TODO

            try
            {
                using(var Context = CreateDataContext())
                {
                    List<int> states = new List<int>()
                    {
                        (int)Local.RecordingState.Scheduled,
                        (int)Local.RecordingState.InProgress,
                        (int)Local.RecordingState.Overlapping,
                        (int)Local.RecordingState.Prompt,
                    };

                    var r =
                        from r1 in Context.Recording
                        join r2 in Context.Recording on r1.Id equals r2.Id
                        where r1.Id != r2.Id && r1.Start < r2.Stop && r1.Stop > r2.Start
                        && r1.State == (int)Local.RecordingState.Scheduled && states.Contains(r2.State)
                        select r1;

                    int conflicts = r.Count();

                    if(conflicts > 0 && conflicts >= Context.Tuner.Count())
                    {
                        var rec = Context.Recording.FirstOrDefault(row => row.Id == recordingId);

                        if(rec != null)
                        {
                            rec.State = (int)Local.RecordingState.Overlapping;
                        }
                    }

                    Context.SubmitChanges();
                }

                ResetWebUpdate(WebUpdateType.OverlappingRecordings);
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        void UpdateRecordingPathWatcher()
		{
            lock(_sync)
            {
                try
                {
                    if(_recordingPathWatcher != null)
                    {
                        _recordingPathWatcher.EnableRaisingEvents = false;
                        _recordingPathWatcher.Dispose();
                        _recordingPathWatcher = null;
                    }

                    string path = RecordingPath;

                    if(path != null)
                    {
                        _recordingPathWatcher = new FileSystemWatcher(path, "*");
                        _recordingPathWatcher.NotifyFilter = NotifyFilters.FileName;
                        _recordingPathWatcher.Renamed += OnRecordingRenamed;
                        _recordingPathWatcher.Deleted += OnRecordingDeleted;
                        _recordingPathWatcher.EnableRaisingEvents = true;
                    }
                }
                catch(Exception e)
                {
                    Log.WriteLine(e);

                    if(_recordingPathWatcher != null)
                    {
                        _recordingPathWatcher.EnableRaisingEvents = false;
                        _recordingPathWatcher.Dispose();
                        _recordingPathWatcher = null;
                    }

                    _recordingPathWatcher = new FileSystemWatcher();
                }
            }
		}

		void OnRecordingRenamed(Object source, RenamedEventArgs e)
		{
			lock(_sync)
            {
    			RenameRecording(e.OldFullPath, e.FullPath);

    			Log.WriteLine("{0} => {1}", e.OldName, e.Name);
            }
		}

		void OnRecordingDeleted(Object source, FileSystemEventArgs e)
		{
			lock(_sync)
            {
    			if(e.ChangeType == WatcherChangeTypes.Deleted)
    			{
    				RenameRecording(e.FullPath, null);
    			}

                Log.WriteLine("{0} {1}", e.Name, e.ChangeType);
            }
		}

        void RenameRecording(string src, string dst)
        {
            try
            {
                lock(_recordingPathWatcher)
                {
                    using(var Context = CreateDataContext())
                    {
                        if(dst != null)
                        {
                            foreach(var r in from row in Context.Recording where row.Path == src select row)
                            {
                                r.Path = dst;
                            }
                        }
                        else
                        {
                            foreach(var r in from row in Context.Recording where row.Path == src select row)
                            {
                                r.State = (int)Local.RecordingState.Deleted;
                            }
                        }

                        Context.SubmitChanges();
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        public DateTime? GetNextRecordingStart()
        {
            try
            {
                using(var Context = CreateDataContext(false))
                {
                    var q =
                        from row in Context.Recording
                        where row.Start > DateTime.Now && row.State == (int)Local.RecordingState.Scheduled
                        select new { Start = row.Start.AddSeconds(row.Start_delay) };

                    var r = (from row in q orderby row.Start select row).FirstOrDefault();

                    if(r != null)
                    {
                        return r.Start;
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }

            return null;
        }

        List<TunerForRecording> EnumTunersForRecording(int channelId, int presetId)
		{
			try
			{
                List<TunerForRecording> ts = new List<TunerForRecording>();

                using(var Context = CreateDataContext(false))
                {
                    if(channelId == 0 && presetId > 0)
                    {
                        channelId = (from row in Context.Preset where row.Id == presetId select row.Channel_id).SingleOrDefault();
                    }

                    IQueryable<Data.Preset> q = null;

                    if(channelId > 0)
                    {
                        q = from row in Context.Preset where row.Channel_id == channelId && row.Enabled > 0 select row;
                    }
                    else
                    {
                        q = from row in Context.Preset where row.Id == presetId select row;
                    }

                    int index = 0;

                    foreach(var r in q)
                    {
                        TunerForRecording t = new TunerForRecording();

                        t.index = index++;
                        t.presetId = r.Id;
                        t.tunerId = r.Tuner_id;
                        t.state = Local.RecordingTimeState.None;
                        t.activeAndFree = false;

                        Guid id = IsRecording(t.tunerId);

                        if(id != Guid.Empty)
                        {
                            t.state = GetRecording(id).GetTimeState();
                        }
                        else
                        {
                            if(t.tunerId == _tunerId)
                            {
                                Tuner tuner = null;

                                if(_tuners.TryGetValue(t.tunerId, out tuner))
                                {
                                    if(t.presetId == tuner.GetPresetId())
                                    {
                                        t.activeAndFree = true;
                                    }
                                }
                            }
                        }

                        ts.Add(t);
                    }
                }

                ts.Sort(delegate(TunerForRecording x, TunerForRecording y)
                {
                    if(x == y) return 0;

                    if(x.activeAndFree) return -1;

                    Local.RecordingTimeState[] states = new Local.RecordingTimeState[] 
                    {
                        Local.RecordingTimeState.None, 
                        Local.RecordingTimeState.StopDelay
                    };

                    foreach(var state in states)
                    {
                        if(x.state != state && y.state == state) return +1;
                        if(x.state == state && y.state != state) return -1;

                        if(x.state == state && y.state == state)
                        {
                            if(x.tunerId == _tunerId) return +1;
                            if(y.tunerId == _tunerId) return -1;

                            break;
                        }
                    }

                    return x.index - y.index;
                });

				return ts;
			}
			catch(Exception e)
			{
				Log.WriteLine(e);
			}					

			return null;
		}

        void RepairTuners()
		{
            try
            {
                using(var Context = CreateDataContext())
                {
                    if(Context.Tuner.Count() == 0)
                    {
                        return;
                    }

                    // find tuners that are not present on the system, look for the same friendly name on the avail list

                    var devs = Tuner.EnumTuners();// NoType();

                    var avail = new List<Local.TunerDevice>(devs);

                    avail.RemoveAll(dev => Context.Tuner.Any(t => t.Display_name == dev.dispname));// .FirstOrDefault(t => t.Display_name == dev.dispname) != null);

                    foreach(var t in Context.Tuner)
                    {
                        if(!devs.Any(dev => dev.dispname == t.Display_name))
                        {
                            Local.TunerDevice match = avail.FirstOrDefault(dev => dev.name == t.Friendly_name);

                            if(match != null)
                            {
                                Log.WriteLine("Tuner display name changed: {0}, {1} => {2}", t.Friendly_name, t.Display_name, match.dispname);
                                
                                t.Display_name = match.dispname;

                                avail.Remove(match);
                            }
                            else
                            {
                                Log.WriteLine("Tuner lost ({0}, {1})", t.Friendly_name, t.Display_name);
                            }
                        }
                    }

                    Context.SubmitChanges();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e.Message);
            }
        }

        public void PostServiceMessage(string message)
        {
            PostServiceMessage(message, 0);
        }

        public void PostServiceMessage(string message, Local.ServiceMessageType type)
        {
            PostServiceMessage(message, type, new TimeSpan(0, 1, 0), 0, 0);
        }

        public void PostServiceMessage(string message, TimeSpan? valid, int repeat, int delay)
        {
            PostServiceMessage(message, Local.ServiceMessageType.None, valid, repeat, delay);
        }

        public void PostServiceMessage(string message, Local.ServiceMessageType type, TimeSpan? valid, int repeat, int delay)
        {
            try
            {
                using(var Context = CreateDataContext())
                {
                    Context.Delete(
                        Context.ServiceMessage,
                        from row in Context.ServiceMessage
                        where row.Valid_until < DateTime.Now || row.Type == (int)type
                        select row.Id);

                    Context.ServiceMessage.InsertOnSubmit(new Data.ServiceMessage()
                    {
                        Message = message,
                        Post_date = DateTime.Now,
                        Valid_until = valid != null ? DateTime.Now + valid : null,
                        Repeat = repeat,
                        Delay = delay,
                        Type = (int)type,
                    });

                    Context.SubmitChanges();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        public void PostUpdateGuiderServiceMessage(int current, int steps)
        {
            SetWebUpdatePercent(WebUpdateType.Guide, current, steps);

            PostServiceMessage(String.Format(_localizer[StringLocalizerType.GuideUpdateStep], current, steps));
        }

        public DateTime GetWebUpdate(WebUpdateType type)
        {
            DateTime at;

            using(var Context = CreateDataContext())
            {
                at = (from row in Context.WebUpdate where row.Type == (int)type select row.At).SingleOrDefault();

                if(at.Ticks == 0)
                {
                    at = DateTimeMin;

                    Context.WebUpdate.InsertOnSubmit(new Data.WebUpdate() { Type = (int)type, At = at });
                }

                Context.SubmitChanges();
            }

            return at;
        }

        public string GetWebUpdateError(WebUpdateType type)
        {
            using(var Context = CreateDataContext(false))
            {
                return (from row in Context.WebUpdate where row.Type == (int)type select row.Msg).SingleOrDefault();
            }
        }

        public int GetWebUpdatePercent(WebUpdateType type)
        {
            using(var Context = CreateDataContext(false))
            {
                return (from row in Context.WebUpdate where row.Type == (int)type select row.Percent_done).SingleOrDefault();
            }
        }

        public void SetWebUpdate(WebUpdateType type, DateTime at)
        {
            GetWebUpdate(type);

            using(var Context = CreateDataContext())
            {
                var r = (from row in Context.WebUpdate where row.Type == (int)type select row).SingleOrDefault();

                if(r != null)
                {
                    r.Msg = null;
                    r.At = at;
                }

                Context.SubmitChanges();
            }
        }

        public void SetWebUpdateError(WebUpdateType type, string msg)
        {
            GetWebUpdate(type);

            using(var Context = CreateDataContext())
            {
                var r = (from row in Context.WebUpdate where row.Type == (int)type select row).SingleOrDefault();

                if(r != null)
                {
                    r.Msg = msg;
                }

                Context.SubmitChanges();
            }
        }

        public void SetWebUpdatePercent(WebUpdateType type, int step, int total)
        {
            GetWebUpdate(type);

            using(var Context = CreateDataContext())
            {
                var r = (from row in Context.WebUpdate where row.Type == (int)type select row).SingleOrDefault();

                if(r != null)
                {
                    r.Percent_done = Math.Min(step * 100 / total, 100);
                }

                Context.SubmitChanges();
            }
        }

        public void ResetWebUpdate(WebUpdateType type)
        {
            SetWebUpdate(type, DateTimeMin);
        }

        #region IDisposable Members

        public void Dispose()
        {
            bool taken = false;

            try
            {
                Monitor.TryEnter(_timer.recording, ref taken);

                if(taken)
                {
                    _timer.wakeup.Dispose();
                    _timer.wakeup = null;
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
            finally
            {
                if(taken)
                {
                    Monitor.Exit(_timer.recording);
                }
            }

            if(_timer.wakeup != null)
            {
                _timer.wakeup.Dispose();
            }

            DeactivateAll();
            /**/
            if(_mediaserver != null)
            {
                _mediaserver.Unregister();
            }
            
        }

        #endregion

        #region IService Members

        public Local.MachineReg Machine
        {
            get
            {
                using(var Context = CreateDataContext(false))
                {
                    var q = from row in Context.Machine select new Local.MachineReg()
                    {
                        id = row.Id,
                        webId = row.Web_id.HasValue ? row.Web_id.Value : Guid.Empty,
                        name = row.Name,
                        published = row.Published,
                        recordingPath = row.Recording_path ?? DefaultRecordingPath,
                        recordingFormat = row.Recording_format ?? DefaultRecordingFormat,
                        timeshiftPath = row.Timeshift_path ?? Path.GetTempPath(),
                        timeshiftDur = TimeSpan.FromMinutes((double)row.Timeshift_dur),
                        downloadPath = row.Download_path ?? DefaultDownloadPath,
                        username = row.Username ?? "",
                        password = row.Password ?? ""
                    };

                    return q.FirstOrDefault();
                }
            }
        }

        public bool IsOnline
        {
            get
            {
                return _online_user && _online_server;
            }
        }

        public string Username
        {
            get
            {
                Local.MachineReg m = Machine;

                return m != null ? m.username : null;
            }

            set
            {
                bool changed = false;

                using(var Context = CreateDataContext())
                {
                    var m = (from row in Context.Machine select row).FirstOrDefault();

                    if(m != null && m.Username != value)
                    {
                        m.Username = value;

                        changed = true;
                    }

                    Context.SubmitChanges();
                }

                if(changed)
                {
                    OnLoginChanged();
                }
            }
        }

        public string Password
        {
            get
            {
                Local.MachineReg m = Machine;

                return m != null ? m.password : null;
            }

            set
            {
                bool changed = false;

                using(var Context = CreateDataContext())
                {
                    var m = (from row in Context.Machine select row).FirstOrDefault();

                    if(m != null && m.Password != value)
                    {
                        m.Password = value;
                        
                        changed = true;
                    }

                    Context.SubmitChanges();
                }

                if(changed)
                {
                    OnLoginChanged();
                }
            }
        }

        public string Language
        {
            get
            {
                return _localizer.Language;
            }

            set
            {
                _localizer.Language = value;
            }
        }

        public string Region
        {
            get
            {
                return _guide.Region;
            }

            set
            {
                _guide.Region = value;
            }
        }

        public string RecordingPath
        {
            get
            {
                Local.MachineReg m = Machine;

                return (m != null ? m.recordingPath : DefaultRecordingPath).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            }

            set
            {
                if(!Directory.Exists(value))
                {
                    throw new FaultException("Recording path does not exists");
                }

                using(var Context = CreateDataContext())
                {
                    var m = (from row in Context.Machine select row).FirstOrDefault();

                    if(m != null)
                    {
                        m.Recording_path = value;
                    }

                    Context.SubmitChanges();
                }

                UpdateRecordingPathWatcher();
            }
        }

        public string RecordingFormat
        {
            get
            {
                Local.MachineReg m = Machine;

                return m != null ? m.recordingFormat : DefaultRecordingFormat;
            }

            set
            {
                using(var Context = CreateDataContext())
                {
                    var m = (from row in Context.Machine select row).FirstOrDefault();

                    if(m != null)
                    {
                        m.Recording_format = value;
                    }

                    Context.SubmitChanges();
                }

                UpdateRecordingPathWatcher();
            }
        }

        public string TimeshiftPath
        {
            get
            {
                Local.MachineReg m = Machine;

                return (m != null ? m.timeshiftPath : DefaultTimeshiftPath).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            }

            set
            {
                if(!Directory.Exists(value))
                {
                    throw new FaultException("Timeshift path does not exists");
                }

                using(var Context = CreateDataContext())
                {
                    var m = (from row in Context.Machine select row).FirstOrDefault();

                    if(m != null)
                    {
                        m.Timeshift_path = value;
                    }

                    Context.SubmitChanges();
                }
            }
        }

        public TimeSpan TimeshiftDuration
        {
            get
            {
                Local.MachineReg m = Machine;

                return m != null ? m.timeshiftDur : new TimeSpan(0);
            }

            set
            {
                using(var Context = CreateDataContext())
                {
                    var m = (from row in Context.Machine select row).FirstOrDefault();

                    if(m != null)
                    {
                        m.Timeshift_dur = (int)value.TotalMinutes;
                    }

                    Context.SubmitChanges();
                }
            }
        }

        public string DownloadPath
        {
            get
            {
                Local.MachineReg m = Machine;

                return (m != null ? m.downloadPath : DefaultDownloadPath).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            }

            set
            {
                if(!Directory.Exists(value))
                {
                    throw new FaultException("Recording path does not exists");
                }

                using(var Context = CreateDataContext())
                {
                    var m = (from row in Context.Machine select row).FirstOrDefault();

                    if(m != null)
                    {
                        m.Download_path = value;
                    }

                    Context.SubmitChanges();
                }

                UpdateRecordingPathWatcher();
            }
        }

        public IEnumerable<Local.TunerDevice> AllTuners
        {
            get
            {
                DeactivateAll(); // FIXME

                return Tuner.EnumTuners();
            }
        }

        public IEnumerable<Local.TunerReg> Tuners
        {
            get
            {
                using(var Context = CreateDataContext(false))
                {
                    var q = from r in Context.Tuner select new Local.TunerReg()
                    {
                        id = r.Id,
                        type = (Local.TunerDeviceType)r.Type,
                        webId = r.Web_id.HasValue ? r.Web_id.Value : Guid.Empty,
                        dispname = r.Display_name,
                        name = r.Friendly_name,
                        connectors = GetTunerConnectors(r.Id).ToList()
                    };

                    return q.ToList();
                }
            }
        }

        public IEnumerable<Local.SmartCardDevice> SmartCards
        {
            get
            {
                lock(_sync)
                {
                    try
                    {
                        if(_cardserver == null)
                        {
                            _cardserver = new SmartCardServer();
                        }

                        return _cardserver.GetSmartCardDevices();
                    }
                    catch(Exception e)
                    {
                        Log.WriteLine(e);

                        throw;
                    }
                }
            }
        }

        public IEnumerable<Local.Channel> Channels
        {
            get
            {
                using(var Context = CreateDataContext(false))
                {
                    var q =
                        from r in Context.Channel
                        orderby r.Radio, r.Name
                        select new Local.Channel()
                        {
                            id = r.Id,
                            name = r.Name,
                            logo = r.Logo != null ? r.Logo.ToArray() : null,
                            radio = r.Radio,
                            rank = r.Rank
                        };

                    return q.ToList();
                }
            }
        }

        public IEnumerable<Local.Recording> Recordings
        {
            get
            {
                return GetRecordings();
            }
        }

        public IEnumerable<Local.WishlistRecording> WishlistRecordings
        {
            get
            {
                return GetWishlistRecordings();
            }
        }

        public Local.ParentalSettings Parental
        {
            get
            {
                using(var Context = CreateDataContext(false))
                {
                    var q = from r in Context.Parental select new Local.ParentalSettings()
                    {
                        password = r.Password,
                        enabled = r.Enabled,
                        inactivity = r.Inactivity,
                        rating = r.Rating
                    };

                    return q.First();
                }
            }

            set
            {
                using(var Context = CreateDataContext())
                {
                    Data.Parental p = Context.Parental.First();

                    p.Enabled = value.enabled;
                    p.Inactivity = value.inactivity;
                    p.Rating = value.rating;
                    p.Password = p.Enabled || p.Inactivity > 0 ? value.password : "";

                    Context.SubmitChanges();
                }
            }
        }

        public Local.AppVersion MostRecentVersion
        {
            get
            {
                lock(_sync)
                {
                    Local.AppVersion av = new Local.AppVersion();

                    av.version = _appversion.version;
                    av.url = _appversion.url;
                    av.required = _appversion.required;

                    return av;
                }
            }
        }

        public UInt64 InstalledVersion
        {
            get
            {
                Version v = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version;

                UInt64 version =
                    ((UInt64)v.Major << 48) |
                    ((UInt64)v.Minor << 32) |
                    ((UInt64)v.Build << 16) |
                    ((UInt64)v.Revision << 0);

                return version;
            }
        }

        public DateTime LastGuideUpdate
        {
            get
            {
                return GetWebUpdate(WebUpdateType.Guide);
            }
        }

        public string LastGuideError
        {
            get
            {
                return GetWebUpdateError(WebUpdateType.Guide);
            }
        }

        public int GuideUpdatePercent
        {
            get
            {
                return GetWebUpdatePercent(WebUpdateType.Guide);
            }
        }
        public void RegisterUser(string username, string password, string email)
        {
            using(var svc = CreateWebService())
            {
                string userId = svc.RegisterUser(username, password, email);

                if(userId == null || userId == "")
                {
                    throw new FaultException("RegisterUser failed");
                }

                using(var Context = CreateDataContext())
                {
                    var m = Context.Machine.First();

                    m.Username = username;
                    m.Password = password;

                    Context.SubmitChanges();
                }
            }

            Unpublish();
        }

        public bool UserExists(string username)
        {
            using(var svc = CreateWebService())
            {
                return svc.UserExists(username);
            }
        }

        public bool LoginExists(string username, string password)
        {
            using(var svc = CreateWebService())
            {
                return svc.LoginExists(username, password);
            }
        }

        public void RegisterMachine(string name, List<Local.TunerDevice> devs)
        {
            lock(_sync)
            {
                DeactivateAll();

                List<Guid> oldTunerIds = new List<Guid>();
                List<Guid> newTunerIds = new List<Guid>();

                using(TransactionScope ts = new TransactionScope())
                {
                    using(var Context = CreateDataContext())
                    {
                        var m = Context.Machine.First();

                        Context.SubmitChanges();

                        foreach(var t in Context.Tuner)
                        {
                            int index = devs.FindIndex(td => td.dispname == t.Display_name);

                            if(index < 0)
                            {
                                oldTunerIds.Add(t.Id);
                            }
                            else
                            {
                                devs.RemoveAt(index);
                            }
                        }

                        foreach(Local.TunerDevice td in devs)
                        {
                            Guid tunerId = Guid.NewGuid();

                            using(Tuner t = new Tuner())
                            {
                                t.Open(td, tunerId, null);

                                Context.Tuner.InsertOnSubmit(new Data.Tuner() 
                                { 
                                    Id = tunerId, 
                                    Type = (int)td.type, 
                                    Display_name = td.dispname, 
                                    Friendly_name = td.name
                                });

                                int num = 0;

                                Context.Connector.InsertAllOnSubmit(from type in t.GetConnectorTypes() select new Data.Connector()
                                {
                                    Tuner_id = tunerId,
                                    Num = num++,
                                    Type = type,
                                    Name = Tuner.GetConnectorName(type)
                                });

                                newTunerIds.Add(tunerId);
                            }
                        }

                        Context.SubmitChanges();

                        Context.Delete(Context.Tuner, oldTunerIds);
                        Context.Delete(Context.Preset, oldTunerIds, "tuner_id");
                    }

                    ts.Complete();
                }

                UpdateRecordingPathWatcher();
            }

            lock(_presets)
            {
                _presets.Clear();
            }

            Unpublish();
        }

        public void SetUserInfo(Local.UserInfo info)
        {
            using(var svc = CreateWebService(true))
            {
                Web.UserInfo winfo = new Web.UserInfo();

                winfo.Email = info.email;
                winfo.FirstName = info.firstName;
                winfo.LastName = info.lastName;
                winfo.Country = info.country;
                winfo.Address = info.address;
                winfo.PostalCode = info.postalCode;
                winfo.PhoneNumber = info.phoneNumber;
                winfo.Gender = info.gender;

                svc.SetUserInfo(svc.SessionId, winfo);
            }
        }

        public Local.UserInfo GetUserInfo()
        {
            using(var svc = CreateWebService(true))
            {
                Web.UserInfo winfo = svc.GetUserInfo(svc.SessionId);

                Local.UserInfo info = new Local.UserInfo();

                info.email = winfo.Email;
                info.firstName = winfo.FirstName;
                info.lastName = winfo.LastName;
                info.country = winfo.Country;
                info.address = winfo.Address;
                info.postalCode = winfo.PostalCode;
                info.phoneNumber = winfo.PhoneNumber;
                info.gender = winfo.Gender;

                return info;
            }
        }

        public bool GoOnline()
        {
            try
            {
                using(var svc = CreateWebService(true))
                {
                    // nothing to do, _online_user = true was already set if CreateWebService was successful

                    return true;
                }
            }
            catch(Exception)
            {
            }

            return false;
        }

        public Local.TunerReg GetTuner(Guid tunerId)
        {
            using(var Context = CreateDataContext(false))
            {
                var q = from r in Context.Tuner where r.Id == tunerId select new Local.TunerReg()
                {
                    id = r.Id,
                    type = (Local.TunerDeviceType)r.Type,
                    webId = r.Web_id.HasValue ? r.Web_id.Value : Guid.Empty,
                    dispname = r.Display_name,
                    name = r.Friendly_name,
                    connectors = GetTunerConnectors(r.Id).ToList()
                };
                
                return q.SingleOrDefault();
            }
        }

        public IEnumerable<Local.TunerConnector> GetTunerConnectors(Guid tunerId)
        {
            using(var Context = CreateDataContext())
            {
                var q = from r in Context.Connector where r.Tuner_id == tunerId select new Local.TunerConnector()
                {
                    id = r.Id,
                    num = r.Num,
                    type = r.Type,
                    name = r.Name
                };
                
                return q.ToList();
            }
        }

        public Local.TunerStat GetTunerStat(Guid tunerId)
        {
            lock(_sync)
            {
				Tuner t;

				if(_tuners.TryGetValue(tunerId, out t))
				{
                    return t.GetStat();
				}

                lock(_scan)
                {
    				if(_scan.tuner != null)
    				{
                        return _scan.tuner.GetStat();
    				}
                }

				return null;
            }
        }

        public IEnumerable<Local.Preset> GetPresets(Guid tunerId, bool enabled)
        {
            using(var Context = CreateDataContext(false))
            {
                var r =
                    from r1 in Context.Preset
                    join r2 in Context.Connector on r1.Connector_id equals r2.Id
                    where r1.Tuner_id == tunerId && r1.Enabled >= (enabled ? 1 : 0)
                    orderby r1.Enabled
                    select new Local.Preset()
                    {
                        id = r1.Id,
                        channelId = r1.Channel_id,
                        scrambled = r1.Scrambled,
                        enabled = r1.Enabled,
                        rating = r1.Rating,
                        provider = r1.Provider,
                        name = r1.Name,
                        tunereq = new Local.TuneRequest()
                        {
                            freq = r1.Freq,
                            standard = r1.Standard,
                            connector = new Local.TunerConnector() { num = r2.Num, type = r2.Type, name = r2.Name },
                            sid = r1.Sid,
                            ifec = r1.Ifec,
                            ifecRate = r1.Ifecrate,
                            ofec = r1.Ofec,
                            ofecRate = r1.Ofecrate,
                            modulation = r1.Modulation,
                            symbolRate = r1.Symbol_rate,
                            polarisation = r1.Polarisation,
                            west = r1.West,
                            orbitalPosition = r1.Orbital_position,
                            azimuth = r1.Azimuth,
                            elevation = r1.Elevation,
                            lnbSource = r1.Lnb_source,
                            lowOscillator = r1.Low_oscillator,
                            highOscillator = r1.High_oscillator,
                            lnbSwitch = r1.Lnb_switch,
                            spectralInversion = r1.Spectral_inversion,
                            bandwidth = r1.Bandwidth,
                            lpifec = r1.Lpifec,
                            lpifecRate = r1.Lpifecrate,
                            halpha = r1.Halpha,
                            guard = r1.Guard,
                            transmissionMode = r1.Transmission_mode,
                            otherFreqInUse = r1.Other_freq_in_use,
                            path = r1.Path,
                        }
                    };

                return r.ToList();
            }
        }

        public void SetPresets(Guid tunerId, List<int> presetIds)
        {
            Dictionary<int, bool> oldChannelIds = new Dictionary<int, bool>();
            Dictionary<int, bool> newChannelIds = new Dictionary<int, bool>();

            using(TransactionScope ts = new TransactionScope())
            {
                using(var Context = CreateDataContext())
                {
                    foreach(var r in (from row in Context.Preset where row.Tuner_id == tunerId select row))
                    {
                        if(r.Enabled > 0 && r.Channel_id > 0)
                        {
                            oldChannelIds[r.Channel_id] = true;
                        }

                        r.Enabled = 0;
                    }

                    Context.SubmitChanges();

                    int i = 0;

                    foreach(int id in presetIds)
                    {
                        var r = (from row in Context.Preset where row.Id == id select row).SingleOrDefault();

                        if(r == null) continue;

                        if(r.Channel_id > 0)
                        {
                            newChannelIds[r.Channel_id] = true;
                        }

                        r.Enabled = ++i;
                    }

                    Context.SubmitChanges();
                }

                ts.Complete();
            }

            if(oldChannelIds.Count != newChannelIds.Count || oldChannelIds.Intersect(newChannelIds).Count() != oldChannelIds.Count)
            {
                _guide.UpdateActivePrograms();
            }

            _presets.Clear();
        }

        public Local.Preset GetPreset(int presetId)
        {
            Local.Preset preset;

            lock(_presets)
            {
                if(_presets.TryGetValue(presetId, out preset))
                {
                    return preset;
                }
            }

            using(var Context = CreateDataContext(false))
            {
                var r =
                    from r1 in Context.Preset
                    join r2 in Context.Connector on r1.Connector_id equals r2.Id
                    where r1.Id == presetId
                    select new Local.Preset()
                    {
                        id = r1.Id,
                        channelId = r1.Channel_id,
                        scrambled = r1.Scrambled,
                        enabled = r1.Enabled,
                        rating = r1.Rating,
                        provider = r1.Provider,
                        name = r1.Name,
                        tunereq = new Local.TuneRequest()
                        {
                            freq = r1.Freq,
                            standard = r1.Standard,
                            connector = new Local.TunerConnector() { num = r2.Num, type = r2.Type, name = r2.Name },
                            sid = r1.Sid,
                            ifec = r1.Ifec,
                            ifecRate = r1.Ifecrate,
                            ofec = r1.Ofec,
                            ofecRate = r1.Ofecrate,
                            modulation = r1.Modulation,
                            symbolRate = r1.Symbol_rate,
                            polarisation = r1.Polarisation,
                            west = r1.West,
                            orbitalPosition = r1.Orbital_position,
                            azimuth = r1.Azimuth,
                            elevation = r1.Elevation,
                            lnbSource = r1.Lnb_source,
                            lowOscillator = r1.Low_oscillator,
                            highOscillator = r1.High_oscillator,
                            lnbSwitch = r1.Lnb_switch,
                            spectralInversion = r1.Spectral_inversion,
                            bandwidth = r1.Bandwidth,
                            lpifec = r1.Lpifec,
                            lpifecRate = r1.Lpifecrate,
                            halpha = r1.Halpha,
                            guard = r1.Guard,
                            transmissionMode = r1.Transmission_mode,
                            otherFreqInUse = r1.Other_freq_in_use,
                            path = r1.Path,
                        }
                    };

                preset = r.SingleOrDefault();

                if(preset != null)
                {
                    lock(_presets)
                    {
                        _presets[presetId] = preset;
                    }
                }

                return preset;
            }
        }

        public Local.Preset GetCurrentPreset(Guid tunerId)
        {
            using(var Context = CreateDataContext(false))
            {
                return GetPreset((from r in Context.Tuner where r.Id == tunerId select r.Preset_id).SingleOrDefault());
            }
        }

        public int GetCurrentPresetId(Guid tunerId)
        {
            using(var Context = CreateDataContext(false))
            {
                return (from r in Context.Tuner where r.Id == tunerId select r.Preset_id).SingleOrDefault();
            }
        }

        public void SetPreset(Guid tunerId, int presetId)
        {
            lock(_sync)
            {
				Tuner t;

				if(!_tuners.TryGetValue(tunerId, out t))
				{
                    throw new FaultException("Invalid tunerId");
				}

                if(!t.IsTimeshiftDummy() && t.GetPresetId() == presetId)
                {
                    return;
                }

				if(t.IsRecording())
				{
                    throw new FaultException("Cannot change preset while recording");
				}

                Local.Preset p = GetPreset(presetId);

				if(p == null)
				{
                    throw new FaultException("Invalid presetId");
				}

                Local.Channel c = null;

				bool radio = false;

				if(p.channelId != 0)
				{
                    c = GetChannel(p.channelId);

					if(c != null)
					{
						radio = c.radio; // TODO
					}
				}

                if(!t.Timeshift(TimeshiftPath, p.tunereq, p.id))
                {
                    throw new FaultException("Cannot start timeshifting");
                }

                using(var Context = CreateDataContext())
                {
                    var r = (from row in Context.Tuner where row.Id == tunerId select row).SingleOrDefault();

                    if(r != null)
                    {
                        r.Preset_id = presetId;
                    }

                    Context.SubmitChanges();
                }

                CallbackService svc = (CallbackService)_callback.SingletonInstance;

                svc.Dispatch(delegate(Local.ICallback cb) { cb.OnCurrentPresetChanged(tunerId); });
            }
        }

        public void UpdatePreset(Local.Preset preset)
        {
            UpdatePresets(new List<Local.Preset> { preset });
        }

        public void UpdatePresets(List<Local.Preset> presets)
        {
            bool changed = false;

            using(var Context = CreateDataContext())
            {
                foreach(Local.Preset p in presets)
                {
                    var r = (from row in Context.Preset where row.Id == p.id select row).SingleOrDefault();

                    if(r != null)
                    {
                        if(r.Channel_id != p.channelId)
                        {
                            changed = r.Channel_id != p.channelId;
                        }

                        r.Channel_id = p.channelId;
                        r.Rating = p.rating;
                        r.Provider = p.provider;
                        r.Name = p.name;
                        r.Freq = p.tunereq.freq;
                        r.Standard = p.tunereq.standard;
                        r.Sid = p.tunereq.sid;
                        r.Ifec = p.tunereq.ifec;
                        r.Ifecrate = p.tunereq.ifecRate;
                        r.Ofec = p.tunereq.ofec;
                        r.Ofecrate = p.tunereq.ofecRate;
                        r.Modulation = p.tunereq.modulation;
                        r.Symbol_rate = p.tunereq.symbolRate;
                        r.Polarisation = p.tunereq.polarisation;
                        r.West = p.tunereq.west;
                        r.Orbital_position = p.tunereq.orbitalPosition;
                        r.Azimuth = p.tunereq.azimuth;
                        r.Elevation = p.tunereq.elevation;
                        r.Lnb_source = p.tunereq.lnbSource;
                        r.Low_oscillator = p.tunereq.lowOscillator;
                        r.High_oscillator = p.tunereq.highOscillator;
                        r.Lnb_switch = p.tunereq.lnbSwitch;
                        r.Spectral_inversion = p.tunereq.spectralInversion;
                        r.Bandwidth = p.tunereq.bandwidth;
                        r.Lpifec = p.tunereq.lpifec;
                        r.Lpifecrate = p.tunereq.lpifecRate;
                        r.Halpha = p.tunereq.halpha;
                        r.Guard = p.tunereq.guard;
                        r.Transmission_mode = p.tunereq.transmissionMode;
                        r.Other_freq_in_use = p.tunereq.otherFreqInUse;
                        r.Path = p.tunereq.path;
                    }
                }

                Context.SubmitChanges();
            }

            if(changed)
            {
                _guide.UpdateActivePrograms();
            }

            lock(_presets)
            {
                foreach(Local.Preset p in presets)
                {
                    if(_presets.ContainsKey(p.id))
                    {
                        _presets.Remove(p.id);
                    }
                }
            }
        }

        public Local.Preset CreatePreset(Guid tunerId)
        {
            int presetId = 0;

            using(var Context = CreateDataContext())
            {
                var r =
                    from row in Context.Connector
                    where row.Tuner_id == tunerId && row.Type == PhysConn_Video_Tuner
                    orderby row.Num
                    select row.Id;

                int connectorId = r.First(); // ok to throw an exception

                Data.Preset p = new Data.Preset()
                {
                    Tuner_id = tunerId,
                    Name = _localizer[StringLocalizerType.NewPresetName],
                    Connector_id = connectorId
                };

                Context.Preset.InsertOnSubmit(p);

                Context.SubmitChanges();

                presetId = p.Id;
            }

            return GetPreset(presetId);
        }

        public void DeletePreset(int presetId)
        {
            using(var Context = CreateDataContext())
            {
                Context.Delete(Context.Preset, new int[] { presetId });
            }

            lock(_presets)
            {
                if(_presets.ContainsKey(presetId))
                {
                    _presets.Remove(presetId);
                }
            }
        }

        public IEnumerable<Local.Channel> GetChannels(Guid tunerId)
        {
            using(var Context = CreateDataContext(false))
            {
                IQueryable<Data.Channel> q = null;

                if(tunerId == Guid.Empty)
                {
                    q =
                        from c in Context.Channel
                        orderby c.Radio, c.Name
                        select c;
                }
                else
                {
                    q =
                        from c in Context.Channel
                        join p in Context.Preset on c.Id equals p.Channel_id
                        where p.Tuner_id == tunerId && p.Enabled > 0
                        orderby p.Enabled, c.Radio
                        select c;
                }

                var q2 = 
                    from r in q
                    select new Local.Channel()
                    {
                        id = r.Id,
                        name = r.Name,
                        logo = r.Logo != null ? r.Logo.ToArray() : null,
                        radio = r.Radio,
                        rank = r.Rank
                    };

                List<Local.Channel> res = q2.ToList();

                lock(_cache)
                {
                    foreach(Local.Channel c in res)
                    {
                        _cache.channel[c.id] = c;
                    }
                }

                return res;
            }
        }

        public Local.Channel GetChannel(int channelId)
        {
            Local.Channel c;

            lock(_cache)
            {
                if(_cache.channel.TryGetValue(channelId, out c))
                {
                    return c;
                }
            }

            using(var Context = CreateDataContext(false))
            {
                var q =
                    from row in Context.Channel
                    where row.Id == channelId
                    select new Local.Channel()
                    {
                        id = row.Id,
                        name = row.Name,
                        logo = row.Logo != null ? row.Logo.ToArray() : null,
                        radio = row.Radio,
                        rank = row.Rank
                    };

                c = q.SingleOrDefault();

                if(c != null)
                {
                    lock(_cache)
                    {
                        _cache.channel[c.id] = c;
                    }
                }

                return c;
            }
        }

        public IEnumerable<Local.Program> GetPrograms(Guid tunerId, int channelId, DateTime start, DateTime stop)
        {
            using(var Context = CreateDataContext(false))
            {
                var q =
                    from r in Context.ProgramActive
                    from rec in (from row in Context.Recording where row.Program_id == r.Id select row).DefaultIfEmpty()
                    where r.Stop > start && r.Start < stop && !r.Is_deleted
                    orderby r.Start
                    select new Local.Program()
                    {
                        id = r.Id,
                        channelId = r.Channel_id,
                        start = r.Start,
                        stop = r.Stop,
                        episode = GetEpisode(r.Episode_id),
                        state = rec != null ? (Local.RecordingState)rec.State : Local.RecordingState.NotScheduled,
                    };

                IQueryable<Local.Program> q2 = null;

                if(channelId > 0)
                {
                    q2 = from r in q where r.channelId == channelId select r;
                }
                else if(tunerId != Guid.Empty)
                {
                    var c =
                        from r in Context.Preset
                        where r.Tuner_id == tunerId
                        select r.Channel_id;

                    q2 = from r in q where c.Contains(r.channelId) select r;
                }
                else
                {
                    return null;
                }

                List<Local.Program> res = q2.ToList();
                /*
                lock(_cache)
                {
                    foreach(Local.Program p in res)
                    {
                        _cache.program[p.id] = p;
                    }
                }
                */
                return res;
            }
        }

        public Local.Program GetProgram(int programId)
        {
            Local.Program p;

            lock(_cache)
            {
                if(_cache.program.TryGetValue(programId, out p))
                {
                    return p;
                }
            }

            using(var Context = CreateDataContext(false))
            {
                var q =
                    from r in Context.ProgramActive
                    from rec in (from row in Context.Recording where row.Program_id == r.Id select row).DefaultIfEmpty()
                    where r.Id == programId
                    select new Local.Program()
                    {
                        id = r.Id,
                        channelId = r.Channel_id,
                        start = r.Start,
                        stop = r.Stop,
                        episode = GetEpisode(r.Episode_id),
                        state = rec != null ? (Local.RecordingState)rec.State : Local.RecordingState.NotScheduled
                    };
                try
                {
                    p = q.FirstOrDefault();
                }
                catch(Exception e)
                {
                    throw e;
                }
                /*
                lock(_cache)
                {
                    _cache.program[programId] = p;
                }
                */
                return p;
            }
        }

        public IEnumerable<Local.Program> GetFavoritePrograms(int count, bool picture)
        {
            using(var Context = CreateDataContext(false))
            {
                List<Local.Program> res = new List<Local.Program>();

                var q =
                    from p in Context.ProgramActive
                    join mf in Context.MovieFavorite on p.Movie_id equals mf.Id
                    where p.Start >= DateTime.Now
                    select p;
                
                if(picture)
                {
                    q = 
                        from p in q
                        join m in Context.MovieActive on p.Movie_id equals m.Id
                        where m.Has_picture
                        select p;
                }

                foreach(Data.ProgramActive r in q.Distinct().OrderBy(row => row.Start).Take(count))
                {
                    Local.Program p = GetProgram(r.Id);

                    // Log.WriteLine("{0}. {1} {2}", res.Count, p.start, p.episode.movie.title);

                    res.Add(p);
                }

                return res;
            }
        }

        public IEnumerable<Local.Program> GetRelatedPrograms(int programId)
        {
            using(var Context = CreateDataContext(false))
            {
                var q =
                    from p in Context.ProgramActive
                    from pr in Context.ProgramRelated
                    from rec in (from row in Context.Recording where row.Program_id == p.Id select row).DefaultIfEmpty()
                    where pr.Program_id == programId && pr.Other_program_id == p.Id
                    select new Local.Program()
                    {
                        id = p.Id,
                        channelId = p.Channel_id,
                        start = p.Start,
                        stop = p.Stop,
                        state = rec != null ? (Local.RecordingState)rec.State : Local.RecordingState.NotScheduled,
                        episode = GetEpisode(p.Episode_id)
                    };

                return q.Distinct().ToList();
            }
        }

        public IEnumerable<Local.MovieType> GetRecommendedMovieTypes()
        {
            using(var Context = CreateDataContext(false))
            {
                // TODO: store Movietype_id in Progam to speed up this lookup

                var q =
                    from p in Context.ProgramActive
                    join m in Context.MovieActive on p.Movie_id equals m.Id
                    join mt in Context.MovieType on m.Movietype_id equals mt.Id
                    where p.Is_recommended && p.Start >= DateTime.Now
                    // orderby mt.Name_long
                    select new Local.MovieType()
                    {
                        id = mt.Id,
                        nameLong = mt.Name_long,
                        nameShort = mt.Name_short
                    };

                return q.Distinct().OrderBy(row => row.nameLong).ToList();
            }
        }

        public IEnumerable<Local.Program> GetRecommendedProgramsByMovieType(int movieTypeId)
        {
            using(var Context = CreateDataContext(false))
            {
                var q =
                    from p in Context.ProgramActive
                    join m in Context.MovieActive on p.Movie_id equals m.Id
                    from rec in (from row in Context.Recording where p.Id == row.Program_id select row).DefaultIfEmpty()
                    where p.Is_recommended && p.Start >= DateTime.Now && m.Movietype_id == movieTypeId
                    // orderby p.Start
                    select new Local.Program()
                    {
                        id = p.Id,
                        channelId = p.Channel_id,
                        start = p.Start,
                        stop = p.Stop,
                        state = rec != null ? (Local.RecordingState)rec.State : Local.RecordingState.NotScheduled,
                        episode = GetEpisode(p.Episode_id)
                    };

                return q.Distinct().OrderBy(row => row.start).ToList();
            }
        }

        public IEnumerable<Local.Program> GetRecommendedProgramsByChannel(int channelId)
        {
            using(var Context = CreateDataContext(false))
            {
                var q =
                    from p in Context.ProgramActive
                    from rec in (from row in Context.Recording where p.Id == row.Program_id select row).DefaultIfEmpty()
                    where p.Is_recommended && p.Start >= DateTime.Now && p.Channel_id == channelId
                    // orderby p.Start
                    select new Local.Program()
                    {
                        id = p.Id,
                        channelId = p.Channel_id,
                        start = p.Start,
                        stop = p.Stop,
                        state = rec != null ? (Local.RecordingState)rec.State : Local.RecordingState.NotScheduled,
                        episode = GetEpisode(p.Episode_id)
                    };

                return q.Distinct().OrderBy(row => row.start).ToList();
            }
        }

        public Local.Episode GetEpisode(int episodeId)
        {
            Local.Episode e;

            lock(_cache)
            {
                if(_cache.episode.TryGetValue(episodeId, out e))
                {
                    return e;
                }
            }

            using(var Context = CreateDataContext(false))
            {
                e = (from row in Context.EpisodeActive
                     where row.Id == episodeId
                     select new Local.Episode()
                     {
                         id = row.Id,
                         title = row.Title,
                         desc = row.Description,
                         episodeNumber = row.Episode_number,
                         movie = GetMovie(row.Movie_id),
                         directors = new List<Local.Cast>(),
                         actors = new List<Local.Cast>()
                     }).SingleOrDefault();

                if(e != null)
                {
                    var q =
                        from c in Context.Cast
                        join ec in Context.EpisodeCast on c.Id equals ec.Cast_id
                        where ec.Episode_id == episodeId
                        // orderby c.Name, ec.Name
                        select new { c.Id, c.Name, MovieName = ec.Name, Role = ec.Role_id };

                    foreach(var r in q.Distinct().OrderBy(row => row.Name).ThenBy(row => row.MovieName))
                    {
                        Local.Cast c = new Local.Cast()
                        {
                            id = r.Id,
                            name = r.Name,
                            movieName = r.MovieName,
                        };

                        switch(r.Role)
                        {
                            case 1: e.directors.Add(c); break;
                            case 2: e.actors.Add(c); break;
                        }
                    }
                }

                lock(_cache)
                {
                    _cache.episode[episodeId] = e;
                }

                return e;
            }
        }

        public Local.Movie GetMovie(int movieId)
        {
            Local.Movie m;

            lock(_cache)
            {
                if(_cache.movie.TryGetValue(movieId, out m))
                {
                    return m;
                }
            }

            using(var Context = CreateDataContext(false))
            {
                var r =
                    from r1 in Context.MovieActive
                    from r2 in (from row in Context.MovieType where row.Id == r1.Movietype_id select row).DefaultIfEmpty()
                    from r3 in (from row in Context.DVBCategory where row.Id == r1.Dvbcategory_id select row).DefaultIfEmpty()
                    from r4 in (from row in Context.MovieFavorite where row.Id == r1.Id select row).DefaultIfEmpty()
                    where r1.Id == movieId
                    select new Local.Movie()
                    {
                        id = r1.Id,
                        title = r1.Title,
                        desc = r1.Description,
                        rating = r1.Rating,
                        year = r1.Year,
                        episodeCount = r1.Episode_count,
                        movieTypeShort = r2.Name_short,
                        movieTypeLong = r2.Name_long,
                        dvbCategory = r3.Name,
                        favorite = r4 != null,
                        hasPicture = r1.Has_picture
                   };

                m = r.SingleOrDefault();

                lock(_cache)
                {
                    _cache.movie[movieId] = m;
                }

                return m;
            }
        }

        public IEnumerable<Local.ProgramSearchResult> SearchPrograms(string query, Local.ProgramSearchType type)
        {
            if(query.Length < 4)
            {
                return null;
            }

            using(var Context = CreateDataContext(false))
            {
                IQueryable<KeyValuePair<int, string>> q = null;

                switch(type)
                {
                    case Local.ProgramSearchType.ByTitle:
                        q = from p in Context.ProgramActive
                            join m in Context.MovieActive on p.Movie_id equals m.Id
                            where p.Stop > DateTime.Now && !p.Is_deleted && m.Title.Contains(query)
                            select new KeyValuePair<int, string>(p.Id, m.Title);
                        break;
                    case Local.ProgramSearchType.ByMovieType:
                        q = from p in Context.ProgramActive
                            join m in Context.MovieActive on p.Movie_id equals m.Id
                            join mt in Context.MovieType on m.Movietype_id equals mt.Id
                            where p.Stop > DateTime.Now && !p.Is_deleted && mt.Name_long.Contains(query)
                            select new KeyValuePair<int, string>(p.Id, mt.Name_long);
                        break;
                    case Local.ProgramSearchType.ByDirector:
                        q = from p in Context.ProgramActive
                            join e in Context.EpisodeActive on p.Episode_id equals e.Id
                            join ec in Context.EpisodeCast on e.Id equals ec.Episode_id
                            join c in Context.Cast on ec.Cast_id equals c.Id
                            where p.Stop > DateTime.Now && !p.Is_deleted && ec.Role_id == 1 && c.Name.Contains(query)
                            select new KeyValuePair<int, string>(p.Id, c.Name);
                        break;
                    case Local.ProgramSearchType.ByActor:
                        q = from p in Context.ProgramActive
                            join e in Context.EpisodeActive on p.Episode_id equals e.Id
                            join ec in Context.EpisodeCast on e.Id equals ec.Episode_id
                            join c in Context.Cast on ec.Cast_id equals c.Id
                            where p.Stop > DateTime.Now && !p.Is_deleted && ec.Role_id == 2 && c.Name.Contains(query)
                            select new KeyValuePair<int, string>(p.Id, c.Name);
                        break;
                    case Local.ProgramSearchType.ByYear:
                        q = from p in Context.ProgramActive
                            join m in Context.MovieActive on p.Movie_id equals m.Id
                            where p.Stop > DateTime.Now && !p.Is_deleted && m.Year == Convert.ToInt32(query)
                            select new KeyValuePair<int, string>(p.Id, m.Year.ToString());
                        break;
                }

                List<Local.ProgramSearchResult> res = new List<Local.ProgramSearchResult>();
                Dictionary<int, Local.ProgramSearchResult> map = new Dictionary<int, Local.ProgramSearchResult>();

                foreach(var r in q)
                {
                    Local.ProgramSearchResult psr = null;

                    if(map.TryGetValue(r.Key, out psr))
                    {
                        psr.matches.Add(r.Value);
                    }
                    else
                    {
                        psr = new Local.ProgramSearchResult() 
                        {
                            program = GetProgram(r.Key), 
                            matches = new List<string>() { r.Value }
                        };

                        res.Add(psr);

                        map.Add(r.Key, psr);
                    }
                }

                return res;
            }
        }

        public IEnumerable<Local.Program> SearchProgramsByMovie(int movieId)
        {
            using(var Context = CreateDataContext(false))
            {
                var q =
                    from p in Context.ProgramActive
                    join m in Context.MovieActive on p.Movie_id equals m.Id
                    where m.Id == movieId && !p.Is_deleted
                    orderby p.Start
                    select GetProgram(p.Id);

                return q.ToList();
            }
        }

        public IEnumerable<Local.Recording> GetRecordings(Local.RecordingState state = Local.RecordingState.None)
        {
            using(var Context = CreateDataContext(false))
            {
                List<Local.Recording> recordings = new List<Local.Recording>();

                var q =
                    from r in Context.Recording
                    where r.State == (int)state || state == Local.RecordingState.None
                    orderby r.Start
                    select new Local.Recording()
                    {
                        id = r.Id,
                        webId = r.Web_id,
                        name = r.Name,
                        start = r.Start,
                        stop = r.Stop.HasValue ? r.Stop.Value : DateTimeMin,
                        startDelay = new TimeSpan((long)10000000 * r.Start_delay),
                        stopDelay = new TimeSpan((long)10000000 * r.Stop_delay),
                        state = (Local.RecordingState)r.State,
                        preset = r.Preset_id.HasValue ? GetPreset(r.Preset_id.Value) : null,
                        program = r.Program_id.HasValue ? GetProgram(r.Program_id.Value) : null,
                        channelId = r.Channel_id,
                        path = r.Path
                    };

                return q.ToList();
            }
        }

        public Local.Recording GetRecording(Guid recordingId)
        {
            using(var Context = CreateDataContext(false))
            {
                var q =
                    from r in Context.Recording
                    where r.Id == recordingId
                    select new Local.Recording()
                    {
                        id = r.Id,
                        webId = r.Web_id,
                        name = r.Name,
                        start = r.Start,
                        stop = r.Stop.HasValue ? r.Stop.Value : DateTimeMin,
                        startDelay = new TimeSpan((long)10000000 * r.Start_delay),
                        stopDelay = new TimeSpan((long)10000000 * r.Stop_delay),
                        state = (Local.RecordingState)r.State,
                        preset = r.Preset_id.HasValue ? GetPreset(r.Preset_id.Value) : null,
                        program = r.Program_id.HasValue ? GetProgram(r.Program_id.Value) : null,
                        channelId = r.Channel_id,
                        path = r.Path
                    };

                return q.Single();
            }
        }

        public void UpdateRecording(Local.Recording recording)
        {
            using(var Context = CreateDataContext())
            {
                var r = (from row in Context.Recording where row.Id == recording.id select row).SingleOrDefault();

                if(r != null)
                {
                    r.Name = recording.name;
                    r.Start = recording.start;
                    r.Stop = recording.stop;
                    r.Start_delay = (int)recording.startDelay.TotalSeconds;
                    r.Stop_delay = (int)recording.stopDelay.TotalSeconds;

                    if(recording.preset != null && recording.preset.id > 0)
                    {
                        r.Preset_id = recording.preset.id;
                        r.Channel_id = recording.channelId;
                    }
                    else
                    {
                        r.Preset_id = null;
                    }

                    if(r.State == (int)Local.RecordingState.Overlapping)
                    {
                        r.State = (int)Local.RecordingState.Scheduled;
                    }
                }

                Context.SubmitChanges();
            }

            UpdateRecordingState(recording.id);
        }

        public void DeleteRecording(Guid recordingId)
        {
            lock(_recordingPathWatcher)
            {
                using(var Context = CreateDataContext())
                {
                    var r = Context.Recording.Single(row => row.Id == recordingId);

                    if(r.Path != null && File.Exists(r.Path))
                    {
                        File.Delete(r.Path);
                    }

                    if(r.Web_id > 0)
                    {
                        r.State = (int)Local.RecordingState.Deleted;
                    }
                    else
                    {
                        Context.Recording.DeleteOnSubmit(r);
                    }

                    Context.SubmitChanges();
                }
            }

            OnRecordingsChanged();
        }

        public void DeleteMovieRecording(int movieId)
        {
            using(var Context = CreateDataContext())
            {
                foreach(var r in from row in Context.MovieRecording where row.Movie_id == movieId select row)
                {
                    Context.MovieRecording.DeleteOnSubmit(r);
                }

                Context.SubmitChanges();
            }
        }

        private List<Local.WishlistRecording> GetWishlistRecordings()
        {
            using(var Context = CreateDataContext(false))
            {
                List<Local.WishlistRecording> recordings = new List<Local.WishlistRecording>();

                var q =
                    from r in Context.WishlistRecording
                    select new Local.WishlistRecording()
                    {
                        id = r.Id,
                        title = r.Title,
                        description = r.Description,
                        actor = r.Actor,
                        director = r.Director,
                        channelId = r.Channel_id ?? 0
                    };

                return q.ToList();
            }
        }

        private HashSet<int> GetWishlistProgramIds(Local.WishlistRecording rec)
        {
            HashSet<int> ids = null;

            try
            {
                DateTime now = DateTime.Now;

                using(var Context = CreateDataContext())
                {
                    if(rec.channelId > 0)
                    {
                        var q =
                            from p in Context.ProgramActive
                            where p.Start > now && (rec.channelId == 0 || p.Channel_id == rec.channelId) 
                            && p.Channel_id == rec.channelId
                            select p.Id;

                        if(ids == null) ids = new HashSet<int>(q);
                        else ids.IntersectWith(q);

                        if(ids.Count == 0) return ids;
                    }

                    if(rec.title != null && rec.title.Length >= 3)
                    {
                        var q =
                            from p in Context.ProgramActive
                            join m in Context.MovieActive on p.Movie_id equals m.Id
                            where p.Start > now && (rec.channelId == 0 || p.Channel_id == rec.channelId) 
                            && m.Title.Contains(rec.title)
                            select p.Id;

                        if(ids == null) ids = new HashSet<int>(q);
                        else ids.IntersectWith(q);

                        if(ids.Count == 0) return ids;
                    }

                    if(rec.description != null && rec.description.Length >= 3)
                    {
                        var q =
                            from p in Context.ProgramActive
                            join e in Context.EpisodeActive on p.Episode_id equals e.Id
                            join m in Context.MovieActive on p.Movie_id equals m.Id
                            where p.Start > now && (rec.channelId == 0 || p.Channel_id == rec.channelId)
                            && (e.Description.Contains(rec.description) || m.Description.Contains(rec.description))
                            select p.Id;

                        if(ids == null) ids = new HashSet<int>(q);
                        else ids.IntersectWith(q);

                        if(ids.Count == 0) return ids;
                    }

                    if(rec.actor != null && rec.actor.Length >= 3)
                    {
                        var q =
                            from p in Context.ProgramActive
                            join ec in Context.EpisodeCast on p.Episode_id equals ec.Episode_id
                            join c in Context.Cast on ec.Cast_id equals c.Id
                            where p.Start > now && (rec.channelId == 0 || p.Channel_id == rec.channelId)
                            && ec.Role_id == 2 && c.Name.Contains(rec.actor)
                            select p.Id;

                        if(ids == null) ids = new HashSet<int>(q);
                        else ids.IntersectWith(q);

                        if(ids.Count == 0) return ids;
                    }

                    if(rec.director != null && rec.director.Length >= 3)
                    {
                        var q =
                            from p in Context.ProgramActive
                            join ec in Context.EpisodeCast on p.Episode_id equals ec.Episode_id
                            join c in Context.Cast on ec.Cast_id equals c.Id
                            where p.Start > now && (rec.channelId == 0 || p.Channel_id == rec.channelId)
                            && ec.Role_id == 1 && c.Name.Contains(rec.director)
                            select p.Id;

                        if(ids == null) ids = new HashSet<int>(q);
                        else ids.IntersectWith(q);

                        if(ids.Count == 0) return ids;
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }

            if(ids == null)
            {
                ids = new HashSet<int>();
            }

            return ids;
        }

        public List<Local.Program> GetProgramsForWishlist(Local.WishlistRecording rec)
        {
            List<Local.Program> res = new List<Local.Program>();

            foreach(int id in GetWishlistProgramIds(rec))
            {
                res.Add(GetProgram(id));
            }

            res.Sort((a, b) => a.start.CompareTo(b.start));

            return res;
        }

        public Local.WishlistRecording GetWishlistRecording(Guid wishlistRecordingId)
        {
            using(var Context = CreateDataContext(false))
            {
                List<Local.WishlistRecording> recordings = new List<Local.WishlistRecording>();

                var q =
                    from r in Context.WishlistRecording
                    where r.Id == wishlistRecordingId
                    select new Local.WishlistRecording()
                    {
                        id = r.Id,
                        title = r.Title,
                        description = r.Description,
                        actor = r.Actor,
                        director = r.Director,
                        channelId = r.Channel_id ?? 0
                    };

                return q.Single();
            }
        }

        public void DeleteWishlistRecording(Guid wishlistRecordingId)
        {
            using(var Context = CreateDataContext())
            {
                foreach(var r in from row in Context.WishlistRecording where row.Id == wishlistRecordingId select row)
                {
                    Context.WishlistRecording.DeleteOnSubmit(r);
                }

                Context.SubmitChanges();
            }
        }

        public Guid RecordByProgram(int programId, bool warnOnly)
        {
            Local.RecordingState state = warnOnly ? Local.RecordingState.Warning : Local.RecordingState.Scheduled;

            Guid recordingId = Guid.NewGuid();

            using(var Context = CreateDataContext())
            {
                Context.Delete(Context.Recording, 
                    from row in Context.Recording
                    where row.Program_id == programId && row.State == (int)Local.RecordingState.Warning 
                    select row.Id);

                var r = Context.Recording.FirstOrDefault(row => row.Program_id == programId);

                if(r != null)
                {
                    switch(r.State)
                    {
                        case (int)Local.RecordingState.Scheduled:
                        case (int)Local.RecordingState.InProgress:
                        case (int)Local.RecordingState.Prompt:
                            return r.Id;
                        case (int)Local.RecordingState.Warning:
                        case (int)Local.RecordingState.Deleted:
                            r.State = (int)state;
                            Context.SubmitChanges();
                            return r.Id;
                    }
                }

                var p = Context.Program.SingleOrDefault(row => row.Id == programId);

                if(p == null)
                {
                    throw new FaultException("invalid programId");
                }

                if(p.Stop <= DateTime.Now)
                {
                    throw new FaultException("Program.Stop must be in the future");
                }

                r = new Data.Recording()
                {
                    Id = recordingId,
                    Name = "",
                    Start = p.Start,
                    Stop = p.Stop,
                    State = (int)state,
                    Preset_id = null,
                    Program_id = p.Id,
                    Channel_id = p.Channel_id,
                    Start_delay = Service.Start_delay,
                    Stop_delay = Service.Stop_delay
                };

                Context.Recording.InsertOnSubmit(r);

                Context.SubmitChanges();
            }

            Log.WriteLine("RecordByProgram (programId = {0}, recordingId = {1}, warnOnly = {2})", programId, recordingId, warnOnly);

            UpdateRecordingState(recordingId);

            OnRecordingsChanged();

            return recordingId;
        }

        public Guid RecordByPreset(int presetId, DateTime start, DateTime? stop)
        {
            Guid recordingId = Guid.NewGuid();

            using(var Context = CreateDataContext())
            {
                var p = Context.Preset.SingleOrDefault(row => row.Id == presetId);

                if(p == null)
                {
                    throw new FaultException("invalid presetId");
                }

                if(stop.HasValue && stop <= DateTime.Now)
                {
                    throw new FaultException("Recording.Stop must be in the future");
                }

                var r = new Data.Recording()
                {
                    Id = recordingId,
                    Name = "",
                    Start = start,
                    Stop = stop,
                    State = (int)(stop.HasValue ? Local.RecordingState.Scheduled : Local.RecordingState.Prompt),
                    Preset_id = p.Id,
                    Program_id = null,
                    Channel_id = p.Channel_id,
                    Start_delay = Service.Start_delay,
                    Stop_delay = Service.Stop_delay
                };

                Context.Recording.InsertOnSubmit(r);

                Context.SubmitChanges();
            }

            Log.WriteLine("RecordByPreset (presetId = {0}, recordingId = {1})", presetId, recordingId);

            UpdateRecordingState(recordingId);

            OnRecordingsChanged();

            return recordingId;
        }

        public Guid RecordByMovie(int movieId, int channelId)
        {
            Guid movieRecordingId = Guid.NewGuid();

            List<int> programIdsToRecord = null;

            using(var Context = CreateDataContext())
            {
                var r = Context.MovieRecording.FirstOrDefault(row => row.Movie_id == movieId && (row.Channel_id == channelId || row.Channel_id == 0));

                if(r != null)
                {
                    movieRecordingId = r.Id;
                }
                else
                {
                    if(channelId == 0)
                    {
                        Context.Delete(Context.MovieRecording, from row in Context.MovieRecording where row.Movie_id == movieId select row.Id);
                    }

                    if(!Context.MovieActive.Any(row => row.Id == movieId))
                    {
                        throw new FaultException("invalid movieId");
                    }

                    Context.MovieRecording.InsertOnSubmit(new Homesys.Data.MovieRecording()
                    {
                        Id = movieRecordingId,
                        Movie_id = movieId,
                        Channel_id = channelId
                    });

                    Context.SubmitChanges();
                }

                var q =
                    from p in Context.ProgramActive
                    from rec in (from row in Context.Recording where row.Program_id == p.Id select row).DefaultIfEmpty()
                    where p.Stop > DateTime.Now && !p.Is_deleted && (p.Channel_id == channelId || channelId == 0)
                    && p.Movie_id == movieId
                    && rec == null
                    orderby p.Start, p.Stop
                    select p.Id;

                programIdsToRecord = q.ToList();
            }

            Log.WriteLine("RecordByMovie (movieId = {0}, channelId = {1}, movieRecordingId = {2})", movieId, channelId, movieRecordingId);

            foreach(int programId in programIdsToRecord)
            {
                try
                {
                    RecordByProgram(programId, false);
                }
                catch(Exception e)
                {
                    Log.WriteLine(e);
                }
            }

            return movieRecordingId;
        }

        public Guid RecordByWishlist(Local.WishlistRecording rec)
        {
            try
            {
                using(var Context = CreateDataContext())
                {
                    bool valid = false;

                    Data.WishlistRecording row = new Data.WishlistRecording();

                    row.Id = Guid.NewGuid();

                    if(rec.title != null && rec.title.Length >= 3) { row.Title = rec.title; valid = true; }
                    if(rec.description != null && rec.description.Length >= 3) { row.Description = rec.description; valid = true; }
                    if(rec.actor != null && rec.actor.Length >= 3) { row.Actor = rec.actor; valid = true; }
                    if(rec.director != null && rec.director.Length >= 3) { row.Director = rec.director; valid = true; }
                    if(rec.channelId > 0) { row.Channel_id = rec.channelId; }

                    if(valid)
                    {
                        Context.WishlistRecording.InsertOnSubmit(row);

                        Context.SubmitChanges();

                        foreach(int id in GetWishlistProgramIds(rec))
                        {
                            RecordByProgram(id, false);
                        }

                        return row.Id;
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }

            return Guid.Empty;
        }

        internal void RecordByWishlist(HashSet<int> newProgramIds)
        {
            try
            {
                using(var Context = CreateDataContext(false))
                {
                    foreach(Local.WishlistRecording rec in WishlistRecordings)
                    {
                        foreach(int id in GetWishlistProgramIds(rec))
                        {
                            if(newProgramIds.Contains(id))
                            {
                                RecordByProgram(id, false);
                            }
                        }
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        public Guid Record(Guid tunerId)
        {
            lock(_sync)
            {
				Guid id = Guid.Empty;

				try
				{
					Tuner t;

					if(!_tuners.TryGetValue(tunerId, out t))
					{
                        throw new FaultException("Invalid tunerId");
					}

					if(t.IsRecording())
					{
                        throw new FaultException("Tuner is already recording");
					}

                    Local.Preset preset = GetCurrentPreset(tunerId);

					if(preset == null || preset.id == 0)
					{
						throw new FaultException("Tuner preset not set");
					}

					id = RecordByPreset(preset.id, DateTime.Now, null);

					t.StartRecording(RecordingPath, RecordingFormat, preset.name, id);

					Log.WriteLine("Recording (tunerId = {0}, presetId = {1}, recordingId = {2})", tunerId, preset.id, id);

					return id;
				}
				catch(Exception e)
				{
					Log.WriteLine(e);
				}

                if(id != Guid.Empty)
                {
                    DeleteRecording(id);
                }

				return Guid.Empty;
            }
        }

        public Guid IsRecording(Guid tunerId)
        {
            lock(_sync)
            {
				Tuner t;

				if(_tuners.TryGetValue(tunerId, out t))
				{
					if(t.IsRecording())
					{
						return t.GetRecordingId();
					}
				}

				return Guid.Empty;
            }
        }

        public void StopRecording(Guid tunerId)
        {
            StopRecording(tunerId, Local.RecordingState.Aborted);
        }

        public void StopRecording(Guid tunerId, Local.RecordingState state)
		{
            lock(_sync)
            {
                Tuner t;

                if(_tuners.TryGetValue(tunerId, out t) && t.IsRecording())
                {
                    t.StopRecording();

                    UpdateRecording(t.GetRecordingId(), state, t.GetRecordingPath());

                    Log.WriteLine("Recording aborted (tunerId = {0}, recordingId = {1})", tunerId, t.GetRecordingId());
                }
            }
		}

        public Local.TimeshiftFile Activate(Guid tunerId)
        {
            lock(_sync)
            {
				Tuner t = null;

				if(!_tuners.TryGetValue(tunerId, out t))
				{
                    Local.TunerReg tr = GetTuner(tunerId);

					if(tr != null)
					{
   						t = new Tuner();

                        try
                        {
                            if(_tuners.Count == 0 && _cardserver == null)
                            {
                                _cardserver = new SmartCardServer();

                                _cardserver.GetSmartCardDevices(); // detect cards
                            }

                            t.Open(tr, tr.id, _cardserver);

                            _tuners[tunerId] = t;

                            int presetId = GetCurrentPresetId(tunerId);

                            if(presetId > 0)
                            {
                                SetPreset(tunerId, presetId);
                            }

                            Log.WriteLine("Tuner activated (tunerId = {0})", tunerId);
                        }
                        catch(Exception e)
                        {
                            Log.WriteLine(e);

                            _tuners.Remove(tunerId);

                            t.Dispose();

                            t = null;
                        }
                        finally
                        {
                            if(_tuners.Count == 0 && _cardserver != null)
                            {
                                _cardserver.Dispose();

                                _cardserver = null;
                            }
                        }
					}
				}

				if(t != null)
				{
					t.ResetIdleTime();

                    Local.TimeshiftFile tf = new Local.TimeshiftFile();

                    tf.path = t.GetFileName();
                    tf.port = t.GetPort();
                    tf.version = t.GetFileVersion();

                    return tf;
				}

				return null;
            }
        }

        public void Deactivate(Guid tunerId)
        {
            lock(_sync)
            {
                try
                {
                    Tuner t;

                    if(_tuners.TryGetValue(tunerId, out t))
                    {
                        Log.WriteLine("Tuner deactivating (tunerId = {0})", tunerId);

                        StopRecording(tunerId);

                        _tuners.Remove(tunerId);

                        t.Dispose();

                        Log.WriteLine("Tuner deactivated (tunerId = {0})", tunerId);
                    }
                }
                catch(Exception e)
                {
                    Log.WriteLine(e);
                }
                finally
                {
                    if(_tuners.Count == 0 && _cardserver != null)
                    {
                        _cardserver.Dispose();

                        _cardserver = null;

                        Log.WriteLine("Card server deactivated");
                    }
                }
            }
        }

        public void DeactivateAll()
        {
            lock(_sync)
            {
                List<Guid> ids = new List<Guid>(_tuners.Keys);
                
				foreach(Guid id in ids)
				{
					Deactivate(id);
				}
            }
        }

        public void SetPrimaryTuner(Guid tunerId, bool active)
        {
            lock(_sync)
            {
                _tunerId = tunerId;
            }

            try
            {
                using(TransactionScope ts = new TransactionScope())
                {
                    using(var Context = CreateDataContext())
                    {
                        DateTime now = DateTime.Now;

                        var q =
                            from t in Context.Tuner
                            join pre in Context.Preset on t.Preset_id equals pre.Id
                            join p in Context.ProgramActive on pre.Channel_id equals p.Channel_id
                            where t.Id == tunerId && !p.Is_deleted && p.Start <= now && now < p.Stop
                            select new { p.Id, p.Channel_id };

                        var r = q.FirstOrDefault();

                        if(r != null)
                        {
                            Data.TvStat s = Context.TvStat.FirstOrDefault(row => row.Active && row.Channel_id == r.Channel_id && row.Program_id == r.Id);

                            if(s != null)
                            {
                                s.Stop = now;
                            }
                            else
                            {
                                foreach(var row in Context.TvStat.Where(row => row.Active))
                                {
                                    row.Active = false;
                                }

                                Context.TvStat.InsertOnSubmit(new Data.TvStat()
                                {
                                    Channel_id = r.Channel_id,
                                    Program_id = r.Id,
                                    Start = now,
                                    Stop = now,
                                    Active = true
                                });
                            }

                            Context.SubmitChanges();
                        }
                    }

                    ts.Complete();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        public void UpdateGuideNow(bool reset)
        {
            lock(_sync)
            {
                // _updateStationNow = true;

                _guide.UpdateNow(reset);
            }
        }

        public Local.ServiceMessage PopServiceMessage()
        {
            lock(_sync)
            {
                Local.ServiceMessage sm = null;

                try
                {
                    using(var Context = CreateDataContext())
                    {
                        var r =
                            from row in Context.ServiceMessage
                            where row.Valid_until > DateTime.Now && !row.Deleted
                            orderby row.Post_date
                            select row;

                        Data.ServiceMessage m = r.FirstOrDefault();

                        if(m != null)
                        {
                            sm = new Local.ServiceMessage() { id = m.Id, message = m.Message };

                            Context.ServiceMessage.DeleteOnSubmit(m);
                        }

                        Context.SubmitChanges();
                    }
                }
                catch(Exception e)
                {
                    Log.WriteLine(e);
                }

                return sm;
            }
        }

        static Local.Movie Translate(Web.Movie r)
        {
            return new Local.Movie()
            {
                id = r.Id,
                title = r.Title,
                desc = r.Description,
                rating = r.Rating,
                year = r.Year,
                episodeCount = r.EpisodeCount
                // TODO:  = r.MovieTypeId,
                // TODO: dvbCategory = r.DVBCategoryId
                // TODO: favorite = 
            };
        }

        public IEnumerable<Local.TuneRequestPackage> GetTuneRequestPackages(Guid tunerId)
        {
            List<Local.TuneRequestPackage> packages = new List<Local.TuneRequestPackage>();

            Local.TunerReg tr = GetTuner(tunerId);

            Local.TunerDeviceType type = tr.type;

            if(tr.dispname.Contains(@"vid_0572&pid_88e6"))
            {
                type = Local.TunerDeviceType.DVBC;
            }

            using(var Context = CreateDataContext(false))
            {
                var q =
                    from p in Context.TuneRequestPackage
                    where p.Type == (int)tr.type || p.Type == (int)type
                    orderby p.Type, p.Provider == "", p.Name, p.Provider
                    select new Local.TuneRequestPackage
                    {
                        id = p.Id,
                        provider = p.Provider,
                        name = p.Name
                    };

                return q.ToList();
            }
        }

        void TunerScanThread(object o)
        {
			Local.TunerReg tr = GetTuner(_scan.tunerId);

            if(tr != null)
            {
                Tuner t = new Tuner();

                try
                {
                    t.Open(tr, tr.id, null);

                    _scan.tuner = t;

                    Dictionary<ulong, bool> presets = new Dictionary<ulong, bool>();

                    int enabled = 0;

                    foreach(Local.TuneRequest r in _scan.reqs)
                    {
                        if(tr.type == Local.TunerDeviceType.DVBF)
                        {
                            Log.WriteLine("{0}", r.path);
                        }
                        else
                        {
                            Log.WriteLine("{0} KHz", r.freq);
                        }

                        t.Tune(r, -1);

                        foreach(TunerProgram p in t.GetPrograms())
                        {
                            Log.WriteLine("[{0}] {1} / {2} {3}", p.sid, p.provider, p.name, p.scrambled ? "*" : "");

                            Local.Preset preset = new Local.Preset();

                            preset.provider = p.provider;
                            preset.name = p.name;
                            preset.scrambled = p.scrambled;
                            preset.tunereq = r.Clone();
                            preset.tunereq.sid = p.sid;

                            ulong hash = (ulong)preset.tunereq.sid << 32;

                            if(tr.type == Local.TunerDeviceType.DVBF)
                            {
                                hash |= (ulong)(uint)preset.tunereq.path.GetHashCode();
                            }
                            else
                            {
                                hash |= (ulong)(uint)preset.tunereq.freq << 4;
                                hash |= (ulong)(uint)preset.tunereq.lnbSource;
                            }

                            if(presets.ContainsKey(hash))
                            {
                                continue;
                            }

                            presets.Add(hash, true);

                            lock(_scan)
                            {
                                if(_scan.provider == null || _scan.provider.Length == 0 || _scan.provider == p.provider)
                                {
                                    preset.enabled = 1;

                                    enabled++;
                                }
                                
                                _scan.presets.Add(preset);
                            }
                        }

                        if(_scan.abort)
                        {
                            break;
                        }
                    }

                    if(enabled == 0)
                    {
                        lock(_scan)
                        {
                            foreach(var p in _scan.presets)
                            {
                                p.enabled = 1;
                            }
                        }
                    }
                }
                catch(Exception e)
                {
                    Log.WriteLine(e);
                }
                finally
                {
                    _scan.tuner = null;

                    t.Dispose();
                }
            }

            _scan.done = true;
        }

        public void StartTunerScanByPackage(Guid tunerId, int id)
        {
            StopTunerScan();

            Deactivate(tunerId);

            _scan.tunerId = tunerId;
            _scan.presets = new List<Local.Preset>();
            _scan.reqs = new List<Local.TuneRequest>();
			_scan.abort = false;
			_scan.done = false;

            Local.TunerReg tr = GetTuner(tunerId);
            Local.TunerConnector tc = tr.connectors.FirstOrDefault(c => c.type == PhysConn_Video_Tuner);

            using(var Context = CreateDataContext(false))
            {
                _scan.provider = (from p in Context.TuneRequestPackage where p.Id == id select p.Provider).SingleOrDefault();

                var q =
                    from p in Context.TuneRequest
                    where p.Package_id == id
                    orderby p.Channel_id, p.Freq, p.Modulation
                    select p;

                foreach(var r in q)
                {
                    Local.TuneRequest req = new Local.TuneRequest 
                    {
                        freq = r.Freq,
                        standard = r.Standard,
                        connector = tc,
                        ifec = r.Ifec,
                        ifecRate = r.Ifecrate,
                        ofec = r.Ofec,
                        ofecRate = r.Ofecrate,
                        modulation = r.Modulation,
                        symbolRate = r.Symbol_rate,
                        polarisation = r.Polarisation,
                        lnbSource = r.Lnb_source,
                        spectralInversion = r.Spectral_inversion,
                        bandwidth = r.Bandwidth,
                        lpifec = r.Lpifec,
                        lpifecRate = r.Lpifecrate,
                        halpha = r.Halpha,
                        guard = r.Guard,
                        transmissionMode = r.Transmission_mode,
                        otherFreqInUse = r.Other_freq_in_use,
                        path = r.Path,
                    };

                    if(tc != null)
                    {
                        req.connector = tc;
                    }

                    if(r.Channel_id > 0)
                    {
                        Data.Channel channel = Context.Channel.Where(c => c.Id == r.Channel_id).SingleOrDefault();

                        Local.Preset p = new Local.Preset
                        {
                            channelId = r.Channel_id,
                            name = channel != null ? channel.Name : "",
                            scrambled = false,
                            enabled = 1,
                            tunereq = req,
                        };

                        _scan.presets.Add(p);
                    }
                    else
                    {
                        _scan.reqs.Add(req);
                    }
                }
            }

            _scan.presets.AddRange(
                from c in tr.connectors
                where c.type != PhysConn_Video_Tuner
                select new Local.Preset
                {
                    channelId = 0,
                    name = c.name,
                    scrambled = false,
                    enabled = 2,
                    tunereq = new Local.TuneRequest
                    {
                        freq = 0,
                        standard = AnalogVideo_PAL_B,
                        connector = new Local.TunerConnector { id = c.id }
                    }
                });

            _scan.thread = new System.Threading.Thread(TunerScanThread);
            _scan.thread.SetApartmentState(ApartmentState.MTA);
            _scan.thread.Start();
        }

        public void StartTunerScan(Guid tunerId, List<Local.TuneRequest> reqs, string provider)
        {
            StopTunerScan();

            Deactivate(tunerId);

            _scan.tunerId = tunerId;
            _scan.provider = provider;
            _scan.reqs = reqs;
			_scan.presets.Clear();
			_scan.abort = false;
			_scan.done = false;
            _scan.thread = new System.Threading.Thread(TunerScanThread);
            _scan.thread.SetApartmentState(ApartmentState.MTA);
            _scan.thread.Start();
        }

        public void StopTunerScan()
        {
            _scan.abort = true;

            if(_scan.thread != null)
            {
                _scan.thread.Join();
                _scan.thread = null;
            }

            _scan.done = true;
        }

        public IEnumerable<Local.Preset> GetTunerScanResult()
        {
            lock(_scan)
            {
                return new List<Local.Preset>(_scan.presets);
            }
        }

        public bool IsTunerScanDone()
        {
            return _scan.done;
        }

        public void SaveTunerScanResult(Guid tunerId, List<Local.Preset> presets, bool append)
        {
            Local.TunerReg t = GetTuner(tunerId);

            using(TransactionScope ts = new TransactionScope())
            {
                using(var Context = CreateDataContext())
                {
                    if(!append)
                    {
                        Context.Delete(Context.Preset, from row in Context.Preset where row.Tuner_id == tunerId select row.Id);
                    }

                    var q = from row in Context.Preset where row.Tuner_id == tunerId orderby row.Enabled descending select row.Enabled;

                    int num = q.FirstOrDefault();

                    foreach(Local.Preset p in presets)
                    {
                        Log.WriteLine(" {0}", p.name);

                        if(append)
                        {
                            if(t.type == Local.TunerDeviceType.DVBF)
                            {
                                if(Context.Preset.Count(r => r.Tuner_id == tunerId && r.Sid == p.tunereq.sid && r.Path == p.tunereq.path) > 0)
                                {
                                    Log.WriteLine("dup!");

                                    continue;
                                }
                            }
                            else
                            {
                                if(Context.Preset.Count(r => r.Tuner_id == tunerId && r.Sid == p.tunereq.sid && r.Freq == p.tunereq.freq) > 0)
                                {
                                    Log.WriteLine("dup!");

                                    continue;
                                }
                            }
                        }

                        Context.Preset.InsertOnSubmit(new Data.Preset()
                        {
                            Tuner_id = tunerId,
                            Channel_id = p.channelId,
                            Scrambled = p.scrambled,
                            Enabled = ++num,
                            Provider = p.provider,
                            Name = p.name,
                            Freq = p.tunereq.freq,
                            Standard = p.tunereq.standard,
                            Connector_id = p.tunereq.connector != null && p.tunereq.connector.id > 0 ? p.tunereq.connector.id : t.connectors.First().id,
                            Sid = p.tunereq.sid,
                            Ifec = p.tunereq.ifec,
                            Ifecrate = p.tunereq.ifecRate,
                            Ofec = p.tunereq.ofec,
                            Ofecrate = p.tunereq.ofecRate,
                            Modulation = p.tunereq.modulation,
                            Symbol_rate = p.tunereq.symbolRate,
                            Polarisation = p.tunereq.polarisation,
                            West = p.tunereq.west,
                            Orbital_position = p.tunereq.orbitalPosition,
                            Azimuth = p.tunereq.azimuth,
                            Elevation = p.tunereq.elevation,
                            Lnb_source = p.tunereq.lnbSource,
                            Low_oscillator = p.tunereq.lowOscillator,
                            High_oscillator = p.tunereq.highOscillator,
                            Spectral_inversion = p.tunereq.spectralInversion,
                            Bandwidth = p.tunereq.bandwidth,
                            Lpifec = p.tunereq.lpifec,
                            Lpifecrate = p.tunereq.lpifecRate,
                            Halpha = p.tunereq.halpha,
                            Guard = p.tunereq.guard,
                            Transmission_mode = p.tunereq.transmissionMode,
                            Other_freq_in_use = p.tunereq.otherFreqInUse,
                            Path = p.tunereq.path,
                        });
                    }

                    Context.SubmitChanges();

                    Data.Tuner tuner = (from row in Context.Tuner where row.Id == tunerId select row).SingleOrDefault();
                    Data.Preset preset = (from row in Context.Preset where row.Tuner_id == tunerId orderby row.Id select row).FirstOrDefault();

                    if(tuner != null && preset != null)
                    {
                        tuner.Preset_id = preset.Id;
                    }

                    Context.SubmitChanges();
                }

                ts.Complete();
            }

            _guide.UpdateChannels();

            _guide.UpdateActivePrograms();

            lock(_presets)
            {
                _presets.Clear();
            }
        }

        static string HttpPost(string url, List<string> sl)
        {
            string content = string.Join("&", sl.ToArray());

            string res = null;

            try
            {
                HttpWebRequest request = (HttpWebRequest)WebRequest.Create(url);

                request.Timeout = 10000;

                request.Method = "POST";
                request.ContentType = "application/x-www-form-urlencoded";
                request.ContentLength = content.Length;

                using(Stream s = request.GetRequestStream())
                {
                    ASCIIEncoding encoding = new ASCIIEncoding();
                    byte[] bytes = encoding.GetBytes(content);
                    s.Write(bytes, 0, bytes.Length);
                }

                using(HttpWebResponse response = (HttpWebResponse)request.GetResponse())
                {
                    using(Stream s = response.GetResponseStream())
                    {
                        using(StreamReader r = new StreamReader(s, Encoding.UTF8))
                        {
                            res = r.ReadToEnd();
                        }
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e.Message);
            }

            return res;
        }

        DateTime _lastMousePositionTime = DateTime.Now;
        object _lastMousePositionSync = new object();

        public void DispatchUserInput(Local.UserInputType type, int param)
        {
            if(type == Local.UserInputType.MousePosition)
            {
                lock(_lastMousePositionSync)
                {
                    DateTime now = DateTime.Now;

                    TimeSpan ts = now - _lastMousePositionTime;

                    if(ts < new TimeSpan(0, 0, 0, 0, 10))
                    {
                        // Log.WriteLine("skipped {0}", ts);

                        return;
                    }

                    // Log.WriteLine("mouse");

                    _lastMousePositionTime = now;
                }
            }

            CallbackService svc = (CallbackService)_callback.SingletonInstance;

            svc.Dispatch(delegate(Local.ICallback cb) { cb.OnUserInput((int)type, param); });
        }

        #endregion
    }

    [ServiceBehavior(InstanceContextMode = InstanceContextMode.Single)]
    public partial class CallbackService : Local.ICallbackService
    {
        Dictionary<long, Local.ICallback> _clients;

        public CallbackService()
        {
            _clients = new Dictionary<long, Local.ICallback>();
        }

        public static ServiceHost Create()
        {
            CallbackService svc = new CallbackService();

            ServiceHost serviceHost = new ServiceHost(svc, new Uri("http://" + Environment.MachineName + ":24936/"));

            #if DEBUG

            ServiceMetadataBehavior metadataBehavior = serviceHost.Description.Behaviors.Find<ServiceMetadataBehavior>();

            if(metadataBehavior != null)
            {
                metadataBehavior.HttpGetEnabled = true;
            }
            else
            {
                metadataBehavior = new ServiceMetadataBehavior();
                metadataBehavior.HttpGetEnabled = true;
                serviceHost.Description.Behaviors.Add(metadataBehavior);
            }

            #endif

            ServiceDebugBehavior debugBehavior = serviceHost.Description.Behaviors.Find<ServiceDebugBehavior>();

            if(debugBehavior != null)
            {
                debugBehavior.IncludeExceptionDetailInFaults = true;
            }
            else
            {
                debugBehavior = new ServiceDebugBehavior();
                debugBehavior.IncludeExceptionDetailInFaults = true;
                serviceHost.Description.Behaviors.Add(debugBehavior);
            }

            NetNamedPipeBinding netNamedPipeBinding = new NetNamedPipeBinding();

            serviceHost.AddServiceEndpoint(typeof(Local.ICallbackService), netNamedPipeBinding, "net.pipe://localhost/homesys/service/callback");

            return serviceHost;
        }

        public delegate void DispatchDelegate(Local.ICallback cb);

        public void Dispatch(DispatchDelegate d)
        {
            lock(_clients)
            {
                List<long> failed = new List<long>();

                foreach(var pair in _clients)
                {
                    try
                    {
                        d(pair.Value);
                    }
                    catch(Exception e)
                    {
                        Log.WriteLine(e);

                        failed.Add(pair.Key);
                    }
                }

                foreach(long id in failed)
                {
                    _clients.Remove(id);
                }
            }
        }

        #region ICallback members

        public void Register(long id)
        {
            try
            {
                lock(_clients)
                {
                    
                    _clients[id] = OperationContext.Current.GetCallbackChannel<Local.ICallback>();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        public void Unregister(long id)
        {
            try
            {
                lock(_clients)
                {
                    _clients.Remove(id);
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        #endregion
    }
}
