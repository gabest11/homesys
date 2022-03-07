#include "stdafx.h"
#include "Player.h"
#include "client.h"
#include "../../DirectShow/Filters.h"

// TunerPlayer

TunerPlayer::TunerPlayer(const GUID& tunerId, HRESULT& hr)
	: m_tunerId(tunerId)
	, m_overscan(false)
	, m_disabled(false)
	, m_paused_pos(-1)
	, m_paused_at(-1)
	, m_prevPresetId(0)
{
	m_graph = new CFGManagerPlayer(L"CFGManagerPlayer", NULL, GetSourceConfig(), GetDecoderConfig());

	Homesys::TunerReg tr;

	if(g_env->svc.GetTuner(m_tunerId, tr))
	{
		m_overscan = tr.type == Homesys::TunerDevice::Analog;
	}

	Homesys::TimeshiftFile tf;

	if(!g_env->svc.Activate(m_tunerId, tf))
	{
		hr = E_FAIL;

		return;
	}

	m_source = new CDSMMultiSourceFilter(NULL, &hr);

	hr = m_graph->AddFilter(m_source, L"DSM Multi-Source Filter");

	if(FAILED(hr)) return;

	hr = CComQIPtr<IFileSourceFilter>(m_source)->Load(tf.path.c_str(), NULL);

	if(FAILED(hr)) return;

	if(FAILED(GetOutputs(m_source)))
	{
		hr = E_FAIL;

		return;
	}

	m_tf = tf;

	UpdateDisabled();

	m_thread.TimerPeriod = 2000;
	m_thread.TimerEvent.Add(&TunerPlayer::OnTimer, this);
	m_thread.Create();
}

TunerPlayer::~TunerPlayer()
{
	m_thread.Join();
}

bool TunerPlayer::OnTimer(DXUI::Control* c, UINT64 n)
{
	Homesys::TimeshiftFile tf;
		
	g_env->svc.Activate(m_tunerId, tf);

	if(m_active || clock() - m_lastactivity < 10000)
	{
		if(m_tf.version != tf.version || m_tf.path != tf.path)
		{
			m_tf = tf;

			m_lastactivity = clock();

			PostMessage(m_param.wnd, WM_REACTIVATE_TUNER, 0, (LPARAM)(ITunerPlayer*)this);
		}
	}
	else
	{
		m_tf.version = -1;

		if(m_paused_pos < 0)
		{
			if(m_state == State_Running)
			{
				m_paused_at = clock();
			}

			GetCurrent(m_paused_pos);
		}

		Pause();
	}
			
	UpdateDisabled();

	return true;
}

HRESULT TunerPlayer::Render(IPin* pin)
{
	HRESULT hr;
	
	hr = __super::Render(pin);

	ForeachInterface<IMPADecoderFilter>(m_graph, [] (IBaseFilter* pBF, IMPADecoderFilter* pMDF) -> HRESULT
	{
		pMDF->SetAutoDetectMediaType(true);

		return S_CONTINUE;
	});
	
	return hr;
}

void TunerPlayer::UpdateDisabled()
{
	bool disabled = false;

	// if(g_env->parental.enabled)
	{
		Homesys::Preset preset;

		if(g_env->svc.GetCurrentPreset(m_tunerId, preset))
		{
			if(preset.channelId)
			{
				if(m_channel.id != preset.channelId)
				{
					if(!g_env->svc.GetChannel(preset.channelId, m_channel))
					{
						m_channel = Homesys::Channel();
					}
				}

				if(m_channel.radio)
				{
					disabled = true;
				}
			}

			int rating = g_env->parental.GetMinRating(preset.rating);

			if(rating == 0)
			{
				disabled = true;
			}
			else if(rating < INT_MAX)
			{
				Homesys::Program program;

				REFERENCE_TIME current;
				REFERENCE_TIME start;
				REFERENCE_TIME stop;

				GetCurrent(current);
				GetAvailable(start, stop);

				CTime now =  CTime::GetCurrentTime() - CTimeSpan((stop - current) / 10000000);

				if(g_env->svc.GetCurrentProgram(m_tunerId, 0, program, now))
				{
					if(program.episode.movie.rating >= rating)
					{
						disabled = true;
					}
				}
			}
		}
	}
/*
	if(m_disabled ^ disabled)
	{
		// flush

		Preset preset;

		if(g_env.svc.GetCurrentPreset(m_tunerId, preset))
		{
			g_env.svc.SetPreset(m_tunerId, preset.id);
		}
	}
*/
	m_disabled = disabled;
}

