using System;
using System.Collections.Generic;
using System.Text;
using System.ServiceModel;
using System.Runtime.Serialization;
using System.ServiceModel.Web;

namespace Homesys.Local
{
    [ServiceContract]
    public interface IService
    {
        MachineReg Machine
        {
            [OperationContract]
            get;
        }

        bool IsOnline
        {
            [OperationContract]
            get;
        }

        string Username
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        string Password
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        string Language
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        string Region
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        string RecordingPath
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        string RecordingFormat
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        string TimeshiftPath
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        TimeSpan TimeshiftDuration
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        string DownloadPath
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        IEnumerable<TunerDevice> AllTuners
        {
            [OperationContract]
            get;
        }

        IEnumerable<TunerReg> Tuners
        {
            [OperationContract]
            get;
        }

        IEnumerable<SmartCardDevice> SmartCards
        {
            [OperationContract]
            get;
        }

        IEnumerable<Channel> Channels
        {
            [OperationContract]
            get;
        }

        IEnumerable<Recording> Recordings
        {
            [OperationContract]
            get;
        }

        IEnumerable<WishlistRecording> WishlistRecordings
        {
            [OperationContract]
            get;
        }

        ParentalSettings Parental
        {
            [OperationContract]
            get;

            [OperationContract]
            set;
        }

        AppVersion MostRecentVersion
        {
            [OperationContract]
            get;
        }

        UInt64 InstalledVersion
        {
            [OperationContract]
            get;
        }

        DateTime LastGuideUpdate
        {
            [OperationContract]
            get;
        }

        string LastGuideError
        {
            [OperationContract]
            get;
        }

        int GuideUpdatePercent
        {
            [OperationContract]
            get;
        }

        [OperationContract]
        void RegisterUser(string username, string password, string email);

        [OperationContract]
        bool UserExists(string username);

        [OperationContract]
        bool LoginExists(string username, string password);

        [OperationContract]
        void RegisterMachine(string name, List<TunerDevice> dev);

        [OperationContract]
        void SetUserInfo(UserInfo info);

        [OperationContract]
        UserInfo GetUserInfo();

        [OperationContract]
        bool GoOnline();

        [OperationContract]
        TunerReg GetTuner(Guid tunerId);

        [OperationContract]
        IEnumerable<TunerConnector> GetTunerConnectors(Guid tunerId);

        [OperationContract]
        TunerStat GetTunerStat(Guid tunerId);

        [OperationContract]
        IEnumerable<Preset> GetPresets(Guid tunerId, bool enabled);

        [OperationContract]
        void SetPresets(Guid tunerId, List<int> presetIds);

        [OperationContract]
        Preset GetPreset(int presetId);

        [OperationContract]
        Preset GetCurrentPreset(Guid tunerId);

        [OperationContract]
        int GetCurrentPresetId(Guid tunerId);

        [OperationContract]
        void SetPreset(Guid tunerId, int presetId);

        [OperationContract]
        void UpdatePreset(Preset preset);

        [OperationContract]
        void UpdatePresets(List<Local.Preset> presets);

        [OperationContract]
        Preset CreatePreset(Guid tunerId);

        [OperationContract]
        void DeletePreset(int presetId);

        [OperationContract]
        IEnumerable<Channel> GetChannels(Guid tunerId);

        [OperationContract]
        Channel GetChannel(int channelId);

        [OperationContract]
        IEnumerable<Program> GetPrograms(Guid tunerId, int channelId, DateTime start, DateTime stop);

        [OperationContract]
        Program GetProgram(int programId);

        [OperationContract]
        IEnumerable<Program> GetFavoritePrograms(int count, bool picture);

        [OperationContract]
        IEnumerable<Program> GetRelatedPrograms(int programId);

        [OperationContract]
        IEnumerable<MovieType> GetRecommendedMovieTypes();

        [OperationContract]
        IEnumerable<Program> GetRecommendedProgramsByMovieType(int movieTypeId);

        [OperationContract]
        IEnumerable<Program> GetRecommendedProgramsByChannel(int channelId);

        [OperationContract]
        Episode GetEpisode(int episodeId);

        [OperationContract]
        Movie GetMovie(int movieId);

        [OperationContract]
        IEnumerable<ProgramSearchResult> SearchPrograms(string query, ProgramSearchType type);

        [OperationContract]
        IEnumerable<Program> SearchProgramsByMovie(int movieId);

        [OperationContract]
        IEnumerable<Recording> GetRecordings(RecordingState state);

        [OperationContract]
        Recording GetRecording(Guid recordingId);

        [OperationContract]
        void UpdateRecording(Recording recording);

        [OperationContract]
        void DeleteRecording(Guid recordingId);

        [OperationContract]
        void DeleteMovieRecording(int movieId);

        [OperationContract]
        List<Program> GetProgramsForWishlist(WishlistRecording r);

        [OperationContract]
        WishlistRecording GetWishlistRecording(Guid wishlistRecordingId);

