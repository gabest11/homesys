#include "stdafx.h"
#include "Environment.h"
#include "client.h"
#include "../../DirectShow/IKeyFrameInfo.h"
#include "../../DirectShow/AudioSwitcher.h"

using namespace std::tr1;

Environment::Environment(HINSTANCE hInstance)
	: m_hInstance(hInstance)
	, m_hotlinks_invalid(true)
{
	#pragma region Events

	PrevPlayerEvent.Add0(&Environment::OnPrevPlayer, this);
	NextPlayerEvent.Add0(&Environment::OnNextPlayer, this);
	PrevPlayerEvent.Add0(&Environment::OnPrevNextPlayer, this);
	NextPlayerEvent.Add0(&Environment::OnPrevNextPlayer, this);
	PrevEvent.Add0(&Environment::OnPrev, this);
	NextEvent.Add0(&Environment::OnNext, this);
	SkipBackwardEvent.Add0(&Environment::OnSkipBackward, this);
	SkipForwardEvent.Add0(&Environment::OnSkipForward, this);
	StopEvent.Add0(&Environment::OnStop, this);
	PauseEvent.Add0(&Environment::OnPause, this);
	PlayEvent.Add0(&Environment::OnPlay, this);
	PlayPauseEvent.Add0(&Environment::OnPlayPause, this);
	ReturnEvent.Add0(&Environment::OnReturn, this);
	RecordEvent.Add0(&Environment::OnRecord, this);
	SeekAsyncEvent.Add(&Environment::OnSeekAsync, this);
	SeekSyncEvent.Add(&Environment::OnSeekSync, this);

	TrackSwitchEvent.Add(&Environment::OnTrackSwitch, this);
	VideoSwitchEvent.Add0(&Environment::OnVideoSwitch, this);
	AudioSwitchEvent.Add0(&Environment::OnAudioSwitch, this);
	SubtitleSwitchEvent.Add0(&Environment::OnSubtitleSwitch, this);
	
	PresetSwitchEvent.Add0(&Environment::OnPresetSwitch, this);
	PresetSwitchEvent.Chain(VideoClickedEvent);
	ProgramScheduleEvent.Add(&Environment::OnProgramSchedule, this);
	ProgramScheduleByMovieEvent.Add(&Environment::OnProgramScheduleByMovie, this);
	ProgramScheduleByMovieAndChannelEvent.Add(&Environment::OnProgramScheduleByMovieAndChannel, this);
	ProgramCancelRecordingEvent.Add(&Environment::OnProgramCancelRecording, this);
	ProgramCancelMovieRecordingEvent.Add(&Environment::OnProgramCancelMovieRecording, this);
	
	DefaultSoundEvent.Add0(&Environment::OnDefaultSoundEvent, this);

	function<bool()> pc = [&] () -> bool {return !players.empty();};

	PrevPlayerEvent.Guard(pc);
	NextPlayerEvent.Guard(pc);
	PrevEvent.Guard(pc);
	NextEvent.Guard(pc);
	SkipBackwardEvent.Guard(pc);
	SkipForwardEvent.Guard(pc);
	StopEvent.Guard(pc);
	PauseEvent.Guard(pc);
	PlayEvent.Guard(pc);
	PlayPauseEvent.Guard(pc);
	RecordEvent.Guard(pc);
	SeekAsyncEvent.Guard(pc);
	SeekSyncEvent.Guard(pc);
	TrackSwitchEvent.Guard(pc);
	VideoSwitchEvent.Guard(pc);
	AudioSwitchEvent.Guard(pc);
	SubtitleSwitchEvent.Guard(pc);
	VideoClickedEvent.Guard(pc);
	PresetSwitchEvent.Guard(pc);

	#pragma endregion

	#pragma region Properties

	PositionRT.Get(&Environment::GetPositionRT, this);
	PositionF.Get(&Environment::GetPositionF, this);
	PositionText.Get(&Environment::GetPositionText, this);
	DurationRT.Get(&Environment::GetDurationRT, this);
	DurationText.Get(&Environment::GetDurationText, this);
	Muted.Set(&Environment::SetMuted, this);
	Volume.Set(&Environment::SetVolume, this);
	Beep.Get(&Environment::GetBeep, this);
	Beep.Set(&Environment::SetBeep, this);

	AppPath.Get(&Environment::GetAppPath, this);
	AppDataPath.Get(&Environment::GetAppDataPath, this);
	CachePath.Get(&Environment::GetCachePath, this);
	ResourcePath.Get(&Environment::GetResourcePath, this);
	TempPath.Get(&Environment::GetTempPath, this);

	Muted = false;
	Volume = 1.0f;
	Beep = Util::Config(L"Homesys", L"Settings").GetInt(L"Beep", 1) != 0;

	#pragma endregion

	//

	svc.GetParentalSettings(parental);

	std::wstring lang;

	if(svc.GetLanguage(lang))
	{
		Util::Config(L"Homesys", L"Settings").SetString(L"UILang", lang.c_str());
	}
	else
	{
		lang = Util::Config(L"Homesys", L"Settings").GetString(L"UILang", L"");
	}

	if(!lang.empty())
	{
		DXUI::Control::SetLanguage(lang.c_str());
	}

	UpdateServiceState();

	m_thread.TimerEvent.Add(&Environment::OnThreadTimer, this);
	m_thread.TimerPeriod = 1000;
	m_thread.TimerStartAfter = 3;
	m_thread.Create();

	SetThreadPriority(m_thread, THREAD_PRIORITY_BELOW_NORMAL);
}