STDMETHODIMP TunerPlayer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(ITunerPlayer)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

// IGenericPlayer

STDMETHODIMP TunerPlayer::GetAvailable(REFERENCE_TIME& start, REFERENCE_TIME& stop)
{
	CheckPointer(m_graph, E_UNEXPECTED);

	HRESULT hr;

	hr = CComQIPtr<IMediaSeeking>(m_graph)->GetAvailable(&start, &stop);

	// stop = std::max<REFERENCE_TIME>(start, stop - 10000000);

	return hr;
}

STDMETHODIMP TunerPlayer::Stop()
{
	g_env->svc.StopRecording(m_tunerId);

	return S_OK;
}

void TunerPlayer::StepPreset(int dir)
{
	std::list<Homesys::Preset> presets;

	if(g_env->svc.GetPresets(m_tunerId, true, presets) || presets.empty())
	{
		if(dir < 0) 
		{
			presets.reverse();
		}

		auto i = presets.end();

		Homesys::Preset preset;

		if(g_env->svc.GetCurrentPreset(m_tunerId, preset))
		{
			i = std::find_if(presets.begin(), presets.end(), [&] (const Homesys::Preset& p) -> bool {return p.id == preset.id;});
		}

		if(i != presets.end())
		{
			i++;
		}

		if(i == presets.end())
		{
			i = presets.begin();
		}

		SetPreset(i->id);
	}
}

STDMETHODIMP TunerPlayer::Prev()
{
	StepPreset(-1);

	return S_OK;
}

STDMETHODIMP TunerPlayer::Next()
{
	StepPreset(+1);

	return S_OK;
}

STDMETHODIMP TunerPlayer::ReRender()
{
	HRESULT hr;

	CComQIPtr<IMediaControl> pMC = m_graph;

	hr = pMC->Stop();

	OAFilterState state;

	hr = pMC->GetState(INFINITE, &state);

	float volume = -1.0f;

	hr = GetVolume(volume);

	bool hasVolume = SUCCEEDED(hr);

	NukeOutputs();

	m_outputs.clear();

	CComQIPtr<IFileSourceFilter> pFSF = m_source;

	pFSF->Load(m_tf.path.c_str(), NULL);

	GetOutputs(m_source);

	hr = __super::ReRender();

	Pause();

	REFERENCE_TIME start;
	REFERENCE_TIME stop;

	GetAvailable(start, stop);

	if(m_paused_pos >= 0)
	{
		stop = m_paused_pos;

		if(m_paused_at >= 0)
		{
			stop += 10000i64 * (clock() - m_paused_at);

			m_paused_at = -1;
		}

		if(stop < start)
		{
			stop = start;
		}

		m_paused_pos = -1;
	}

	Seek(stop);

	if(hasVolume && volume >= 0)
	{
		SetVolume(volume);
	}

	Play();

	return hr;
}

STDMETHODIMP TunerPlayer::GetVideoTexture(IDirect3DTexture9** ppt, Vector2i& ar, Vector4i& src)
{
	if(m_disabled) return E_ACCESSDENIED;

	HRESULT hr = __super::GetVideoTexture(ppt, ar, src);

	if(SUCCEEDED(hr))
	{
		if(m_overscan)
		{
			//ar.SetSize(4, 3);
			src = src.deflate(Vector4i(16, 12, 16, 12));
		}
	}

	return hr;
}