        [OperationContract]
        void DeleteWishlistRecording(Guid wishlistRecordingId);

        [OperationContract]
        Guid RecordByProgram(int programId, bool warnOnly);

        [OperationContract]
        Guid RecordByPreset(int presetId, DateTime start, DateTime? stop);

        [OperationContract]
        Guid RecordByMovie(int movieId, int channelId);

        [OperationContract]
        Guid RecordByWishlist(WishlistRecording r);

        [OperationContract]
        Guid Record(Guid tunerId);

        [OperationContract]
        Guid IsRecording(Guid tunerId);

        [OperationContract]
        void StopRecording(Guid tunerId);

        [OperationContract]
        TimeshiftFile Activate(Guid tunerId);

        [OperationContract]
        void Deactivate(Guid tunerId);

        [OperationContract]
        void DeactivateAll();

        [OperationContract(IsOneWay = true)]
        void SetPrimaryTuner(Guid tunerId, bool active);

        [OperationContract(IsOneWay = true)]
        void UpdateGuideNow(bool reset);

        [OperationContract]
        ServiceMessage PopServiceMessage();

        [OperationContract]
        IEnumerable<TuneRequestPackage> GetTuneRequestPackages(Guid tunerId);

        [OperationContract]
        void StartTunerScanByPackage(Guid tunerId, int id);

        [OperationContract]
        void StartTunerScan(Guid tunerId, List<TuneRequest> trs, string provider);

        [OperationContract]
        void StopTunerScan();

        [OperationContract]
        IEnumerable<Preset> GetTunerScanResult();

        [OperationContract]
        bool IsTunerScanDone();

        [OperationContract]
        void SaveTunerScanResult(Guid tunerId, List<Preset> presets, bool append);

        [OperationContract]
        void DispatchUserInput(UserInputType type, int param);
    }

    [DataContract]
    public class IntKey
    {
        [DataMember]
        public int id;
    }

    [DataContract]
    public class MachineReg
    {
        [DataMember]
        public Guid id;

        public Guid webId;

        [DataMember]
        public string name;

        [DataMember]
        public bool published;

        [DataMember]
        public string recordingPath;

        [DataMember]
        public string recordingFormat;

        [DataMember]
        public string downloadPath;

        [DataMember]
        public string timeshiftPath;

        [DataMember]
        public TimeSpan timeshiftDur;

        [DataMember]
        public string username;

        [DataMember]
        public string password;
    }

    [DataContract]
    public class TunerReg : TunerDevice
    {
        [DataMember]
        public Guid id;

        public Guid webId;

        [DataMember]
        public List<TunerConnector> connectors;
    }

    [DataContract]
    public class Preset : IntKey
    {
        [DataMember]
        public int channelId;

        [DataMember]
        public string provider;

        [DataMember]
        public string name;

        [DataMember]
        public bool scrambled;

        [DataMember]
        public int enabled;

        [DataMember]
        public int rating;

        [DataMember]
        public TuneRequest tunereq;
    }

    [DataContract]
    public enum RecordingState
    {
        [EnumMember]
        None = -1,

        [EnumMember]
        NotScheduled,

        [EnumMember]
        Scheduled,

        [EnumMember]
        InProgress,

        [EnumMember]
        Aborted,

        [EnumMember]
        Missed,

        [EnumMember]
        Error,

        [EnumMember]
        Finished,

        [EnumMember]
        Overlapping,

        [EnumMember]
        Prompt,

        [EnumMember]
        Deleted,

        [EnumMember]
        Warning,
    }

    public class RecordingStateString
    {
        public const string NotScheduled = "NotScheduled";
        public const string Scheduled = "Scheduled";
        public const string InProgress = "InProgress";
        public const string Aborted = "Aborted";
        public const string Missed = "Missed";
        public const string Error = "Error";
        public const string Finished = "Finished";
        public const string Overlapping = "Overlapping";
        public const string Prompt = "Prompt";
        public const string Deleted = "Deleted";
        public const string Warning = "Warning";
    }

    [DataContract]
    public class Channel : IntKey
    {
        [DataMember]
        public string name;

        [DataMember]
        public byte[] logo;

        [DataMember]
        public bool radio;

        [DataMember]
        public bool hasProgram;

        [DataMember]
        public int rank;
    }

    [DataContract]
    public class Cast : IntKey
    {
        [DataMember]
        public string name;

        [DataMember]
        public string movieName;
    }

    [DataContract]
    public class Program : IntKey
    {
        [DataMember]
        public int channelId;

        [DataMember]
        public DateTime start;

        [DataMember]
        public DateTime stop;

        [DataMember]
        public RecordingState state;

        [DataMember]
        public Episode episode;
    }

    [DataContract]
    public class Episode : IntKey
    {
        [DataMember]
        public string title;

        [DataMember]
        public string desc;

        [DataMember]
        public int episodeNumber;

        [DataMember]
        public Movie movie;

        [DataMember]
        public List<Cast> directors;

