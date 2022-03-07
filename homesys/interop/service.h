#pragma once

#include "api.h"
#include <string>
#include <list>
#include <vector>
#include <atltime.h>
#include "../../util/String.h"

namespace Homesys
{
	class IntKey
	{
	public:
		int id;

		IntKey() {id = 0;}
		virtual ~IntKey() {}
	};

	class MachineReg
	{
	public:
		GUID id;
		std::wstring name;
		bool published;
		std::wstring downloadPath;
		std::wstring recordingPath;
		std::wstring recordingFormat;
		std::wstring timeshiftPath;
		CTimeSpan timeshiftDur;
		std::wstring username;
		std::wstring password;

		MachineReg() {id = GUID_NULL;}
	};

	class TunerConnector : public IntKey
	{
	public:
		int num;
		int type;
		std::wstring name;

		TunerConnector() {num = type = 0;}
	};

	class TunerDevice
	{
	public:
		enum Type
		{
			None = -1,
			Analog = 0,
			DVBS = 1,
			DVBT = 2,
			DVBC = 3,
			DVBF = 4,
		};

		std::wstring dispname;
		std::wstring name;
		Type type;

		TunerDevice() {type = None;}
	};

	class TunerReg : public TunerDevice
	{
	public:
		GUID id;
		std::list<TunerConnector> connectors;

		TunerReg() {id = GUID_NULL;}
	};

	class TunerStat
	{
	public:
		int freq;
		int present;
		int locked;
		int strength;
		int quality;
		__int64 received;
		bool scrambled;

		TunerStat() {freq = present = locked = strength = quality = 0; received = 0; scrambled = false;}
	};

	class SmartCardSubscription
	{
	public:
		WORD id;
		std::wstring name;
		std::list<CTime> date;
	};

	class SmartCardDevice
	{
	public:
		std::wstring name;
		bool inserted;
		std::wstring serial;
		struct {WORD id; std::wstring name;} system;
		std::list<SmartCardSubscription> subscriptions;
	};

	class TuneRequest
	{
	public:
		int freq;
		int standard;
		TunerConnector connector;
		int sid;
		int ifec;
		int ifecRate;
		int ofec;
		int ofecRate;
		int modulation;
		int symbolRate;
		int polarisation;
		bool west;
		int orbitalPosition;
		int azimuth;
		int elevation;
		int lnbSource;
		int lowOscillator;
		int highOscillator;
		int lnbSwitch;
		int spectralInversion;
		int bandwidth;
		int lpifec;
		int lpifecRate;
		int halpha;
		int guard;
		int transmissionMode;
		bool otherFreqInUse;
		std::wstring path;

		TuneRequest() {freq = standard = sid = ifec = ifecRate = ofec = ofecRate = modulation = symbolRate = polarisation = orbitalPosition = azimuth = elevation = lnbSource = lowOscillator = highOscillator = lnbSwitch = spectralInversion = bandwidth = lpifec = lpifecRate = halpha = guard = transmissionMode = 0; west = otherFreqInUse = false;}
	};

	class Preset : public IntKey
	{
	public:
		int channelId;
		std::wstring provider;
		std::wstring name;
		bool scrambled;
		int enabled;
		int rating;
		TuneRequest tunereq;

		Preset() {channelId = rating = 0; scrambled = false; enabled = 1;}
	};

	class RecordingState
	{
	public:
		enum Type
		{
			NotScheduled,
			Scheduled,
			InProgress,
			Aborted,
			Missed,
			Error,
			Finished,
			Overlapping,
			Prompt,
			Deleted,
			Warning
		};
	};

	class Channel : public IntKey
	{
	public:
		std::wstring name;
		bool radio;
		std::vector<BYTE> logo;
		int rank;

		Channel() {radio = false; rank = 0;}
	};

	class Cast : public IntKey
	{
	public:
		std::wstring name;
		std::wstring movieName;
	};

	class Movie : public IntKey
	{
	public:
		std::wstring title;
		std::wstring desc;
		int rating;
		int year;
		int episodeCount;
		std::wstring movieTypeShort;
		std::wstring movieTypeLong;
		std::wstring dvbCategory;
		bool favorite;
		bool hasPicture;

		Movie() {rating = year = episodeCount = 0; favorite = hasPicture = false;}
	};

	class MovieType : public IntKey
	{
	public:
		std::wstring nameShort;
		std::wstring nameLong;

		MovieType() {id = 0;}
	};

	class Episode : public IntKey
	{
	public:
		std::wstring title;
		std::wstring desc;
		int episodeNumber;
		Movie movie;
		std::list<Cast> directors;
		std::list<Cast> actors;

		Episode() {episodeNumber = 0;}
	};

	class Program : public IntKey
	{
	public:
		int channelId;
		CTime start;
		CTime stop;
		RecordingState::Type state;
		Episode episode;

		Program() {channelId = 0;}
	};

	class Recording
	{
	public:
		GUID id;
		int webId;
		std::wstring name;
		CTime start;
		CTime stop;
		CTimeSpan startDelay;
		CTimeSpan stopDelay;
		RecordingState::Type state;
		Program program;
		Preset preset;
		int channelId;
		std::wstring path;
		int rating;