Environment::~Environment()
{
	m_thread.Join();

	for(auto i = hotlinks.begin(); i != hotlinks.end(); i++)
	{
		delete *i;
	}

	hotlinks.clear();
}

void Environment::GetAppPath(std::wstring& path)
{
	if(path.empty())
	{
		wchar_t buff[MAX_PATH + 1] = {0};

		GetModuleFileName(m_hInstance, buff, MAX_PATH);

		PathRemoveFileSpec(buff);

		path = buff;
	}
}

void Environment::GetAppDataPath(std::wstring& path)
{
	if(path.empty())
	{
		wchar_t buff[MAX_PATH + 1] = {0};

		SHGetSpecialFolderPath(NULL, buff, CSIDL_COMMON_APPDATA, TRUE);

		if(wcslen(buff) > 0)
		{
			PathAppend(buff, L"Homesys");

			CreateDirectory(buff, NULL);

			path = buff;
		}
		else
		{
			ASSERT(0);

			path = AppPath;
		}
	}
}

void Environment::GetCachePath(std::wstring& path)
{
	if(path.empty())
	{
		wchar_t buff[MAX_PATH + 1] = {0};

		wcscpy(buff, AppDataPath->c_str());

		if(wcslen(buff) > 0)
		{
			PathAppend(buff, L"Cache");

			CreateDirectory(buff, NULL);

			path = buff;
		}
		else
		{
			ASSERT(0);

			path = AppPath;
		}
	}
}

void Environment::GetResourcePath(std::wstring& path)
{
	if(path.empty())
	{
		std::list<std::wstring> paths;

		std::wstring s = AppPath;

		paths.push_back(s + L"\\res");
		paths.push_back(s + L"\\..\\..\\homesys");

		for(auto i = paths.begin(); i != paths.end(); i++)
		{
			if(PathIsDirectory(i->c_str()))
			{
				path = *i;

				break;
			}
		}
	}
}

void Environment::GetTempPath(std::wstring& path)
{
	if(path.empty())
	{
		wchar_t buff[MAX_PATH + 1] = {0};

		::GetTempPath(MAX_PATH, buff);

		path = buff;

		ASSERT(!path.empty());
	}
}

void Environment::GetPositionRT(REFERENCE_TIME& value)
{
	value = ps.GetCurrent() - ps.start;
}

void Environment::GetPositionF(float& value)
{
	value = 0;

	if(ps.stop > ps.start)
	{
		double pos = (double)((ps.GetCurrent() - ps.start) / 10000);
		double dur = (double)((ps.stop - ps.start) / 10000);

		value = (float)(pos / dur);
		value = std::min<float>(std::max<float>(value, 0.0f), 1.0f);
	}
}

