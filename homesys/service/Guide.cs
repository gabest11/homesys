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
using Microsoft.Win32;
using System.Globalization;
using System.Runtime.Serialization.Json;

namespace Homesys
{
    internal class Guide
    {
        Service _svc;
        Timer _timer;
        bool _forced = false;
        bool _reset = false;
        object _sync = new object();
        string _region = null;

        public delegate void UpdatedEventHandler();
        public event UpdatedEventHandler UpdatedEvent;

        public Guide(Service svc)
        {
            _svc = svc;
            _timer = new Timer(OnTimer, null, UpdateDelay, UpdatePeriod);

            UpdateActivePrograms(false);

            /*
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(EPG));
            MemoryStream ms = new MemoryStream();
            serializer.WriteObject(ms, new EPG { Date = DateTime.Now, TuneRequestPackages = new TuneRequestPackage[] {new TuneRequestPackage { Name = "test" } } });
            string json = Encoding.Default.GetString(ms.ToArray());
            json = json;
            */
        }

        public string Region
        {
            // TODO: HKEY_CURRENT_USER\Control Panel\International\Geo (User=SYSTEM???)

            get
            {
                lock(_sync)
                {
                    try
                    {
                        if(_region == null)
                        {
                            using(RegistryKey key = Registry.LocalMachine.OpenSubKey("Software\\Homesys"))
                            {
                                _region = (string)key.GetValue("Region");
                            }

                            if(_region == null)
                            {
                                _region = "unk"; // RegionInfo.CurrentRegion.ThreeLetterISORegionName.ToLower();
                            }
                        }
                    }
                    catch(Exception e)
                    {
                        Log.WriteLine(e.Message);
                    }

                    return _region;
                }
            }

            set
            {
                lock(_sync)
                {
                    try
                    {
                        bool changed = value != Region;

                        using(RegistryKey key = Registry.LocalMachine.CreateSubKey("Software\\Homesys"))
                        {
                            key.SetValue("Region", value);
                        }

                        _region = value;

                        if(changed) UpdateNow(true);
                    }
                    catch(Exception e)
                    {
                        Log.WriteLine(e);
                    }
                }
            }
        }

        TimeSpan UpdateDelay
        {
            get 
            {
                DateTime ts = _svc.GetWebUpdate(WebUpdateType.Guide);

                if(DateTime.Now - ts > new TimeSpan(1, 0, 0, 0))
                {
                    new TimeSpan(0, 0, 5);
                }

                return new TimeSpan(0, 30, 0); 
            }
        }

        TimeSpan UpdatePeriod
        {
            get { return new TimeSpan(1, 0, 0); }
        }
    
        TimeSpan UpdateLimit
        {
            get { return new TimeSpan(1, 0, 0, 0); }
        }

        public void UpdateNow(bool reset)
        {
            _forced = true;
            _reset = reset;
            _timer.Change(new TimeSpan(0), UpdatePeriod);
        }