STDMETHODIMP TunerPlayer::Activate(float volume)
{
	g_env->svc.SetPrimaryTuner(m_tunerId, true);

	return __super::Activate(volume);
}

// ITuner

STDMETHODIMP_(GUID) TunerPlayer::GetTunerId()
{
	return m_tunerId;
}

STDMETHODIMP TunerPlayer::SetPreset(int presetId)
{
	int curPresetId = 0;

	g_env->svc.GetCurrentPresetId(m_tunerId, curPresetId);

	g_env->svc.SetPreset(m_tunerId, presetId);

	if(curPresetId != presetId)
	{
		m_prevPresetId = curPresetId;
	}

	// UpdateDisabled();

	return S_OK;
}

STDMETHODIMP TunerPlayer::SetPreviousPreset()
{
	if(m_prevPresetId > 0)
	{
		SetPreset(m_prevPresetId);
	}

	return S_OK;
}

STDMETHODIMP TunerPlayer::Reactivate()
{
	ReRender();

	return S_OK;
}

STDMETHODIMP TunerPlayer::Update()
{
	m_thread.PostTimer();

	return S_OK;
}

// ImagePlayer

ImagePlayer::ImagePlayer(LPCWSTR path, HRESULT& hr)
	: CUnknown(L"ImageRenderer", NULL)
	, m_volume(1.0f)
	, m_state(State_Stopped)
{
	hr = E_FAIL;

	if(wcsstr(path, L"://") != NULL)
	{
		m_items.push_back(path);
	}
	else
	{
		if(!PathFileExists(path))
		{
			return;
		}

		LPCWSTR exts = L".jpeg|.jpg|.gif|.png|.bmp";

		bool valid = false;

		if(LPCWSTR s = PathFindExtension(path))
		{
			std::list<std::wstring> sl;

			Util::Explode(std::wstring(exts), sl, L"|");

			for(auto i = sl.begin(); i != sl.end() && !valid; i++)
			{
				valid = wcsicmp(i->c_str(), s) == 0;
			}
		}

		if(!valid)
		{
			return;
		}

		DXUI::Navigator navigator;

		navigator.Extensions = exts;
		navigator.Location = Util::RemoveFileSpec(path);

		for(auto i = navigator.begin(); i != navigator.end(); i++)
		{
			if(!i->isdir)
			{
				m_items.push_back(navigator.GetPath(&*i));
			}
		}

	}

	m_pos = m_items.end();

	for(auto i = m_items.begin(); i != m_items.end(); i++)
	{
		if(wcscmp(path, i->c_str()) == 0)
		{
			m_pos = i;

			break;
		}
	}

	if(m_pos == m_items.end())
	{
		return;
	}

	hr = Open(0);

	m_thread.TimerPeriod = 1000;

	m_thread.InitEvent.Add0([&] (DXUI::Control* c) -> bool
	{
		return Util::Socket::Startup();
	});

	m_thread.ExitEvent.Add0([&] (DXUI::Control* c) -> bool
	{
		Util::Socket::Cleanup();

		return true;
	});

	m_thread.TimerEvent.Add([&] (DXUI::Control* c, UINT64 n) -> bool
	{
		CAutoLock cAutoLock(&m_thread);

		if(m_state == State_Running)
		{
			if(clock() - m_start >= 5000)
			{
				m_start = INT_MAX;
				m_messages.push_back(EC_COMPLETE);

				PostMessage(m_param.wnd, m_param.msg, 0, (LPARAM)(IGenericPlayer*)this);
			}
		}

		return true;
	});

	m_thread.Create();
}

ImagePlayer::~ImagePlayer()
{
	m_thread.Join();
}

