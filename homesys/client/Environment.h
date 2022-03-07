#pragma once

#include "Player.h"

class HotLink
{
public:
	HotLink() : message(false) {}
	virtual ~HotLink() {}

	bool message;
	std::wstring username;
};

class HotLinkProgram : public HotLink
{
public:
	Homesys::Program program;
};

class Environment
{
	HINSTANCE m_hInstance;

	struct PlayerStatus
	{
		REFERENCE_TIME start;
		REFERENCE_TIME stop;
		REFERENCE_TIME current;
		REFERENCE_TIME seekto;

		OAFilterState state;

		Homesys::Recording recording;

		struct
		{
			DVD_DOMAIN domain;
			int title, chapter;
			int angle, audio, subpicture;
			int error;
		} dvd;

		struct
		{
			bool recording;
			Homesys::Preset preset;
			Homesys::Channel channel;
			Homesys::TunerStat stat;
		} tuner;

		struct PlayerStatus()
		{
			start = stop = current = 0;
			seekto = -1;
			state = State_Stopped;
			memset(&dvd, 0, sizeof(dvd));
			tuner.recording = false;
		}

		REFERENCE_TIME GetCurrent() const {return seekto >= 0 ? seekto : current;}
	};

	struct ServiceStatus
	{
		bool connected, online; 
		Homesys::MachineReg machine;
		std::queue<Homesys::ServiceMessage> messages; 

		struct ServiceStatus()
		{
			connected = online = false;
		}
	};

	void GetAppPath(std::wstring& path);
	void GetAppDataPath(std::wstring& path);
	void GetCachePath(std::wstring& path);
	void GetResourcePath(std::wstring& path);
	void GetTempPath(std::wstring& path);

	void GetPositionRT(REFERENCE_TIME& value);
	void GetPositionF(float& value);
	void GetPositionText(std::wstring& value);
	void GetDurationRT(REFERENCE_TIME& value);
	void GetDurationText(std::wstring& value);
	void SetMuted(bool& value);
	void SetVolume(float& value);
	void GetBeep(bool& value);
	void SetBeep(bool& value);

	bool OnPrevPlayer(DXUI::Control* c);
	bool OnNextPlayer(DXUI::Control* c);
	bool OnPrevNextPlayer(DXUI::Control* c);
	bool OnPrev(DXUI::Control* c);
	bool OnNext(DXUI::Control* c);
	bool OnSkipBackward(DXUI::Control* c);
	bool OnSkipForward(DXUI::Control* c);
	bool OnStop(DXUI::Control* c);
	bool OnPause(DXUI::Control* c);
	bool OnPlay(DXUI::Control* c);
	bool OnPlayPause(DXUI::Control* c);
	bool OnReturn(DXUI::Control* c);
	bool OnRecord(DXUI::Control* c);
	bool OnSeekAsync(DXUI::Control* c, float value);
	bool OnSeekSync(DXUI::Control* c, float value);

	bool OnTrackSwitch(DXUI::Control* c, int id);
	bool OnVideoSwitch(DXUI::Control* c);
	bool OnAudioSwitch(DXUI::Control* c);
	bool OnSubtitleSwitch(DXUI::Control* c);

	bool OnPresetSwitch(DXUI::Control* c);
	bool OnProgramSchedule(DXUI::Control* c, Homesys::Program* program);
	bool OnProgramScheduleByMovie(DXUI::Control* c, Homesys::Program* program);
	bool OnProgramScheduleByMovieAndChannel(DXUI::Control* c, Homesys::Program* program);
	bool OnProgramCancelRecording(DXUI::Control* c, Homesys::Program* program);
	bool OnProgramCancelMovieRecording(DXUI::Control* c, Homesys::Program* program);

	bool OnDefaultSoundEvent(DXUI::Control* c);

	bool OnThreadTimer(DXUI::Control* c, UINT64 n);

	DXUI::Thread m_thread;
	bool m_hotlinks_invalid;