		Recording() {id = GUID_NULL; webId = channelId = 0; rating = 0;}

		UINT64 GetMovieGroupHash() const
		{
			if(program.id && program.episode.movie.id)
			{
				return ((UINT64)program.channelId << 32) | program.episode.movie.id;
			}

			return 0;
		}
	};

	class WishlistRecording
	{
	public:
		GUID id;
		std::wstring title;
		std::wstring desc;
		std::wstring actor;
		std::wstring director;
		int channelId;

		WishlistRecording() {id = GUID_NULL; channelId = 0;}
	};

	class ProgramSearch
	{
	public:
		enum Type
		{
			ByTitle,
			ByMovieType,
			ByDirector,
			ByActor,
			ByYear,
		};
	};

	class ProgramSearchResult
	{
	public:
		Program program;
		std::list<std::wstring> matches;
	};

	class AppVersion
	{
	public:
		UINT64 version;
		std::wstring url;
		bool required;

		AppVersion() {version = 0; required = false;}
	};

	class ServiceMessage : public IntKey
	{
	public:
		std::wstring message;
	};

	class ParentalSettings
	{
	public:
		std::wstring password;
		bool enabled;
		int rating;
		int inactivity;

		ParentalSettings()
		{
			enabled = false;
			rating = 0;
			inactivity = 0;
		}

		static int GetRating(int level, bool enabled)
		{
			int rating = INT_MAX;

			if(enabled)
			{
				if(level == 1)
				{
					rating = 12;
				}
				else if(level == 2)
				{
					rating = 16;
				}
				else if(level == 3)
				{
					rating = 18;
				}
				else if(level == 4)
				{
					rating = 0;
				}
			}

			return rating;
		}

		int GetRating()
		{
			return GetRating(rating, enabled);
		}

		int GetMinRating(int level)
		{
			return min(GetRating(rating, enabled), GetRating(level, true));
		}
	};

	class TimeshiftFile
	{
	public:
		std::wstring path;
		int port;
		int version;

		TimeshiftFile() : port(0), version(0) {}
	};

	class TuneRequestPackage : public IntKey
	{
	public:
		std::wstring provider;
		std::wstring name;
	};

	class UserInfo
	{
	public:
		std::wstring email;
		std::wstring firstName;
		std::wstring lastName;
		std::wstring country;
		std::wstring address;
		std::wstring postalCode;
		std::wstring phoneNumber;
		std::wstring gender;
	};

	class INTEROP_API Service
	{
		class ManagedServicePtr;

		ManagedServicePtr* m_service;

	public:
		Service();
		virtual ~Service();

		bool IsConnected();

		// properties

		bool GetMachine(MachineReg& machine);
		bool IsOnline();
		bool GetUsername(std::wstring& username);
		bool SetUsername(const std::wstring& username);
		bool GetPassword(std::wstring& password);
		bool SetPassword(const std::wstring& password);
		bool GetLanguage(std::wstring& lang);
		bool SetLanguage(const std::wstring& lang);
		bool GetRegion(std::wstring& region);
		bool SetRegion(const std::wstring& region);
		bool GetRecordingPath(std::wstring& path);
		bool SetRecordingPath(const std::wstring& path);
		bool GetRecordingFormat(std::wstring& format);
		bool SetRecordingFormat(const std::wstring& format);
		bool GetTimeshiftPath(std::wstring& path);
		bool SetTimeshiftPath(const std::wstring& path);
		bool GetTimeshiftDuration(CTimeSpan& dur);
		bool SetTimeshiftDuration(const CTimeSpan& dur);
		bool GetDownloadPath(std::wstring& path);
		bool SetDownloadPath(const std::wstring& path);
		bool GetAllTuners(std::list<TunerDevice>& tuners);
		bool GetTuners(std::list<TunerReg>& tuners);
		bool HasTunerType(int type);
		bool GetSmartCards(std::list<SmartCardDevice>& cards);
		bool GetChannels(std::list<Channel>& channels);
		bool GetRecordings(std::list<Recording>& recordings);
		bool GetWishlistRecordings(std::list<WishlistRecording>& recordings);
		bool GetParentalSettings(ParentalSettings& ps);
		bool SetParentalSettings(const ParentalSettings& ps);
        bool GetMostRecentVersion(AppVersion& version);
        bool GetInstalledVersion(UINT64& version);
        bool GetLastGuideUpdate(CTime& t);
		bool GetLastGuideError(std::wstring& error);
        bool GetGuideUpdatePercent(int& percent);

		// members
		