STDMETHODIMP ImagePlayer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IGenericPlayer)
		QI(IImagePlayer)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT ImagePlayer::Open(int step)
{
	CAutoLock cAutoLock(&m_thread);

	if(step > 0)
	{
		m_pos++;

		if(m_pos == m_items.end())
		{
			m_pos = m_items.begin();
		}
	}
	else if(step < 0)
	{
		if(m_pos == m_items.begin())
		{
			m_pos = m_items.end();
		}

		m_pos--;
	}

	m_file = *m_pos;
	m_start = clock();

	HRESULT hr = E_FAIL;

	if(wcsstr(m_file.c_str(), L"://") != NULL)
	{
		Util::Socket s;

		std::map<std::string, std::string> headers;

		if(s.HttpGet(m_file.c_str(), headers, 1, 0))
		{
			if(s.GetContentType().find("image/") == 0)
			{
				std::vector<BYTE> buff;

				if(s.GetContentLength() > 0)
				{
					buff.resize(s.GetContentLength());

					s.ReadAll(buff.data(), buff.size());
				}
				else
				{
					BYTE* tmp = new BYTE[65536];

					int n, i = 0;

					while((n = s.Read(tmp, 65536)) > 0)
					{
						if(i + n > buff.size())
						{
							buff.resize(i + n);
						}

						memcpy(&buff[i], tmp, n);

						i += n;
					}

					delete [] tmp;
				}

				hr = m_id.Open(buff) ? S_OK : E_FAIL;
			}
		}
	}
	else
	{
		hr = m_id.Open(m_file.c_str());
	}

	return hr;
}

// IGenericPlayer

STDMETHODIMP ImagePlayer::GetState(OAFilterState& fs)
{
	fs = m_state;

	return S_OK;
}

STDMETHODIMP ImagePlayer::GetCurrent(REFERENCE_TIME& rt)
{
	rt = 0;

	return S_OK;
}

STDMETHODIMP ImagePlayer::GetAvailable(REFERENCE_TIME& start, REFERENCE_TIME& stop)
{
	start = stop = 0;

	return S_OK;
}

STDMETHODIMP ImagePlayer::Seek(REFERENCE_TIME rt)
{
	return S_OK;
}

STDMETHODIMP ImagePlayer::Play()
{
	CAutoLock cAutoLock(&m_thread);

	m_state = State_Running;
	m_start = clock();

	return S_OK;
}

STDMETHODIMP ImagePlayer::Pause()
{
	CAutoLock cAutoLock(&m_thread);

	m_state = State_Paused;

	return S_OK;
}

STDMETHODIMP ImagePlayer::Stop()
{
	CAutoLock cAutoLock(&m_thread);

	m_state = State_Stopped;

	return S_OK;
}

STDMETHODIMP ImagePlayer::PlayPause()
{
	return S_OK;
}

STDMETHODIMP ImagePlayer::Prev()
{
	HRESULT hr;
	OAFilterState state = m_state;
	hr = Open(-1);
	if(FAILED(hr)) return hr;
	hr = ReRender();
	if(FAILED(hr)) return hr;
	if(state != State_Paused) Play();
	else Pause();
	return S_OK;
}

STDMETHODIMP ImagePlayer::Next()
{
	HRESULT hr;
	OAFilterState state = m_state;
	hr = Open(+1);
	if(FAILED(hr)) return hr;
	hr = ReRender();
	if(FAILED(hr)) return hr;
	if(state != State_Paused) Play();
	else Pause();
	return S_OK;
}

STDMETHODIMP ImagePlayer::FrameStep()
{
	return E_NOTIMPL;
}

STDMETHODIMP ImagePlayer::GetVolume(float& volume)
{
	volume = m_volume;

	return S_OK;
}

STDMETHODIMP ImagePlayer::SetVolume(float volume)
{
	m_volume = volume;

	return S_OK;
}

STDMETHODIMP ImagePlayer::GetTrackCount(int type, int& count)
{
	return E_NOTIMPL;
}

STDMETHODIMP ImagePlayer::GetTrackInfo(int type, int index, TrackInfo& info)
{
	return E_NOTIMPL;
}

STDMETHODIMP ImagePlayer::GetTrack(int type, int& index)
{
	return E_NOTIMPL;
}

STDMETHODIMP ImagePlayer::SetTrack(int type, int index)
{
	return E_NOTIMPL;
}