	void SeekByOffset(REFERENCE_TIME dt, bool keyframe);
	void FindKeyFrame(IGenericPlayer* player, REFERENCE_TIME& rt, REFERENCE_TIME dt = 0);

	std::vector<float> m_fft_max;
	std::vector<clock_t> m_fft_ttl;

	bool m_beep;

public:
	Environment(HINSTANCE hInstance);
	virtual ~Environment();

	Homesys::Service svc;
	Homesys::ParentalSettings parental;
	std::list<CAdapt<CComPtr<IGenericPlayer>>> players;
	PlayerStatus ps;
	ServiceStatus ss;
	std::list<HotLink*> hotlinks;

	void UpdateServiceState();

	void AutoSeek();
	void InvalidateHotLinks() {m_hotlinks_invalid = true;}

	std::wstring FormatMoviePictureUrl(int id);

	DXUI::Texture* GetAudioTexture(IGenericPlayer* player, DXUI::DeviceContext* dc);

	DXUI::Property<std::wstring, true> AppPath;
	DXUI::Property<std::wstring, true> AppDataPath;
	DXUI::Property<std::wstring, true> CachePath;
	DXUI::Property<std::wstring, true> ResourcePath;
	DXUI::Property<std::wstring, true> TempPath;

	DXUI::Property<REFERENCE_TIME> PositionRT;
	DXUI::Property<float> PositionF;
	DXUI::Property<std::wstring> PositionText;
	DXUI::Property<REFERENCE_TIME> DurationRT;
	DXUI::Property<std::wstring> DurationText;
	DXUI::Property<bool> Muted;
	DXUI::Property<float> Volume;
	DXUI::Property<bool> Beep;

	DXUI::Event<std::wstring> PlayFileEvent;
	DXUI::Event<std::wstring> OpenFileEvent;
	DXUI::Event<std::wstring> EditFileEvent;
	DXUI::Event<> EndFileEvent;
	DXUI::Event<> CloseFileEvent;

	DXUI::Event<> PrevPlayerEvent;
	DXUI::Event<> NextPlayerEvent;
	DXUI::Event<> PrevEvent;
	DXUI::Event<> NextEvent;
	DXUI::Event<bool> SkipBackwardEvent;
	DXUI::Event<bool> SkipForwardEvent;
	DXUI::Event<> PlayEvent;
	DXUI::Event<> PlayPauseEvent;
	DXUI::Event<> ReturnEvent;
	DXUI::Event<> PauseEvent;
	DXUI::Event<> StopEvent;
	DXUI::Event<> RecordEvent;
	DXUI::Event<float> SeekAsyncEvent;
	DXUI::Event<float> SeekSyncEvent;
	DXUI::Event<> InfoEvent;
	DXUI::Event<> TeletextEvent;

	DXUI::Event<> MuteEvent;
	DXUI::Event<> FullRangeEvent;
	DXUI::Event<> AspectRatioEvent;
	DXUI::Event<> PIPEvent;

	DXUI::Event<int> TrackSwitchEvent;
	DXUI::Event<> VideoSwitchEvent;
	DXUI::Event<> AudioSwitchEvent;
	DXUI::Event<> SubtitleSwitchEvent;

	DXUI::Event<> VideoClickedEvent;
	DXUI::Event<> RecordClickedEvent;
	DXUI::Event<> ChannelDetailsEvent;
	DXUI::Event<GUID> ModifyRecording;
	DXUI::Event<> RegisterUserEvent;

	DXUI::Event<> PresetSwitchEvent;
	DXUI::Event<Homesys::Program*> ProgramScheduleEvent;
	DXUI::Event<Homesys::Program*> ProgramScheduleByMovieAndChannelEvent;
	DXUI::Event<Homesys::Program*> ProgramScheduleByMovieEvent;
	DXUI::Event<Homesys::Program*> ProgramCancelRecordingEvent;
	DXUI::Event<Homesys::Program*> ProgramCancelMovieRecordingEvent;

	DXUI::Event<> DefaultSoundEvent;
};