void Environment::GetPositionText(std::wstring& value)
{
	DVD_HMSF_TIMECODE tc = DirectShow::RT2HMSF(ps.GetCurrent() - ps.start);

	value = Util::Format(L"%d:%02d:%02d", tc.bHours, tc.bMinutes, tc.bSeconds);
}

void Environment::GetDurationRT(REFERENCE_TIME& value)
{
	value = ps.stop - ps.start;
}

void Environment::GetDurationText(std::wstring& value)
{
	DVD_HMSF_TIMECODE tc = DirectShow::RT2HMSF(ps.stop - ps.start);

	value = Util::Format(L"%d:%02d:%02d", tc.bHours, tc.bMinutes, tc.bSeconds);
}

void Environment::SetMuted(bool& value)
{
	if(!players.empty())
	{
		players.front()->SetVolume(!value ? Volume : 0.0f);
	}
}

void Environment::SetVolume(float& value)
{
	value = std::min<float>(std::max<float>(value, 0.0f), 1.0f);

	if(!players.empty())
	{
		players.front()->SetVolume(!Muted ? value : 0);
	}
}

void Environment::GetBeep(bool& value)
{
	value = m_beep;
}

void Environment::SetBeep(bool& value)
{
	m_beep = value;

	Util::Config(L"Homesys", L"Settings").SetInt(L"Beep", value ? 1 : 0);
}

bool Environment::OnPrevPlayer(DXUI::Control* c)
{
	players.push_front(players.back());
	players.pop_back();

	return true;
}

bool Environment::OnNextPlayer(DXUI::Control* c)
{
	players.push_back(players.front());
	players.pop_front();

	return true;
}

bool Environment::OnPrevNextPlayer(DXUI::Control* c)
{
	if(players.size() > 1)
	{
		auto i = players.begin();

		while(++i != players.end())
		{
			(*i)->Deactivate();
		}

		players.front()->Activate(!Muted ? Volume : 0.0f);
	}

	return true;
}

bool Environment::OnPrev(DXUI::Control* c)
{
	players.front()->Prev();

	InfoEvent(NULL);

	return true;
}

bool Environment::OnNext(DXUI::Control* c)
{
	players.front()->Next();

	InfoEvent(NULL);

	return true;
}

bool Environment::OnSkipBackward(DXUI::Control* c)
{
	bool keyframe = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);

	SeekByOffset(-50000000, keyframe);

	return true;
}

bool Environment::OnSkipForward(DXUI::Control* c)
{
	bool keyframe = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);

	SeekByOffset(+50000000, keyframe);

	return true;
}

bool Environment::OnStop(DXUI::Control* c)
{
	players.front()->Stop();

	return true;
}

bool Environment::OnPause(DXUI::Control* c)
{
	CComPtr<IGenericPlayer> player = players.front();

	OAFilterState fs;
	
	if(SUCCEEDED(player->GetState(fs)) && fs == State_Paused)
	{
		player->FrameStep();
	}
	else 
	{
		player->Pause();
	}

	return true;
}

bool Environment::OnPlay(DXUI::Control* c)
{
	players.front()->Play();

	return true;
}

bool Environment::OnPlayPause(DXUI::Control* c)
{
	players.front()->PlayPause();

	return true;
}

bool Environment::OnReturn(DXUI::Control* c)
{
	CComPtr<IGenericPlayer> player = players.front();

	if(CComQIPtr<ITunerPlayer> t = player)
	{
		t->SetPreviousPreset();
	}

	return true;
}

bool Environment::OnRecord(DXUI::Control* c)
{
	CComPtr<IGenericPlayer> player = players.front();

	if(CComQIPtr<ITunerPlayer> t = player)
	{
		svc.Record(t->GetTunerId());
	}

	return true;
}

static clock_t s_seekto_time = 0; // TODO