STDMETHODIMP ImagePlayer::Render(const VideoPresenterParam& param)
{
	m_param = param;

	return ReRender();
}

STDMETHODIMP ImagePlayer::ReRender()
{
	m_texture = NULL;

	HRESULT hr;

	hr = m_param.dev->CreateTexture(m_id.m_width, m_id.m_height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_texture, NULL);

	if(FAILED(hr)) return E_FAIL;

	D3DLOCKED_RECT lr;

	hr = m_texture->LockRect(0, &lr, NULL, 0);

	if(FAILED(hr)) return E_FAIL;

	BYTE* src = m_id.m_pixels;
	BYTE* dst = (BYTE*)lr.pBits;
	int bytes = std::min<int>(m_id.m_pitch, lr.Pitch);

	for(int i = 0; i < m_id.m_height; i++)
	{
		memcpy(dst, src, bytes);

		src += m_id.m_pitch;
		dst += lr.Pitch;
	}

	m_texture->UnlockRect(0);

	return S_OK;
}

STDMETHODIMP ImagePlayer::ResetDevice(bool before)
{
	if(before)
	{
		m_texture = NULL;
	}
	else
	{
		ReRender();
	}

	return S_OK;
}

STDMETHODIMP ImagePlayer::GetVideoTexture(IDirect3DTexture9** ppt, Vector2i& ar, Vector4i& src)
{
	if(m_texture == NULL)
	{
		return E_FAIL;
	}

	(*ppt = m_texture)->AddRef();

	D3DSURFACE_DESC desc;
	
	memset(&desc, 0, sizeof(desc));

	m_texture->GetLevelDesc(0, &desc);

	ar = Vector2i(desc.Width, desc.Height);
	src = Vector4i(0, 0, ar.x, ar.y);

	return S_OK;
}

STDMETHODIMP ImagePlayer::SetVideoPosition(const Vector4i& r)
{
	return S_OK;
}

STDMETHODIMP ImagePlayer::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	return E_NOTIMPL;
}

STDMETHODIMP ImagePlayer::GetEvent(long* plEventCode, LONG_PTR* plParam1, LONG_PTR* plParam2)
{
	CAutoLock cAutoLock(&m_thread);

	if(!m_messages.empty())
	{
		*plEventCode = m_messages.front();
		*plParam1 = 0;
		*plParam2 = 0;

		m_messages.pop_front();
	
		return S_OK;
	}

	return S_FALSE;
}

STDMETHODIMP ImagePlayer::FreeEvent(long lEventCode, LONG_PTR lParam1, LONG_PTR lParam2)
{
	return S_FALSE;
}

STDMETHODIMP ImagePlayer::Activate(float volume)
{
	return S_OK;
}

STDMETHODIMP ImagePlayer::Deactivate()
{
	return S_OK;
}

STDMETHODIMP ImagePlayer::GetFilterGraph(IFilterGraph** ppFG)
{
	return E_UNEXPECTED;
}

// IImagePlayer

STDMETHODIMP_(LPCWSTR) ImagePlayer::GetFileName()
{
	return m_file.c_str();
}

STDMETHODIMP_(int) ImagePlayer::GetWidth()
{
	return m_id.m_width;
}

STDMETHODIMP_(int) ImagePlayer::GetHeight()
{
	return m_id.m_height;
}

// FlashPlayer

FlashPlayer::FlashPlayer(LPCWSTR path, HWND parent, HRESULT& hr)
	: CUnknown(L"Flash", NULL)
	, m_path(path)
	, m_parent(parent)
	, m_state(State_Stopped)
	, m_rect1(0, 0, 1, 1)
	, m_rect2(0, 0, 1, 1)
	, m_loaded(false)
	, m_frames(0)
	, m_thread(false)
{
	hr = E_FAIL;

	if(wcsstr(path, L"flash://") == NULL && wcsstr(path, L".swf") == NULL)
	{
		return;
	}

	m_thread.InitEvent.Add0(&FlashPlayer::OnThreadInit, this);
	m_thread.ExitEvent.Add0(&FlashPlayer::OnThreadExit, this);
	m_thread.MessageEvent.Add(&FlashPlayer::OnThreadMessage, this);
	//m_thread.TimerEvent += new DXUI::EventHandler<Flash, UINT64>(this, &FlashPlayer::OnThreadTimer);
	//m_thread.TimerPeriod = 33;
	m_thread.Create();

	hr = S_OK;
}