        bool RegisterUser(LPCWSTR username, LPCWSTR password, LPCWSTR email);
        bool UserExists(LPCWSTR username);
		bool LoginExists(LPCWSTR username, LPCWSTR password);
		bool RegisterMachine(LPCWSTR name, const std::list<TunerDevice>& devices);
		bool GetUserInfo(UserInfo& info);
		bool SetUserInfo(const UserInfo& info);
		bool HasUserInfo();
		bool GoOnline();
        bool GetTuner(const GUID& tunerId, TunerReg& tuner);
		bool GetTunerConnectors(const GUID& tunerId, std::list<TunerConnector>& connectors);
        bool GetTunerStat(const GUID& tunerId, TunerStat& stat);
		bool GetPresets(const GUID& tunerId, bool enabled, std::list<Preset>& presets);
		bool SetPresets(const GUID& tunerId, const std::list<int>& presetIds);
        bool GetPreset(int presetId, Preset& preset);
        bool GetCurrentPreset(const GUID& tunerId, Preset& preset);
        bool GetCurrentPresetId(const GUID& tunerId, int& presetId);
        bool SetPreset(const GUID& tunerId, int presetId);
        bool UpdatePreset(const Preset& preset);
		bool UpdatePresets(const std::list<Preset>& presets);
        bool CreatePreset(const GUID& tunerId, Preset& preset);
        bool DeletePreset(int presetId);
		bool GetChannels(const GUID& tunerId, std::list<Channel>& channels);
        bool GetChannel(int channelId, Channel& channel);
		bool GetPrograms(const GUID& tunerId, int channelId, const CTime& start, const CTime& stop, std::list<Program>& programs);
        bool GetProgram(int programId, Program& program);
        bool GetCurrentProgram(const GUID& tunerId, int channelId, Program& program, CTime now = 0);
		bool GetFavoritePrograms(int count, bool picture, std::list<Program>& programs);
		bool GetRelatedPrograms(int programId, std::list<Program>& programs);
        bool GetRecommendedMovieTypes(std::list<MovieType>& movieTypes);
        bool GetRecommendedProgramsByMovieType(int movieTypeId, std::list<Program>& programs);
        bool GetRecommendedProgramsByChannel(int channelId, std::list<Program>& programs);
        bool GetEpisode(int episodeId, Episode& episode);
        bool GetMovie(int movieId, Movie& movie);
		bool SearchPrograms(LPCWSTR query, ProgramSearch::Type type, std::list<ProgramSearchResult>& results);
        bool SearchProgramsByMovie(int movieId, std::list<Program>& programs);
		bool GetRecordings(RecordingState::Type state, std::list<Recording>& recordings);
        bool GetRecording(const GUID& recordingId, Recording& recording);
        bool UpdateRecording(const Recording& recording);
        bool DeleteRecording(const GUID& recordingId);
        bool DeleteMovieRecording(int movieId);
		bool GetProgramsForWishlist(const WishlistRecording& recording, std::list<Program>& programs);
		bool GetWishlistRecording(const GUID& wishlistRecordingId, WishlistRecording& recording);
		bool DeleteWishlistRecording(const GUID& wishlistRecordingId);
		bool RecordByProgram(int programId, bool warnOnly, GUID& recordingId);
        bool RecordByPreset(int presetId, const CTime& start, const CTime& stop, GUID& recordingId);
        bool RecordByMovie(int movieId, int channelId, GUID& movieRecordingId);
        bool RecordByWishlist(const WishlistRecording& recording, GUID& wishlistRecordingId);
        bool Record(const GUID& tunerId, GUID& recordingId);
		bool Record(const GUID& tunerId);
        bool IsRecording(const GUID& tunerId, GUID& recordingId);
        bool IsRecording(const GUID& tunerId);
        bool StopRecording(const GUID& tunerId);
        bool Activate(const GUID& tunerId, TimeshiftFile& tf);
        bool Deactivate(const GUID& tunerId);
        bool DeactivateAll();
        bool SetPrimaryTuner(const GUID& tunerId, bool active);
        bool UpdateGuideNow(bool reset);
        bool PopServiceMessage(ServiceMessage& msg);
        bool GetTuneRequestPackages(const GUID& tunerId, std::list<TuneRequestPackage>& packages);
		bool StartTunerScanByPackage(const GUID& tunerId, int id);
		bool StartTunerScan(const GUID& tunerId, const std::list<TuneRequest>& trs, LPCWSTR provider);
        bool StopTunerScan();
        bool GetTunerScanResult(std::list<Preset>& presets);
        bool IsTunerScanDone();
		bool SaveTunerScanResult(const GUID& tunerId, std::list<Preset>& presets, bool append);
	};

	enum UserInputType
	{
		VirtualKey,
		VirtualChar,
		AppCommand,
		RemoteKey,
		MouseKey,
		MousePosition
	};

	interface ICallback
	{
		virtual void OnUserInput(UserInputType type, int value) = 0;
        virtual void OnCurrentPresetChanged(const GUID& tunerId) = 0;
	};

	class INTEROP_API CallbackService
	{
		class ManagedCallbackServicePtr;

		ManagedCallbackServicePtr* m_service;

	public:
		CallbackService(ICallback* cb);
		virtual ~CallbackService();
	};

	class HttpMedia
	{
	public:
		std::wstring url;
		std::wstring type;
		__int64 length;
	};

	class INTEROP_API HttpMediaDetector
	{
	public:
		HttpMediaDetector();
		virtual ~HttpMediaDetector();

		bool Open(LPCWSTR url, HttpMedia& m);
	};
}