        [DataMember]
        public List<Cast> actors;
    }

    [DataContract]
    public class Movie : IntKey
    {
        [DataMember]
        public string title;

        [DataMember]
        public string desc;

        [DataMember]
        public int rating;

        [DataMember]
        public int year;

        [DataMember]
        public int episodeCount;

        [DataMember]
        public string movieTypeShort;

        [DataMember]
        public string movieTypeLong;

        [DataMember]
        public string dvbCategory;

        [DataMember]
        public bool favorite;

        [DataMember]
        public bool hasPicture;
    }

    [DataContract]
    public class MovieType : IntKey
    {
        [DataMember]
        public string nameShort;

        [DataMember]
        public string nameLong;
    }

    public enum RecordingTimeState
    {
        None,
        Before,
        StartDelay,
        Record,
        StopDelay,
        After
    }

    [DataContract]
    public class Recording
    {
        [DataMember]
        public Guid id;

        [DataMember]
        public int webId;

        [DataMember]
        public string name;

        [DataMember]
        public DateTime start;

        [DataMember]
        public DateTime stop;

        [DataMember]
        public TimeSpan startDelay;

        [DataMember]
        public TimeSpan stopDelay;

        [DataMember]
        public RecordingState state;

        [DataMember]
        public Program program;

        [DataMember]
        public Preset preset;

        [DataMember]
        public int channelId;

        [DataMember]
        public string path;

        [DataMember]
        public int rating;

        public RecordingTimeState GetTimeState()
        {
            DateTime now = DateTime.Now;

            if (now < start + startDelay)
            {
                return RecordingTimeState.Before;
            }

            if (start + startDelay <= now && now < start)
            {
                return RecordingTimeState.StartDelay;
            }

            if (start <= now && now < stop)
            {
                return RecordingTimeState.Record;
            }

            if (stop <= now && now < stop + stopDelay)
            {
                return RecordingTimeState.StopDelay;
            }

            if (stop + stopDelay <= now)
            {
                return RecordingTimeState.After;
            }

            return RecordingTimeState.None;
        }
    }

    [DataContract]
    public class WishlistRecording
    {
        [DataMember]
        public Guid id;

        [DataMember]
        public string title;

        [DataMember]
        public string description;
        
        [DataMember]
        public string actor;

        [DataMember]
        public string director;

        [DataMember]
        public int channelId;
    }

    [DataContract]
    public enum ProgramSearchType
    {
        [EnumMember]
        ByTitle,

        [EnumMember]
        ByMovieType,

        [EnumMember]
        ByDirector,

        [EnumMember]
        ByActor,

        [EnumMember]
        ByYear,
    }

    [DataContract]
    public class ProgramSearchResult
    {
        [DataMember]
        public Program program;

        [DataMember]
        public List<string> matches;
    }

    [DataContract]
    public class AppVersion
    {
        [DataMember]
        public UInt64 version;

        [DataMember]
        public string url;

        [DataMember]
        public bool required;
    }

    [DataContract]
    public class ServiceMessage : IntKey
    {
        [DataMember]
        public string message;
    }

    public enum ServiceMessageType
    {
        None,
        OverlappingRecording,
        SoftwareUpdate,
        DiskSpaceLow
    }

    [DataContract]
    public class ParentalSettings
    {
        [DataMember]
        public string password;

        [DataMember]
        public bool enabled;

        [DataMember]
        public int inactivity;

        [DataMember]
        public int rating;
    }

    [DataContract]
    public class TimeshiftFile
    {
        [DataMember]
        public string path;

        [DataMember]
        public int port;

        [DataMember]
        public int version;
    }

    [DataContract]
    public class TuneRequestPackage : IntKey
    {
        [DataMember]
        public string provider;

        [DataMember]
        public string name;
    }

    [DataContract]
    public class UserInfo
    {
        [DataMember]
        public string email;

        [DataMember]
        public string firstName;

        [DataMember]
        public string lastName;

        [DataMember]
        public string country;

        [DataMember]
        public string address;

        [DataMember]
        public string postalCode;

        [DataMember]
        public string phoneNumber;

        [DataMember]
        public string gender;
    }

    [DataContract]
    public enum UserInputType
    {
        [EnumMember]
        VirtualKey,

        [EnumMember]
        VirtualChar,

        [EnumMember]
        AppCommand,

        [EnumMember]
        RemoteKey,

        [EnumMember]
        MouseKey,
        
        [EnumMember]
        MousePosition
    }

    public interface ICallback
    {
        [OperationContract(IsOneWay = true)]
        void OnUserInput(int type, int value);

        [OperationContract(IsOneWay = true)]
        void OnCurrentPresetChanged(Guid tunerId);
    }

    [ServiceContract(CallbackContract = typeof(ICallback), SessionMode = SessionMode.Required)]
    public interface ICallbackService
    {
        [OperationContract(IsOneWay = true)]
        void Register(long id);

        [OperationContract(IsOneWay = true)]
        void Unregister(long id);
    }
}