FlashPlayer::~FlashPlayer()
{
	m_thread.Post(WM_QUIT);
	m_thread.Join();
}

STDMETHODIMP FlashPlayer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	return 
		QI(IFlashPlayer)
		QI(IFilePlayer)
		QI(__IShockwaveFlashEvents)
		QI(IDispatch)
		__super::NonDelegatingQueryInterface(riid, ppv);
}

bool FlashPlayer::Open()
{
	HRESULT hr;

	m_wnd = CreateWindowEx(0, CAxWindow::GetWndClassName(), 0, WS_POPUP, 0, 0, 1, 1, NULL, 0, 0, 0);

	hr = m_flash.CoCreateInstance(__uuidof(ShockwaveFlash));

/*
	CComQIPtr<IConnectionPointContainer> conptcont = m_flash;

	if(!conptcont)
	{
		return false;
	}

	hr = conptcont->FindConnectionPoint(__uuidof(__IShockwaveFlashEvents), &m_conpt);
	
	if(FAILED(hr) || !m_conpt)
	{
		return false;
	}

	hr = m_conpt->Advise((__IShockwaveFlashEvents*)this, &m_conptid);
	
	if(FAILED(hr))
	{
		return false;
	}
*/
	// hr = m_flash->put_WMode(L"transparent");

	hr = m_flash->put_BackgroundColor(0xffffff);

	// hr = m_flash->put_EmbedMovie(VARIANT_TRUE);

	hr = AtlAxAttachControl(m_flash, m_wnd, 0);

	m_view = m_flash;

	if(!m_view)
	{
		return false;
	}

	hr = CoMarshalInterThreadInterfaceInStream(__uuidof(IViewObject), m_view, &m_stream);

	if(FAILED(hr) || !m_stream)
	{
		return false;
	}

	hr = m_flash->put_Loop(true);

	CComBSTR path(m_path.c_str());

	hr = m_flash->put_Movie(path);

	long frames = 0;

	for(long state = -1; SUCCEEDED(hr) && state != 4 && state != 3; )
	{
		hr = m_flash->get_ReadyState(&state);

		if(state == 4 || state == 3)
		{
			hr = m_flash->get_TotalFrames(&frames);

			m_loaded = true;
			m_frames = frames;
			
			break;
		}

		Sleep(100); // yield
	}

	hr = m_flash->Play();

	return true;
}

void FlashPlayer::Close()
{
	if(m_conpt)
	{
		m_conpt->Unadvise(m_conptid);
	}

	if(m_wnd)
	{
		SendMessage(m_wnd, WM_CLOSE, 0, 0);

		DestroyWindow(m_wnd);
	}

	m_view.Release();
	m_rtview.Release();
	m_conpt = NULL;
	m_stream = NULL;
	m_flash = NULL;
}

bool FlashPlayer::OnThreadInit(DXUI::Control* c)
{
	HRESULT hr = S_OK;

	hr = OleInitialize(0);

	AtlAxWinInit();

	return Open();
}

bool FlashPlayer::OnThreadExit(DXUI::Control* c)
{
	Close();

	OleUninitialize();

	CoFreeUnusedLibraries();

	return true;
}

bool FlashPlayer::OnThreadMessage(DXUI::Control* c, const MSG& msg)
{
	return false;
}

bool FlashPlayer::OnThreadTimer(DXUI::Control* c, UINT64 n)
{
	SetEvent(m_param.flip);

	return true;
}

// IGenericPlayer