bool Environment::OnSeekAsync(DXUI::Control* c, float value)
{
	CComPtr<IGenericPlayer> player = players.front();

	REFERENCE_TIME start = 0;
	REFERENCE_TIME stop = 0;

	if(SUCCEEDED(player->GetAvailable(start, stop)))
	{
		REFERENCE_TIME rt = (REFERENCE_TIME)(value * (stop - start) + start);
		
		if(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
		{
			FindKeyFrame(player, rt);
		}

		ps.seekto = rt;
	
		s_seekto_time = clock();
	}

	return true;
}

bool Environment::OnSeekSync(DXUI::Control* c, float value)
{
	CComPtr<IGenericPlayer> player = players.front();

	REFERENCE_TIME start = 0;
	REFERENCE_TIME stop = 0;

	if(SUCCEEDED(player->GetAvailable(start, stop)))
	{
		REFERENCE_TIME rt = (REFERENCE_TIME)(value * (stop - start) + start);
		
		if(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
		{
			FindKeyFrame(player, rt);
		}

		ps.seekto = -1;
		ps.current = rt;

		player->Seek(rt);
	}

	return true;
}

bool Environment::OnTrackSwitch(DXUI::Control* c, int id)
{
	CComPtr<IGenericPlayer> player = players.front();

	int count;

	if(SUCCEEDED(player->GetTrackCount(id, count)) && count > 1)
	{
		int index;

		if(SUCCEEDED(player->GetTrack(id, index)))
		{
			player->SetTrack(id, ((index + 1) % count));

			InfoEvent(NULL);
		}
	}

	return true;
}

bool Environment::OnVideoSwitch(DXUI::Control* c)
{
	return TrackSwitchEvent(c, TrackType::Video);
}

bool Environment::OnAudioSwitch(DXUI::Control* c)
{
	return TrackSwitchEvent(c, TrackType::Audio);
}

bool Environment::OnSubtitleSwitch(DXUI::Control* c)
{
	return TrackSwitchEvent(c, TrackType::Subtitle);
}

bool Environment::OnPresetSwitch(DXUI::Control* c)
{
	CComPtr<IGenericPlayer> player = players.front();

	if(CComQIPtr<ITunerPlayer> t = player)
	{
		t->SetPreset((int)c->UserData);
	}

	return true;
}

bool Environment::OnProgramSchedule(DXUI::Control* c, Homesys::Program* program)
{
	GUID recordingId;

	svc.RecordByProgram(program->id, false, recordingId);

	return true;
}

bool Environment::OnProgramScheduleByMovie(DXUI::Control* c, Homesys::Program* program)
{
	GUID id;

	svc.RecordByMovie(program->episode.movie.id, 0, id);

	return true;
}

bool Environment::OnProgramScheduleByMovieAndChannel(DXUI::Control* c, Homesys::Program* program)
{
	GUID id;

	svc.RecordByMovie(program->episode.movie.id, program->channelId, id);

	return true;
}

bool Environment::OnProgramCancelRecording(DXUI::Control* c, Homesys::Program* program)
{
	std::list<Homesys::Recording> recordings;

	svc.GetRecordings(recordings);

	for(auto i = recordings.begin(); i != recordings.end(); i++)
	{
		const Homesys::Recording& r = *i;

		if(r.program.id == program->id)
		{
			svc.DeleteRecording(r.id);
		}
	}

	return true;
}

bool Environment::OnProgramCancelMovieRecording(DXUI::Control* c, Homesys::Program* program)
{
	svc.DeleteMovieRecording(program->episode.movie.id);

	std::list<Homesys::Recording> recordings;

	svc.GetRecordings(recordings);

	for(auto i = recordings.begin(); i != recordings.end(); i++)
	{
		const Homesys::Recording& r = *i;

		if(r.state == Homesys::RecordingState::Scheduled && r.program.episode.movie.id == program->episode.movie.id)
		{
			svc.DeleteRecording(r.id);
		}
	}

	return true;
}

bool Environment::OnDefaultSoundEvent(DXUI::Control* c)
{
	if(m_beep)
	{
		std::wstring s = ResourcePath;

		std::wstring s1 = s + L"\\default.wav";
		std::wstring s2 = s + L"\\setup\\default.wav";

		if(PathFileExists(s1.c_str())) s = s1;
		else if(PathFileExists(s2.c_str())) s = s2;
		else return false;

		BOOL b = PlaySound(s.c_str(), NULL, SND_ASYNC | SND_FILENAME | SND_NOSTOP | SND_NODEFAULT);
	}

	return true;
}

bool Environment::OnThreadTimer(DXUI::Control* c, UINT64 n)
{
	UpdateServiceState();

	{
		CComPtr<IGenericPlayer> player;

		{
			CAutoLock cAutoLock(&DXUI::Control::m_lock);

			if(!players.empty())
			{
				player = players.front();
			}
		}

		if(player != NULL)
		{
			if(CComQIPtr<ITunerPlayer> t = player)
			{
				bool recording = svc.IsRecording(t->GetTunerId());
				
				Homesys::Preset preset;
				Homesys::Channel channel;

				svc.GetCurrentPreset(t->GetTunerId(), preset);

				if(preset.channelId)
				{
					svc.GetChannel(preset.channelId, channel);
				}

				Homesys::TunerStat stat;

				svc.GetTunerStat(t->GetTunerId(), stat);
				
				CAutoLock cAutoLock(&DXUI::Control::m_lock);

				ps.tuner.recording = recording;
				ps.tuner.preset = preset;
				ps.tuner.channel = channel;
				ps.tuner.stat = stat;
			}
		}
	}

	{
		CTime now = CTime::GetCurrentTime();

		if((n & 7) == 0)
		{
			std::list<Homesys::Recording> recordings;
			std::list<Homesys::Recording> tmp;

			if(svc.GetRecordings(Homesys::RecordingState::Scheduled, tmp))
			{
				recordings.insert(recordings.end(), tmp.begin(), tmp.end());
			}

			if(svc.GetRecordings(Homesys::RecordingState::Warning, tmp))
			{
				recordings.insert(recordings.end(), tmp.begin(), tmp.end());
			}

			Homesys::Recording recording;

			for(auto i = recordings.begin(); i != recordings.end(); i++)
			{
				const Homesys::Recording& r = *i;

				if(r.state == Homesys::RecordingState::Scheduled || r.state == Homesys::RecordingState::Warning)
				{
					CTimeSpan delay = r.startDelay;

					if(r.state == Homesys::RecordingState::Warning) 
					{
						delay = CTimeSpan(0);
					}

					CTime stop = r.start + delay;
					CTime start = stop - CTimeSpan(0, 0, 1, 0);

					if(now >= start && now < stop)
					{
						recording = r;
						recording.startDelay = delay;

						break;
					}
				}
			}

			CAutoLock cAutoLock(&DXUI::Control::m_lock);

			ps.recording = recording;
		}
	}

	return true;
}

void Environment::UpdateServiceState()
{
	bool connected = false;
	bool online = false;
	Homesys::MachineReg machine;

	if(svc.IsConnected())
	{
		connected = true;
		online = svc.IsOnline();
		svc.GetMachine(machine);

		Homesys::ServiceMessage msg;

		if(svc.PopServiceMessage(msg) && msg.id != 0) 
		{
			CAutoLock cAutoLock(&DXUI::Control::m_lock);

			ss.messages.push(msg);
		}
	}

	CAutoLock cAutoLock(&DXUI::Control::m_lock);

	ss.connected = connected;
	ss.online = online;
	ss.machine = machine;
}

static int RangeBSearch(REFERENCE_TIME t, std::vector<REFERENCE_TIME>& rts)
{
	int i = 0;
	int j = rts.size() - 1;
	int ret = -1;

	if(j >= 0 && t >= rts[j]) 
	{
		return j;
	}

	while(i < j)
	{
		int mid = (i + j) >> 1;

		REFERENCE_TIME midt = rts[mid];
		
		if(t == midt) {ret = mid; break;}
		else if(t < midt) {ret = -1; if(j == mid) mid--; j = mid;}
		else if(t > midt) {ret = mid; if(i == mid) mid++; i = mid;}
	}

	return ret;
}

void Environment::AutoSeek()
{
	if(ps.seekto >= 0)
	{
		if((clock() - s_seekto_time) >= 1000)
		{
			if(!players.empty())
			{
				players.front()->Seek(ps.seekto);
			}

			ps.current = ps.seekto;
			ps.seekto = -1;

			s_seekto_time = 0;
		}
	}
}

void Environment::SeekByOffset(REFERENCE_TIME dt, bool keyframe)
{
	CComPtr<IGenericPlayer> player = players.front();

	REFERENCE_TIME rt = 0;
	REFERENCE_TIME start = 0;
	REFERENCE_TIME stop = 0;

	if(SUCCEEDED(player->GetCurrent(rt)) && SUCCEEDED(player->GetAvailable(start, stop)))
	{
		if(ps.seekto >= 0)
		{
			REFERENCE_TIME diff = abs(rt - ps.seekto);

			if(diff >= 3000000000ui64) dt *= 4; // 5 m
			else if(diff >= 600000000ui64) dt *= 2; // 1 m

			rt = ps.seekto;
		}

		if(keyframe)
		{
			FindKeyFrame(player, rt, dt);
		}
		else
		{
			rt += dt;
		}

		rt = std::min<REFERENCE_TIME>(rt, stop - 10000000);
		rt = std::max<REFERENCE_TIME>(rt, start);

		ps.seekto = rt;
	
		s_seekto_time = clock();
	}
}

void Environment::FindKeyFrame(IGenericPlayer* player, REFERENCE_TIME& rt, REFERENCE_TIME dt)
{
	CComPtr<IFilterGraph> graph;
			
	player->GetFilterGraph(&graph);

	std::vector<REFERENCE_TIME> rts;

	ForeachInterface<IKeyFrameInfo>(graph, [&] (IBaseFilter* pBF, IKeyFrameInfo* pKFI) -> HRESULT
	{
		UINT n = 0;

		if(SUCCEEDED(pKFI->GetKeyFrameCount(n)) && n > 0)
		{
			rts.resize(n);

			if(FAILED(pKFI->GetKeyFrames(&TIME_FORMAT_MEDIA_TIME, rts.data(), n)))
			{
				rts.clear();
			}
		}

		return S_OK;
	});

	if(rts.empty())
	{
		return;
	}

	if(dt < 0)
	{
		REFERENCE_TIME offset = 1;

		int i = RangeBSearch(rt, rts);

		if(i > 0)
		{
			offset = std::min<REFERENCE_TIME>(rt - rts[i - 1], 10000000);
			offset = std::max<REFERENCE_TIME>(offset, 0);
		}

		rt = std::max<REFERENCE_TIME>(rt - offset, 0);
	}

	int i = RangeBSearch(rt, rts);

	if(dt < 0)
	{
		rt = i >= 0 ? rts[i] : 0;
	}
	else
	{
		if(i < 0 || i >= rts.size() - 1) 
		{
			return;
		}

		rt = rts[i + 1];
	}

	rt += 10; // fixes calculation errors
}

std::wstring Environment::FormatMoviePictureUrl(int id)
{
	return Util::Format(L"http://homesyslite.timbuktu.hu/service/image.php?movieId=%d", id);
}

DXUI::Texture* Environment::GetAudioTexture(IGenericPlayer* player, DXUI::DeviceContext* dc)
{
	CComPtr<IFilterGraph> graph;

	if(SUCCEEDED(player->GetFilterGraph(&graph)))
	{
		IAudioSwitcherFilter* pASF = NULL;

		ForeachInterface<IAudioSwitcherFilter>(graph, [&] (IBaseFilter* pBF, IAudioSwitcherFilter* asf) -> HRESULT
		{
			pASF = asf;

			return S_OK;
		});

		if(pASF != NULL)
		{
			std::vector<float> buff(32);

			if(m_fft_max.size() != buff.size())
			{
				m_fft_max.resize(buff.size());
				m_fft_ttl.resize(buff.size());

				for(int i = 0; i < buff.size(); i++)
				{
					m_fft_max[i] = buff[i];
					m_fft_ttl[i] = clock();
				}
			}

			float volume = 0;

			if(S_OK == pASF->GetRecentFrequencies(buff.data(), buff.size(), volume))
			{
				volume = std::min<float>(volume * 3, 1.0f);
				
				//

				DXUI::Device* dev = dc->GetDevice();

				DXUI::Texture* t = dev->CreateRenderTarget(1024, 1024, false); // TODO

				DXUI::Texture* rt = dc->SetTarget(t);

				int w = t->GetWidth();
				int h = t->GetHeight();

				dc->Draw(Vector4i(0, 0, w, h), 0x20000000);

				//

				for(int i = 0; i < buff.size(); i++)
				{
					if(m_fft_max[i] < buff[i])
					{
						m_fft_max[i] = buff[i];
						m_fft_ttl[i] = clock();
					}
				}

				DXUI::Vertex* vertices = (DXUI::Vertex*)_aligned_malloc(sizeof(DXUI::Vertex) * w * 2, 16);

				int index_prev = -1;

				int j = 0;

				for(int i = 0; i < w; i++)
				{
					float d = (float)i / w;

					int index = (int)(d * buff.size());

					if(index == index_prev)
					{
						float s = buff[index];

						DWORD c = 0xff000000;

						{
							float r, g, b;

							float ch = (float)i / w;
							float cs = 1;
							float cv = 1;

							if(cs == 0)
							{
								r = g = b = cv;
							}
							else
							{
								float h = ch * 6;
								int i = (int)floor(h);
								float p = cv * (1.0f - cs);
								float q = cv * (1.0f - cs * (h - i));
								float t = cv * (1.0f - cs * (1.0f - (h - i)));
											
								switch(i)
								{
								case 0: r = cv; g = t; b = p; break;
								case 1: r = q; g = cv; b = p; break;
								case 2: r = p; g = cv; b = t; break;
								case 3: r = p; g = q; b = cv; break;
								case 4: r = t; g = p; b = cv; break;
								default: r = cv; g = p; b = q; break;
								}
							}

							c |= ((DWORD)(b * 255) << 16) | ((DWORD)(g * 255) << 8) | (DWORD)(r * 255);
						}

						vertices[j * 2 + 0].p = Vector4((float)i, (float)(h), 0.5f, 1.0f);
						vertices[j * 2 + 0].c.x = vertices[j * 2 + 0].c.y = c;
						vertices[j * 2 + 1].p = Vector4((float)i, (float)(h) - s, 0.5f, 1.0f);
						vertices[j * 2 + 1].c.x = vertices[j * 2 + 1].c.y = c;

						j++;
					}
					else
					{
						i += 3;
					}

					index_prev = index;
				}

				dc->DrawLineList(vertices, j);

				index_prev = -1;

				j = 0;

				for(int i = 0; i < w; i++)
				{
					float d = (float)i / w;

					int index = (int)(d * buff.size());

					if(index == index_prev)
					{
						float s = m_fft_max[index];

						DWORD c = 0xffffffff;

						vertices[j * 2 + 0].p = Vector4((float)i, (float)(h) - s - 20, 0.5f, 1.0f);
						vertices[j * 2 + 0].c.x = vertices[j * 2 + 0].c.y = c;
						vertices[j * 2 + 1].p = Vector4((float)i, (float)(h) - s, 0.5f, 1.0f);
						vertices[j * 2 + 1].c.x = vertices[j * 2 + 1].c.y = c;

						j++;
					}
					else
					{
						i += 3;
					}

					index_prev = index;
				}

				dc->DrawLineList(vertices, j);

				_aligned_free(vertices);

				for(int i = 0; i < buff.size(); i++)
				{
					int diff = clock() - m_fft_ttl[i];

					if(m_fft_max[i] > 0)
					{
						m_fft_max[i] += diff >> 7;

						if(m_fft_max[i] < 1 || m_fft_max[i] > h) 
						{
							m_fft_max[i] = 0;
						}
					}
				}

				//

				dc->SetTarget(rt);

				return t;
			}
		}
	}

	return NULL;
}