        void OnTimer(object o)
        {
            bool taken = false;

            try
            {
                Monitor.TryEnter(_timer, ref taken);

                if(!taken)
                {
                    return;
                }

                System.Threading.ThreadPriority priority = System.Threading.Thread.CurrentThread.Priority;
                System.Threading.Thread.CurrentThread.Priority = System.Threading.ThreadPriority.Lowest;

                DateTime ts = _svc.GetWebUpdate(WebUpdateType.Guide);

                bool expired = false;

                lock(_sync)
                {
                    if(_reset)
                    {
                        ts = Service.DateTimeMin;

                        _reset = false;
                    }

                    if(_forced || DateTime.Now - ts >= UpdateLimit)
                    {
                        expired = true;

                        _forced = false;
                    }
                }

                if(expired)
                {
                    Update(ts);
                }

                System.Threading.Thread.CurrentThread.Priority = priority;
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
            finally
            {
                if(taken)
                {
                    Monitor.Exit(_timer);
                }
            }
        }

        void Update(DateTime ts)
        {
            try
            {
                Log.WriteLine("Guide.Update");

                CultureInfo.CurrentCulture.ClearCachedData();

                _svc.PostServiceMessage(_svc.Localizer[StringLocalizerType.GuideUpdate]);

                using(var svc = _svc.CreateWebService())
                {
                    svc.Timeout = 600000;

                    svc.BeginUpdate(svc.SessionId);

                    Web.EPGLocation loc = svc.GetEPGLocation(svc.SessionId, ts.ToUniversalTime(), Region);

                    if(loc != null)
                    {
                        /*
                        if(loc.JsonBZip2Url != null)
                        {
                            EPG epg;

                            HttpWebRequest req = (HttpWebRequest)WebRequest.Create(loc.JsonBZip2Url ?? loc.JsonUrl);

                            req.Headers.Add("Accept-Encoding", "gzip, deflate");

                            using(WebResponse res = new Homesys.Web.GzipWebResponse(req.GetResponse()))
                            {
                                using(Stream stream = res.GetResponseStream())
                                {
                                    epg = (EPG)(new DataContractJsonSerializer(typeof(EPG))).ReadObject(stream); 

                                    stream.Close();
                                }
                            }

                            Insert(epg);

                            _svc.SetWebUpdate(WebUpdateType.Guide, epg.Date);
                        }
                        else
                        */
                        {
                            Web.EPG epg;

                            HttpWebRequest req = (HttpWebRequest)WebRequest.Create(loc.BZip2Url ?? loc.Url);

                            req.Headers.Add("Accept-Encoding", "gzip, deflate");

                            using(WebResponse res = new Homesys.Web.GzipWebResponse(req.GetResponse()))
                            {
                                using(Stream stream = res.GetResponseStream())
                                {
                                    epg = (Web.EPG)(new XmlSerializer(typeof(Web.EPG))).Deserialize(stream);

                                    stream.Close();
                                }
                            }

                            Insert(epg);

                            _svc.SetWebUpdate(WebUpdateType.Guide, epg.Date);
                        }
                    }

                    SyncStat(svc);

                    svc.EndUpdate(svc.SessionId);
                }

                // Cleanup();

                _svc.PostServiceMessage(_svc.Localizer[StringLocalizerType.GuideUpdateDone]);

                Log.WriteLine("Guide.Update Done");

                if(UpdatedEvent != null)
                {
                    UpdatedEvent();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);

                _svc.SetWebUpdateError(WebUpdateType.Guide, e.Message);
            }
        }

        void SyncStat(Web.HomesysService svc)
        {
            try
            {
                using(var Context = _svc.CreateDataContext())
                {
                    Dictionary<int, int> ranking = new Dictionary<int, int>();

                    int i = 0;

                    foreach(int channelId in svc.GetChannelRank(svc.SessionId))
                    {
                        ranking[channelId] = ++i;
                    }

                    foreach(var c in Context.Channel)
                    {
                        c.Rank = ranking.TryGetValue(c.Id, out i) ? i : 0;
                    }

                    Context.SubmitChanges();

                    int[] movieIds = svc.GetMovieRank(svc.SessionId);

                    if(movieIds != null && movieIds.Length > 0)
                    {
                        Context.Truncate(Context.MovieFavorite);

                        Context.Insert(Context.MovieFavorite, from id in movieIds select new object[] { id }, "id");
                    }

                    // TODO

                    Web.RelatedProgram[] rp = svc.GetRelatedPrograms(svc.SessionId);

                    if(rp != null)
                    {
                        Context.Truncate(Context.ProgramRelated, "id");

                        Context.Insert(
                            Context.ProgramRelated,
                            from r in rp select new object[] { r.Id, r.OtherId, r.Type },
                            "program_id", "other_program_id", "type");
                    }
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }

            try
            {
                using(var Context = _svc.CreateDataContext(false))
                {
                    var q = from p in Context.Preset where p.Channel_id == 0 select p.Name;

                    svc.SetUnknownChannels(svc.SessionId, Region, q.ToArray());
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }

        void Insert(Web.EPG epg)
        {
            _svc.PostServiceMessage(_svc.Localizer[StringLocalizerType.GuideUpdateImport]);

            _svc.SetWebUpdateError(WebUpdateType.Guide, "");

            Log.WriteLine("Guide.Insert (TuneRequestPackages)");

            using(TransactionScope ts = new TransactionScope(TransactionScopeOption.Required, new TimeSpan(0, 0, 10, 0)))
            {
                using(var Context = _svc.CreateDataContext())
                {
                    // TuneRequestPackages

                    if(epg.TuneRequestPackages != null)
                    {
                        Context.Truncate(Context.TuneRequestPackage, "id");
                        Context.Truncate(Context.TuneRequest);

                        List<object[]> rows = new List<object[]>();

                        foreach(Web.TuneRequestPackage p in epg.TuneRequestPackages)
                        {
                            int type = 0;

                            switch(p.Type)
                            {
                                case "Analog": type = (int)Local.TunerDeviceType.Analog; break;
                                case "DVB-S": type = (int)Local.TunerDeviceType.DVBS; break;
                                case "DVB-T": type = (int)Local.TunerDeviceType.DVBT; break;
                                case "DVB-C": type = (int)Local.TunerDeviceType.DVBC; break;
                                case "DVB-F": type = (int)Local.TunerDeviceType.DVBF; break;
                            }

                            Data.TuneRequestPackage package = new Data.TuneRequestPackage { Type = type, Provider = p.Provider, Name = p.Name };

                            Context.TuneRequestPackage.InsertOnSubmit(package);

                            Context.SubmitChanges();

                            rows.AddRange(from r in p.TuneRequests select new object[]
                                { 
                                    package.Id, 
                                    r.ChannelId, 
                                    r.Frequency,
                                    r.VideoStandard,
                                    r.IFECMethod,
                                    r.IFECRate,
                                    r.OFECMethod,
                                    r.OFECRate,
                                    r.Modulation,
                                    r.SymbolRate,
                                    r.Polarisation,
                                    r.LNBSource,
                                    r.SpectralInversion,
                                    r.Bandwidth,
                                    r.LPIFECMethod,
                                    r.LPIFECRate,
                                    r.HAlpha,
                                    r.Guard,
                                    r.TransmissionMode,
                                    r.OtherFrequencyInUse,
                                    r.Path
                                });
                        }

                        Context.Insert(
                            Context.TuneRequest,
                            rows,
                            "package_id", "channel_id", "freq", "standard",
                            "ifec", "ifecrate", "ofec", "ofecrate",
                            "modulation", "symbol_rate", "polarisation", "lnb_source",
                            "spectral_inversion", "bandwidth", "lpifec", "lpifecrate",
                            "halpha", "guard", "transmission_mode", "other_freq_in_use",
                            "path");
                    }
                }

                ts.Complete();
            }

            HashSet<int> newProgramIds = new HashSet<int>();
            HashSet<int> newEpisodeIds = new HashSet<int>();
            HashSet<int> newMovieIds = new HashSet<int>();

            using(TransactionScope ts = new TransactionScope(TransactionScopeOption.Required, new TimeSpan(0, 0, 10, 0)))
            {
                using(var Context = _svc.CreateDataContext())
                {
                    if(epg.Programs != null && epg.Episodes != null && epg.Movies != null && epg.Casts != null)
                    {
                        Log.WriteLine("Guide.Insert (Program/Episode/Movie/Cast)");

                        HashSet<int> oldProgramIds = new HashSet<int>(from row in Context.Program select row.Id);

                        newProgramIds = new HashSet<int>(from row in epg.Programs select row.Id);
                        newEpisodeIds = new HashSet<int>(from row in epg.Episodes select row.Id);
                        newMovieIds = new HashSet<int>(from row in epg.Movies select row.Id);

                        HashSet<int> recProgramIds = new HashSet<int>(from row in Context.Recording where row.Program_id != null select row.Program_id.Value);
                        recProgramIds.ExceptWith(newProgramIds);
                        Context.TruncateExcept(Context.Program, recProgramIds);

                        HashSet<int> recEpisodeIds = new HashSet<int>(from row in Context.Program select row.Episode_id);
                        recEpisodeIds.ExceptWith(newEpisodeIds);
                        Context.TruncateExcept(Context.Episode, recEpisodeIds);

                        HashSet<int> recMovieIds = new HashSet<int>(from row in Context.Program select row.Movie_id);
                        recMovieIds.ExceptWith(newMovieIds);
                        Context.TruncateExcept(Context.Movie, recMovieIds);

                        newProgramIds.ExceptWith(oldProgramIds);

                        // Program

                        Log.WriteLine("Guide.Insert (Program)");

                        Context.Insert(
                            Context.Program,
                            from r in epg.Programs select new object[] { r.Id, r.ChannelId, r.EpisodeId, r.MovieId, r.StartDate, r.StopDate, r.IsRepeat, r.IsLive, r.IsRecommended, r.Deleted },
                            "id", "channel_id", "episode_id", "movie_id", "start", "stop", "is_repeat", "is_live", "is_recommended", "is_deleted");

                        // Movie

                        Log.WriteLine("Guide.Insert (Movie)");

                        Context.Insert(
                            Context.Movie,
                            from r in epg.Movies select new object[] { r.Id, r.MovieTypeId, r.DVBCategoryId, r.Title, r.Description ?? "", r.Rating, r.Year, r.EpisodeCount, r.Temp, r.HasPicture },
                            "id", "movietype_id", "dvbcategory_id", "title", "description", "rating", "year", "episode_count", "temp", "has_picture");

                        // Episode

                        Log.WriteLine("Guide.Insert (Episode)");

                        Context.Insert(
                            Context.Episode,
                            from r in epg.Episodes select new object[] { r.Id, r.MovieId, r.Title, r.Description ?? "", r.EpisodeNumber, r.Temp },
                            "id", "movie_id", "title", "description", "episode_number", "temp");

                        // EpisodeCast

                        List<object[]> rows = new List<object[]>();

                        foreach(Web.Episode e in epg.Episodes)
                        {
                            if(e.EpisodeCastList != null)
                            {
                                rows.AddRange(from r in e.EpisodeCastList select new object[] { e.Id, r.CastId, r.RoleId, r.Name });
                            }
                        }

                        Context.Truncate(Context.EpisodeCast, "id");

                        Context.Insert(Context.EpisodeCast, rows, "episode_id", "cast_id", "role_id", "name");

                        // Cast

                        Context.Truncate(Context.Cast);

                        Log.WriteLine("Guide.Insert (Cast)");

                        Context.Insert(Context.Cast,
                            from r in epg.Casts select new object[] { r.Id, r.Name },
                            "id", "name");
                    }

                    // Channel

                    Context.Truncate(Context.Channel);
                    Context.Truncate(Context.ChannelAlias);

                    if(epg.Channels != null)
                    {
                        Log.WriteLine("Guide.Insert (Channel)");

                        Context.Insert(
                            Context.Channel,
                            from r in epg.Channels select new object[] { r.Id, r.Name, r.Url, r.Radio, r.Logo },
                            "id", "name", "url", "radio", "logo");

                        Dictionary<string, int> aliases = new Dictionary<string, int>();

                        foreach(Web.Channel c in epg.Channels)
                        {
                            foreach(string s in c.Aliases.Split(new char[] { '|' }))
                            {
                                if(s.Length != 0) aliases[s] = c.Id;
                            }                            
                        }

                        Context.Insert(
                            Context.ChannelAlias,
                            from r in aliases select new object[] { r.Value, r.Key },
                            "channel_id", "name");
                    }

                    // MovieType

                    Context.Truncate(Context.MovieType);

                    if(epg.MovieTypes != null)
                    {
                        Log.WriteLine("Guide.Insert (MovieType)");

                        Context.Insert(
                            Context.MovieType,
                            from r in epg.MovieTypes select new object[] { r.Id, r.NameShort, r.NameLong },
                            "id", "name_short", "name_long");
                    }

                    // DVBCategory

                    Context.Truncate(Context.DVBCategory);

                    if(epg.DVBCategories != null)
                    {
                        Log.WriteLine("Guide.Insert (DVBCategory)");

                        Context.Insert(
                            Context.DVBCategory,
                            from r in epg.DVBCategories select new object[] { r.Id, r.Name },
                            "id", "name");
                    }
                }

                ts.Complete();
            }

            Log.WriteLine("Received {0} channels, {1} programs, {2} episodes, {3} movies, {4} cast", epg.Channels.Length, epg.Programs.Length, epg.Episodes.Length, epg.Movies.Length, epg.Casts.Length);

            UpdateChannels();

            UpdateActivePrograms();

            UpdateRecordings(newProgramIds);
        }
/*
        void Insert(EPG epg)
        {
            _svc.PostServiceMessage(_svc.Localizer[StringLocalizerType.GuideUpdateImport]);

            _svc.SetWebUpdateError(WebUpdateType.Guide, "");

            Log.WriteLine("Guide.Insert (TuneRequestPackages)");

            using(TransactionScope ts = new TransactionScope(TransactionScopeOption.Required, new TimeSpan(0, 0, 10, 0)))
            {
                using(var Context = _svc.CreateDataContext())
                {
                    // TuneRequestPackages

                    if(epg.TuneRequestPackages != null)
                    {
                        Context.Truncate(Context.TuneRequestPackage, "id");
                        Context.Truncate(Context.TuneRequest);

                        List<object[]> rows = new List<object[]>();

                        foreach(Web.TuneRequestPackage p in epg.TuneRequestPackages)
                        {
                            int type = 0;

                            switch(p.Type)
                            {
                                case "Analog": type = (int)Local.TunerDeviceType.Analog; break;
                                case "DVB-S": type = (int)Local.TunerDeviceType.DVBS; break;
                                case "DVB-T": type = (int)Local.TunerDeviceType.DVBT; break;
                                case "DVB-C": type = (int)Local.TunerDeviceType.DVBC; break;
                                case "DVB-F": type = (int)Local.TunerDeviceType.DVBF; break;
                            }

                            Data.TuneRequestPackage package = new Data.TuneRequestPackage { Type = type, Provider = p.Provider, Name = p.Name };

                            Context.TuneRequestPackage.InsertOnSubmit(package);

                            Context.SubmitChanges();

                            rows.AddRange(from r in p.TuneRequests
                                          select new object[]
                                { 
                                    package.Id, 
                                    r.ChannelId, 
                                    r.Frequency,
                                    r.VideoStandard,
                                    r.IFECMethod,
                                    r.IFECRate,
                                    r.OFECMethod,
                                    r.OFECRate,
                                    r.Modulation,
                                    r.SymbolRate,
                                    r.Polarisation,
                                    r.LNBSource,
                                    r.SpectralInversion,
                                    r.Bandwidth,
                                    r.LPIFECMethod,
                                    r.LPIFECRate,
                                    r.HAlpha,
                                    r.Guard,
                                    r.TransmissionMode,
                                    r.OtherFrequencyInUse,
                                    r.Path
                                });
                        }

                        Context.Insert(
                            Context.TuneRequest,
                            rows,
                            "package_id", "channel_id", "freq", "standard",
                            "ifec", "ifecrate", "ofec", "ofecrate",
                            "modulation", "symbol_rate", "polarisation", "lnb_source",
                            "spectral_inversion", "bandwidth", "lpifec", "lpifecrate",
                            "halpha", "guard", "transmission_mode", "other_freq_in_use",
                            "path");
                    }
                }

                ts.Complete();
            }

            HashSet<int> newProgramIds = new HashSet<int>();
            HashSet<int> newEpisodeIds = new HashSet<int>();
            HashSet<int> newMovieIds = new HashSet<int>();

            using(TransactionScope ts = new TransactionScope(TransactionScopeOption.Required, new TimeSpan(0, 0, 10, 0)))
            {
                using(var Context = _svc.CreateDataContext())
                {
                    if(epg.Programs != null && epg.Episodes != null && epg.Movies != null && epg.Casts != null)
                    {
                        Log.WriteLine("Guide.Insert (Program/Episode/Movie/Cast)");

                        HashSet<int> oldProgramIds = new HashSet<int>(from row in Context.Program select row.Id);

                        newProgramIds = new HashSet<int>(from row in epg.Programs select row.Id);
                        newEpisodeIds = new HashSet<int>(from row in epg.Episodes select row.Id);
                        newMovieIds = new HashSet<int>(from row in epg.Movies select row.Id);

                        HashSet<int> recProgramIds = new HashSet<int>(from row in Context.Recording where row.Program_id != null select row.Program_id.Value);
                        recProgramIds.ExceptWith(newProgramIds);
                        Context.TruncateExcept(Context.Program, recProgramIds);

                        HashSet<int> recEpisodeIds = new HashSet<int>(from row in Context.Program select row.Episode_id);
                        recEpisodeIds.ExceptWith(newEpisodeIds);
                        Context.TruncateExcept(Context.Episode, recEpisodeIds);

                        HashSet<int> recMovieIds = new HashSet<int>(from row in Context.Program select row.Movie_id);
                        recMovieIds.ExceptWith(newMovieIds);
                        Context.TruncateExcept(Context.Movie, recMovieIds);

                        newProgramIds.ExceptWith(oldProgramIds);

                        // Program

                        Log.WriteLine("Guide.Insert (Program)");

                        Context.Insert(
                            Context.Program,
                            from r in epg.Programs select new object[] { r.Id, r.ChannelId, r.EpisodeId, r.MovieId, r.StartDate, r.StopDate, r.IsRepeat, r.IsLive, r.IsRecommended, r.Deleted },
                            "id", "channel_id", "episode_id", "movie_id", "start", "stop", "is_repeat", "is_live", "is_recommended", "is_deleted");

                        // Movie

                        Log.WriteLine("Guide.Insert (Movie)");

                        Context.Insert(
                            Context.Movie,
                            from r in epg.Movies select new object[] { r.Id, r.MovieTypeId, r.DVBCategoryId, r.Title, r.Description ?? "", r.Rating, r.Year, r.EpisodeCount, r.Temp, r.HasPicture },
                            "id", "movietype_id", "dvbcategory_id", "title", "description", "rating", "year", "episode_count", "temp", "has_picture");

                        // Episode

                        Log.WriteLine("Guide.Insert (Episode)");

                        Context.Insert(
                            Context.Episode,
                            from r in epg.Episodes select new object[] { r.Id, r.MovieId, r.Title, r.Description ?? "", r.EpisodeNumber, r.Temp },
                            "id", "movie_id", "title", "description", "episode_number", "temp");

                        // EpisodeCast

                        List<object[]> rows = new List<object[]>();

                        foreach(Web.Episode e in epg.Episodes)
                        {
                            if(e.EpisodeCastList != null)
                            {
                                rows.AddRange(from r in e.EpisodeCastList select new object[] { e.Id, r.CastId, r.RoleId, r.Name });
                            }
                        }

                        Context.Truncate(Context.EpisodeCast, "id");

                        Context.Insert(Context.EpisodeCast, rows, "episode_id", "cast_id", "role_id", "name");

                        // Cast

                        Context.Truncate(Context.Cast);

                        Log.WriteLine("Guide.Insert (Cast)");

                        Context.Insert(Context.Cast,
                            from r in epg.Casts select new object[] { r.Id, r.Name },
                            "id", "name");
                    }

                    // Channel

                    Context.Truncate(Context.Channel);

                    if(epg.Channels != null)
                    {
                        Log.WriteLine("Guide.Insert (Channel)");

                        Context.Insert(
                            Context.Channel,
                            from r in epg.Channels select new object[] { r.Id, r.Name, r.Url, r.Radio, r.Logo },
                            "id", "name", "url", "radio", "logo");
                    }

                    // MovieType

                    Context.Truncate(Context.MovieType);

                    if(epg.MovieTypes != null)
                    {
                        Log.WriteLine("Guide.Insert (MovieType)");

                        Context.Insert(
                            Context.MovieType,
                            from r in epg.MovieTypes select new object[] { r.Id, r.NameShort, r.NameLong },
                            "id", "name_short", "name_long");
                    }

                    // DVBCategory

                    Context.Truncate(Context.DVBCategory);

                    if(epg.DVBCategories != null)
                    {
                        Log.WriteLine("Guide.Insert (DVBCategory)");

                        Context.Insert(
                            Context.DVBCategory,
                            from r in epg.DVBCategories select new object[] { r.Id, r.Name },
                            "id", "name");
                    }
                }

                ts.Complete();
            }

            Log.WriteLine("Received {0} channels, {1} programs, {2} episodes, {3} movies, {4} cast", epg.Channels.Length, epg.Programs.Length, epg.Episodes.Length, epg.Movies.Length, epg.Casts.Length);

            UpdateActivePrograms();

            UpdateRecordings(newProgramIds);
        }
*/

        public bool UpdateChannels()
        {
            bool changed = false;

            try
            {
                using(var Context = _svc.CreateDataContext())
                {
                    var q3 =
                        from p in Context.Preset
                        join c in Context.Channel on p.Channel_id equals c.Id into c2
                        from i in c2.DefaultIfEmpty()
                        where p.Channel_id != 0 && i == null
                        select p;

                    foreach(var r in q3.ToList())
                    {
                        r.Channel_id = 0;
                    }

                    Context.SubmitChanges();
                    /*
                    foreach(var r in Context.Preset)
                        if(r.Channel_id == 0)
                            Console.WriteLine("{0} {1}", r.Channel_id, r.Name);

                    foreach(var r in Context.ChannelAlias)
                        if(r.Channel_id == 5)
                            Console.WriteLine("{0} {1}", r.Channel_id, r.Name);
                    */
                    var q1 =
                        from p in Context.Preset
                        join ca in Context.Channel on p.Name equals ca.Name
                        where p.Channel_id == 0
                        select new { p.Id, Channel_id = ca.Id };

                    foreach(var r in q1.ToList())
                    {
                        Data.Preset p = Context.Preset.Single(row => row.Id == r.Id);

                        Log.WriteLine("{0} => {1}", p.Name, r.Channel_id);

                        p.Channel_id = r.Channel_id;

                        changed = true;
                    }

                    Context.SubmitChanges();

                    var q2 =
                        from p in Context.Preset
                        join ca in Context.ChannelAlias on p.Name equals ca.Name
                        where p.Channel_id == 0
                        select new { p.Id, ca.Channel_id };

                    foreach(var r in q2.ToList())
                    {
                        Data.Preset p = Context.Preset.Single(row => row.Id == r.Id);

                        Log.WriteLine("{0} => {1}", p.Name, r.Channel_id);

                        p.Channel_id = r.Channel_id;

                        changed = true;
                    }

                    Context.SubmitChanges();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e.Message);
            }

            return changed;
        }

        public void UpdateActivePrograms(bool force = true)
        {
            try
            {
                using(TransactionScope ts = new TransactionScope())
                {
                    using(var Context = _svc.CreateDataContext())
                    {
                        System.Data.Common.DbCommand cmd = Context.Connection.CreateCommand();

                        if(force || Context.ProgramActive.Count() == 0)
                        {
                            cmd.CommandText = "DELETE FROM ProgramActive";

                            cmd.ExecuteNonQuery();

                            cmd.CommandText =
                                "INSERT INTO ProgramActive " +
                                "SELECT DISTINCT t1.* FROM Program t1 " +
                                "JOIN Preset t2 ON t2.channel_id = t1.channel_id " +
                                "WHERE t1.is_deleted = 0 AND t2.enabled > 0";

                            cmd.ExecuteNonQuery();
                        }

                        if(force || Context.EpisodeActive.Count() == 0)
                        {
                            cmd.CommandText = "DELETE FROM EpisodeActive";

                            cmd.ExecuteNonQuery();

                            cmd.CommandText =
                                "INSERT INTO EpisodeActive " +
                                "SELECT DISTINCT t2.* FROM ProgramActive t1 " +
                                "JOIN Episode t2 ON t2.id = t1.episode_id ";

                            cmd.ExecuteNonQuery();
                        }

                        if(force || Context.MovieActive.Count() == 0)
                        {
                            cmd.CommandText = "DELETE FROM MovieActive";

                            cmd.ExecuteNonQuery();

                            cmd.CommandText =
                                "INSERT INTO MovieActive " +
                                "SELECT DISTINCT t2.* FROM ProgramActive t1 " +
                                "JOIN Movie t2 ON t2.id = t1.movie_id ";

                            cmd.ExecuteNonQuery();
                        }
                    }

                    ts.Complete();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e.Message);
            }
        }

        private void UpdateRecordings(HashSet<int> newProgramIds)
        {
            try
            {
                List<object[]> recs;

                using(var Context = _svc.CreateDataContext(false))
                {
                    var q =
                        from r in Context.Recording
                        join p in Context.ProgramActive on r.Program_id equals p.Id
                        where r.Program_id != null && (r.State == (int)Local.RecordingState.Scheduled || r.State == (int)Local.RecordingState.Warning) && (r.Start != p.Start || r.Stop != p.Stop)
                        orderby p.Start, p.Stop
                        select new object[] {r.Id, p.Start, p.Stop};

                    recs = q.ToList();
                }

                using(var Context = _svc.CreateDataContext())
                {
                    foreach(object[] o in recs)
                    {
                        Data.Recording rec = Context.Recording.SingleOrDefault(r => r.Id == (Guid)o[0]);

                        if(rec != null)
                        {
                            rec.Start = (DateTime)o[1];
                            rec.Stop = (DateTime)o[2];
                        }
                    }

                    Context.SubmitChanges();
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }

            try
            {
                List<int> recProgramIds = null;

                using(var Context = _svc.CreateDataContext(false))
                {
                    var ids =
                        from m in Context.MovieRecording
                        join p in Context.ProgramActive on m.Movie_id equals p.Movie_id
                        where m.Channel_id == p.Channel_id || m.Channel_id == 0
                        orderby p.Start, p.Stop
                        select p.Id;

                    recProgramIds = ids.ToList().Intersect(newProgramIds).ToList();
                }

                foreach(int id in recProgramIds)
                {
                    _svc.RecordByProgram(id, false);
                }

                _svc.RecordByWishlist(newProgramIds);
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }
        /*
        void Cleanup()
        {
            try
            {
                Log.WriteLine("Guide.Cleanup");

                using(var Context = _svc.CreateDataContext())
                {
                    Context.Delete(Context.Program,
                        from p in Context.Program
                        where p.Start < DateTime.Now.AddDays(-3)
                        from r in
                            (from row in Context.Recording where row.Program_id == p.Id select row).DefaultIfEmpty()
                        where r == null
                        select p.Id);

                    Context.ExecuteCommand("DELETE FROM [Program] WHERE [id] = 740159 AND [channel_id] = 5"); // don't ask...

                    Context.Delete(Context.Episode,
                        from e in Context.Episode
                        where e.Temp && !(from row in Context.Program select row.Episode_id).Contains(e.Id)
                        select e.Id);

                    Context.Delete(Context.Movie,
                        from m in Context.Movie
                        where m.Temp && !(from row in Context.Episode select row.Movie_id).Contains(m.Id)
                        select m.Id);

                    Context.Delete(Context.EpisodeCast,
                        from ec in Context.EpisodeCast
                        where !(from row in Context.Episode select row.Id).Contains(ec.Episode_id)
                        select ec.Id);

                    Context.Delete(Context.MovieRecording,
                        from mr in Context.MovieRecording
                        where !(from row in Context.Movie select row.Id).Contains(mr.Movie_id)
                        select mr.Id);
                }
            }
            catch(Exception e)
            {
                Log.WriteLine(e);
            }
        }
        */

        /*
        [DataContract]
        internal class EPG
        {
            [IgnoreDataMember]
            public DateTime Date { get; set; }

            [DataMember(Name = "Date")]
            private string DateString
            {
                get { return Date.ToUniversalTime().ToString("g") + " GMT"; } 
                set { Date = DateTime.Parse(value); }
            }
	
	        // Headend

            [DataMember]
            public TuneRequestPackage[] TuneRequestPackages;
         
            //public Channel[] Channels;
            
            //public Program[] Programs;
	
            //public Episode[] Episodes;

            //public Movie[] Movies;

            //public Cast[] Casts;

            //public MovieType[] MovieTypes;

            //public DVBCategory[] DVBCategories;
        }

        [DataContract]
        internal class TuneRequestPackage
        {
            [DataMember]
            public string Type;

            [DataMember]
            public string Provider;

            [DataMember]
            public string Name;

            [DataMember]
            public TuneRequest[] TuneRequests;
        }

        [DataContract]
        internal class TuneRequest
        {
            [DataMember]
	        public int Frequency;

            [DataMember]
            public int VideoStandard;

            [DataMember]
            public int IFECMethod;

            [DataMember]
            public int IFECRate;

            [DataMember]
            public int OFECMethod;

            [DataMember]
            public int OFECRate;

            [DataMember]
            public int Modulation;

            [DataMember]
            public int SymbolRate;

            [DataMember]
            public int Polarisation;

            [DataMember]
            public int LNBSource;

            [DataMember]
            public int SpectralInversion;

            [DataMember]
            public int Bandwidth;

            [DataMember]
            public int LPIFECMethod;

            [DataMember]
            public int LPIFECRate;

            [DataMember]
            public int HAlpha;

            [DataMember]
            public int Guard;

            [DataMember]
            public int TransmissionMode;

            [DataMember]
            public bool OtherFrequencyInUse;

            [DataMember]
            public string Path;

            [DataMember]
            public int ChannelId;
        }
         */
    }
}