STDMETHODIMP FlashPlayer::GetState(OAFilterState& fs)
{
	// TODO

	/*
	VARIANT_BOOL b;

	if(SUCCEEDED(m_flash->IsPlaying(&b)))
	{
		if(!b && m_state == State_Running)
		{
			m_state = State_Stopped;
		}
		else if(b && m_state == State_Stopped)
		{
			m_state = State_Running;
		}
	}

	fs = m_state;
	*/

	fs = State_Running;

	return S_OK;
}

STDMETHODIMP FlashPlayer::GetCurrent(REFERENCE_TIME& rt)
{
	// TODO

	rt = 0;

	return S_OK;
}

STDMETHODIMP FlashPlayer::GetAvailable(REFERENCE_TIME& start, REFERENCE_TIME& stop)
{
	// TODO

	start = stop = 0;

	return S_OK;
}

STDMETHODIMP FlashPlayer::Seek(REFERENCE_TIME rt)
{
	// TODO

	return S_OK;
}

STDMETHODIMP FlashPlayer::Play()
{
	int x = (m_rect2.left + m_rect2.right) / 2;
	int y = (m_rect2.top + m_rect2.bottom) / 2;

	OnMessage(WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
	OnMessage(WM_LBUTTONUP, 0, MAKELPARAM(x, y));

	// TODO

	/*

	if(m_state != State_Running)
	{
		m_flash->Play();
	}

	m_state = State_Running;

	*/

	return S_OK;
}

STDMETHODIMP FlashPlayer::Pause()
{
	// TODO

	/*

	if(m_state == State_Running)
	{
		m_flash->Stop(); // StopPlay?
	}

	m_state = State_Paused;

	*/

	return S_OK;
}

STDMETHODIMP FlashPlayer::Stop()
{
	// TODO

	/*

	m_flash->Stop(); // StopPlay?

	m_state = State_Stopped;

	*/

	return S_OK;
}

STDMETHODIMP FlashPlayer::PlayPause()
{
	// TODO

	Play();

	/*

	if(m_state == State_Running)
	{
		m_flash->Stop(); // StopPlay?
	}
	else
	{
		m_flash->Play();
	}

	m_state = State_Paused;

	*/

	return S_OK;
}

STDMETHODIMP FlashPlayer::GetVolume(float& volume)
{
	// TODO

	return E_NOTIMPL;
}

STDMETHODIMP FlashPlayer::SetVolume(float volume)
{
	// TODO

	return E_NOTIMPL;
}

STDMETHODIMP FlashPlayer::Render(const VideoPresenterParam& param)
{
	m_param = param;

	return ReRender();
}

STDMETHODIMP FlashPlayer::ReRender()
{
	return S_OK;
}

STDMETHODIMP FlashPlayer::ResetDevice(bool before)
{
	if(before)
	{
		m_texture = NULL;
	}
	else
	{
	}

	return S_OK;
}

STDMETHODIMP FlashPlayer::GetVideoTexture(IDirect3DTexture9** ppt, Vector2i& ar, Vector4i& src)
{
	CAutoLock cAutoLock(&m_thread);

	if(!m_loaded) 
	{
		return E_UNEXPECTED;
	}

	if(m_rtview == NULL)
	{
		CoGetInterfaceAndReleaseStream(m_stream, __uuidof(IViewObject), (void**)&m_rtview);

		m_stream.Detach();

		m_texture = NULL;
	}

	Vector4i cr;

	GetClientRect(m_parent, cr);

	m_rect1 = DXUI::DeviceContext::FitRect(cr, Vector2i(1024, 768));

	D3DSURFACE_DESC desc;
	
	memset(&desc, 0, sizeof(desc));
	
	if(m_texture != NULL)
	{
		m_texture->GetLevelDesc(0, &desc);
	}

	if(m_texture == NULL || m_rect1.width() != desc.Width || m_rect1.height() != desc.Height)
	{
		if(m_rect1.rempty())
		{
			m_rect1 = Vector4i(0, 0, 1, 1);
		}

		m_texture = NULL;

		HRESULT hr;

		hr = m_param.dev->CreateTexture(m_rect1.width(), m_rect1.height(), 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_texture, NULL);

		if(FAILED(hr)) return E_FAIL;

		m_texture->GetLevelDesc(0, &desc);

		MoveWindow(m_wnd, 0, 0, m_rect1.width(), m_rect1.height(), FALSE);
	}

	long frame = 0;

	m_flash->CurrentFrame(&frame);

	HDC hDC = NULL;

	CComPtr<IDirect3DSurface9> s;
	
	m_texture->GetSurfaceLevel(0, &s);
	
	Vector4i r = m_rect1.rsize();

	if(SUCCEEDED(s->GetDC(&hDC)))
	{
		SetMapMode(hDC, MM_TEXT);

		HRESULT hr;

		try
		{
			hr = OleDraw(m_rtview, DVASPECT_CONTENT, hDC, r);
		}
		catch(HRESULT hr1)
		{
			hr = hr1;
		}

		s->ReleaseDC(hDC);
	}

	(*ppt = m_texture)->AddRef();

	ar = r.br;
	src = Vector4i(0, 0, desc.Width, desc.Height);

	return S_OK;
}

STDMETHODIMP FlashPlayer::SetVideoPosition(const Vector4i& r)
{
	CAutoLock cAutoLock(&m_thread);

	m_rect2 = r;

	return S_OK;
}

STDMETHODIMP FlashPlayer::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(m_wnd != NULL)
	{
		if(message >= WM_KEYFIRST && message <= WM_KEYLAST 
		|| message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
		{
			if(message >= WM_MOUSEFIRST && message <= WM_MOUSELAST && message != WM_MOUSEWHEEL)
			{
				if(message >= WM_RBUTTONDOWN && message <= WM_RBUTTONDBLCLK)
				{
					return S_OK;
				}

				CAutoLock cAutoLock(&m_thread);

				Vector2i p(LOWORD(lParam), HIWORD(lParam));

				if(!m_rect2.rempty())
				{
					p.x = (p.x - m_rect2.left) * m_rect1.width() / m_rect2.width();
					p.y = (p.y - m_rect2.top) * m_rect1.height() / m_rect2.height();
				}

				lParam = MAKELPARAM(p.x, p.y);
			}

			for(HWND hWnd = GetWindow(m_wnd, GW_CHILD); hWnd; hWnd = GetNextWindow(hWnd, GW_CHILD))
			{
				PostMessage(hWnd, message, wParam, lParam);
			}
		}
	}

	return S_OK;
}

STDMETHODIMP FlashPlayer::GetEvent(long* plEventCode, LONG_PTR* plParam1, LONG_PTR* plParam2)
{
	return S_FALSE;
}

STDMETHODIMP FlashPlayer::FreeEvent(long lEventCode, LONG_PTR lParam1, LONG_PTR lParam2)
{
	return S_FALSE;
}

STDMETHODIMP FlashPlayer::Activate(float volume)
{
	return S_OK;
}

STDMETHODIMP FlashPlayer::Deactivate()
{
	return S_OK;
}

STDMETHODIMP FlashPlayer::GetFilterGraph(IFilterGraph** ppFG)
{
	return E_UNEXPECTED;
}

// IFilePlayer

STDMETHODIMP_(LPCWSTR) FlashPlayer::GetFileName()
{
	return m_path.c_str();
}

// __IShockwaveFlashEvents

STDMETHODIMP FlashPlayer::OnReadyStateChange(long newState)
{
	return S_OK;
}

STDMETHODIMP FlashPlayer::OnProgress(long percentDone)
{
	return S_OK;
}

STDMETHODIMP FlashPlayer::FSCommand(BSTR command, BSTR args)
{
	return S_OK;
}

STDMETHODIMP FlashPlayer::FlashCall(BSTR request)
{
	return S_OK;
}

//#import "progid:InternetExplorer.Application.1" raw_interfaces_only
