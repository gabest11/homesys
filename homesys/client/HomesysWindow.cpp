#include "StdAfx.h"
#include "HomesysWindow.h"
#include "client.h"
#include "resource.h"
#include "../../DirectShow/DSMMultiSource.h"
#include "../../DirectShow/TeletextFilter.h"
#include "../../DirectShow/AudioSwitcher.h"
#include "../../DirectShow/Filters.h"
#include "../../DirectShow/MediaTypeEx.h"

#pragma warning(disable: 4355)

#define WM_USERINPUT (WM_PLAYERWINDOW_LAST + 1)
#define WM_UPNP_MEDIA_RENDERER (WM_PLAYERWINDOW_LAST + 2)

// TODO
#define CONFIG_KEY L"Homesys"

HomesysWindow::HomesysWindow(LoaderThread* loader, DWORD flags)
	: PlayerWindow(CONFIG_KEY, loader, flags)
	, m_arpreset_changed(0)
	, m_fullrange_changed(0)
	, m_callback(NULL)
	, m_debug(0)
{
	Util::Config cfg(CONFIG_KEY, L"Settings");

	m_fullrange = cfg.GetInt(L"FullRange", 1) != 0;
	m_pip = cfg.GetInt(L"PIP", 1) != 0;
	m_arpreset = cfg.GetInt(L"AspectRatio", 0);

	m_tv.broadcast = false;
	m_tv.streaming = false;

	Control::UserActionEvent.Chain(g_env->DefaultSoundEvent);
	Menu::NavigateBackEvent.Add0([&] (Control* c) -> bool {m_mgr->Close(); return true;});
	Menu::ExitEvent.Add0([&] (Control* c) -> bool {PostMessage(m_hWnd, WM_CLOSE, 0, 0); return true;});
	Edit::EditTextEvent.Add0(&HomesysWindow::OnEditText, this);

	g_env->InfoEvent.Add0(&HomesysWindow::OnFileInfo, this);

	g_env->PlayFileEvent.Add(&HomesysWindow::OnPlayFile, this);
	g_env->OpenFileEvent.Add(&HomesysWindow::OnOpenFile, this);
	g_env->CloseFileEvent.Add0(&HomesysWindow::OnPlayerClose, this);
	g_env->MuteEvent.Add0(&HomesysWindow::OnMute, this);
	g_env->FullRangeEvent.Add0(&HomesysWindow::OnFullRange, this);
	g_env->AspectRatioEvent.Add0(&HomesysWindow::OnAspectRatio, this);
	g_env->PIPEvent.Add0(&HomesysWindow::OnPIP, this);
	g_env->VideoClickedEvent.Add0(&HomesysWindow::OnVideoClicked, this);
	g_env->RegisterUserEvent.Add0(&HomesysWindow::OnRegisterUser, this);
	g_env->RecordClickedEvent.Add0(&HomesysWindow::OnRecordClicked, this);
	g_env->ChannelDetailsEvent.Add0(&HomesysWindow::OnChannelDetails, this);
	g_env->ModifyRecording.Add(&HomesysWindow::OnModifyRecording, this);
	g_env->TeletextEvent.Add0(&HomesysWindow::OnTeletext, this); 

	/*
	g_env->EditFileEvent += bind(&HomesysWindow::OnEditFile, this, _1, _2);
	g_env->FileInfoRequestEvent += bind(&HomesysWindow::OnFileInfoRequest, this, _1, _2);
	g_env->FileInfoEvent += bind(&HomesysWindow::OnFileInfo, this, _1, _2);
	g_env->CinemaShowtimeEvent += bind(&HomesysWindow::OnCinemaShowtime, this, _1, _2);
	*/

	m_menu = new HomesysMenu();

	Vector4i r(0, 0, 1024, 768);
//	Vector4i r(0, 0, 1920, 1080);

	m_menu->main.Create(r, NULL);
	m_menu->tv.Create(r, NULL);
	m_menu->tvosd.Create(r, NULL);
	m_menu->webtvosd.Create(r, NULL);
	m_menu->presets.Create(r, NULL);
	m_menu->presetsorder.Create(r, NULL);
	m_menu->record.Create(r, NULL);
	m_menu->recordings.Create(r, NULL);	
	m_menu->recordingmodify.Create(r, NULL);	
	m_menu->wishlist.Create(r, NULL);
	m_menu->epg.Create(r, NULL);	
	m_menu->epgchannel.Create(r, NULL);
	m_menu->epgsearch.Create(r, NULL);
	m_menu->dvd.Create(r, NULL);
	m_menu->audiocd.Create(r, NULL);
	m_menu->tuner.Create(r, NULL);
	m_menu->smartcard.Create(r, NULL);
	m_menu->tuning.Create(r, NULL);	
	m_menu->fileosd.Create(r, NULL);
	m_menu->edittext.Create(r, NULL);
	m_menu->file.Create(r, NULL);
	m_menu->image.Create(r, NULL);
	m_menu->settings.Create(r, NULL);
	m_menu->personal.Create(r, NULL);
	m_menu->video.Create(r, NULL);
	m_menu->audio.Create(r, NULL);
	m_menu->subtitle.Create(r, NULL);
	m_menu->hotlink.Create(r, NULL);
	m_menu->signup.Create(r, NULL);
	m_menu->parental.Create(r, NULL);
	m_menu->teletext.Create(Vector4i(0, 0, 520, 400), NULL);

	m_menu->main.m_list.MenuClickedEvent.Add(&HomesysWindow::OnMenuMainClicked, this);
	m_menu->tv.m_list.MenuClickedEvent.Add(&HomesysWindow::OnMenuTVClicked, this);
	m_menu->tv.ActivatingEvent.Add0(&HomesysWindow::OnMenuTVActivating, this);
	m_menu->tvosd.ActivatingEvent.Add0(&HomesysWindow::OnMenuTVActivating, this);
	m_menu->webtvosd.ActivatingEvent.Add0(&HomesysWindow::OnMenuTVActivating, this);	
	m_menu->tuner.ActivatingEvent.Add0(&HomesysWindow::OnPlayerClose, this);
	m_menu->tuning.ActivatingEvent.Add0(&HomesysWindow::OnPlayerClose, this);
	m_menu->presets.ActivatingEvent.Add0(&HomesysWindow::OnMenuTVActivating, this);
	m_menu->dvd.m_list.MenuClickedEvent.Add(&HomesysWindow::OnMenuDVDClicked, this);
	m_menu->signup.DoneEvent.Add0([&] (Control* c) -> bool {m_mgr->Close(); m_mgr->Open(&m_menu->settings); return true;});
}

HomesysWindow::~HomesysWindow()
{
	delete m_mr;
	delete m_callback;
	delete m_menu;
}

void HomesysWindow::PreCreate(WNDCLASS* wc, DWORD& style, DWORD& exStyle)
{
	wc->hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_HOMESYS));
}

HRESULT HomesysWindow::OpenFile(LPCTSTR path)
{
	HRESULT hr = E_FAIL;

	ClosePlayers();

	wprintf(L"OpenFile %s\n", path); 

	g_env->ps.dvd.error = 0;

	std::wstring url = path;

	Homesys::HttpMedia m;

	if(url.find(L"http://") == 0)
	{
		Homesys::HttpMediaDetector md;

		if(md.Open(url.c_str(), m))
		{
			url = m.url;

			wprintf(L"=> %s\n   %s, %I64d\n", url.c_str(), m.type.c_str(), m.length); 
		}
	}

	CComPtr<IGenericPlayer> player;

	if(m.type == L"application/x-shockwave-flash")
	{
		wprintf(L"flash?\n");

		if(FAILED(hr)) player = new FlashPlayer(url.c_str(), m_hWnd, hr);
	}
	else
	{
		wprintf(L"dvd?\n");
		
		if(FAILED(hr)) player = new DVDPlayer(url.c_str(), hr);

		wprintf(L"playlist?\n");
		
		if(FAILED(hr)) player = new PlaylistPlayer(url.c_str(), hr);

		if(m.type.empty() || m.type.find(L"image/") == 0)
		{
			wprintf(L"image?\n");
		
			if(FAILED(hr)) player = new ImagePlayer(url.c_str(), hr);
		}

		wprintf(L"file?\n");
		
		if(FAILED(hr)) player = new FilePlayer(url.c_str(), m.type.c_str(), hr);
	}

	if(FAILED(hr)) return hr;

	VideoPresenterParam p;

	p.dev = *(Device9*)m_dev;
	p.flip = m_flip;
	p.wnd = m_hWnd;
	p.msg = WM_GRAPHNOTIFY;

	hr = player->Render(p);

	if(FAILED(hr)) 
	{
		return hr;
	}

	hr = player->SetVolume(!g_env->Muted ? g_env->Volume : 0.0f);

	// if(FAILED(hr)) return hr;

	hr = player->Play();

	if(FAILED(hr))
	{
		return hr;
	}

	g_env->players.push_back(player);

	g_env->InfoEvent(NULL);

	std::wstring title = m_title + L" - " + std::wstring(path);

	SetWindowText(title.c_str());

	return S_OK;
}

HRESULT HomesysWindow::OpenTV(bool broadcast, bool streaming)
{
	HRESULT hr = E_FAIL;

	if(m_tv.broadcast == broadcast && m_tv.streaming == streaming)
	{
		return S_OK;
	}

	ClosePlayers();

	VideoPresenterParam p;

	p.dev = *(Device9*)m_dev;
	p.flip = m_flip;
	p.wnd = m_hWnd;
	p.msg = WM_GRAPHNOTIFY;

	float volume = !g_env->Muted ? g_env->Volume : 0.0f;

	std::list<Homesys::TunerReg> trs;

	g_env->svc.GetTuners(trs);

	bool first = true;

	for(auto i = trs.begin(); i != trs.end(); i++)
	{
		if(!(broadcast && i->type != Homesys::TunerDevice::DVBF || streaming && i->type == Homesys::TunerDevice::DVBF))
		{
			continue;
		}

		CComQIPtr<IGenericPlayer> player = new TunerPlayer(i->id, hr);

		if(FAILED(hr)) 
		{
			continue;
		}

		hr = player->Render(p);
		
		if(FAILED(hr)) 
		{
			continue;
		}

		// hr = player->SetVolume(volume);
		hr = player->Activate(volume);

		if(!first)
		{
			player->Deactivate();
		}

		first = false;

		player->Play();

		g_env->players.push_back(player);
	}

	if(g_env->players.empty())
	{
		return E_FAIL;
	}

	m_tv.broadcast = broadcast;
	m_tv.streaming = streaming;

	return S_OK;
}

void HomesysWindow::ClosePlayers()
{
	CAutoLock cAutoLock(&Control::m_lock);

	// we may get back the control and access g_env->players while the list is being emptied (someone does a message loop, filter or the graph)
	
	std::list<CAdapt<CComPtr<IGenericPlayer>>> players = g_env->players;
	g_env->players.clear();
	players.clear();

	m_tv.broadcast = false;
	m_tv.streaming = false;

	memset(&g_env->ps.dvd, 0, sizeof(g_env->ps.dvd));

	g_env->ps.current = 0;
	g_env->ps.start = 0;
	g_env->ps.stop = 0;
	g_env->ps.seekto = -1;
	g_env->ps.state = State_Stopped;

	SetWindowText(m_title.c_str());
}

void HomesysWindow::PrintScreen(bool ext)
{
	std::wstring fn = g_env->AppDataPath;

	fn += L"\\capture.bmp";

	Texture* t = NULL;

	if(ext && !g_env->players.empty())
	{
		CComPtr<IGenericPlayer> p = g_env->players.front();

		CComPtr<IDirect3DTexture9> vt;
		Vector2i ar;
		Vector4i src;

		if(SUCCEEDED(p->GetVideoTexture(&vt, ar, src)) && vt != NULL)
		{
			t = new Texture9(vt);

			t->Save(fn.c_str());

			return;
		}
	}
	
	m_mgr->Save(fn.c_str());
}

// Window message handlers

LRESULT HomesysWindow::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_DEVICECHANGE:
		OnDeviceChange(wParam);
		break;
	case WM_POWERBROADCAST:
		return OnPowerBroadcast(wParam);
	case WM_GRAPHNOTIFY:
		return OnGraphNotify(wParam, lParam);
	case WM_REACTIVATE_TUNER:
		return OnReactivateTuner(wParam, lParam);
	case WM_USERINPUT:
		OnUserInputMessage((Homesys::UserInputType)wParam, (int)lParam);
		return TRUE;
	case WM_SYSCOMMAND:
		if(OnSysCommand((wParam >> 16) & 0xffff, wParam & 0xffff, (HANDLE)lParam)) return 0;
		break;
	case WM_UPNP_MEDIA_RENDERER:
		OnUPnPMediaRenderer(wParam, lParam);
		return 0;
		break;
	}

	return __super::OnMessage(message, wParam, lParam);
}

bool HomesysWindow::OnCreate()
{
	if(!__super::OnCreate())
	{
		return false;
	}

	m_timer_fast = SetTimer(m_hWnd, m_timer++, 500, NULL);
	m_timer_slow = SetTimer(m_hWnd, m_timer++, 30000, NULL);

	m_mgr->Open(&m_menu->main, true);

	if(m_flags & 4)
	{
		m_mgr->Open(&m_menu->tv);
		m_mgr->Open(&m_menu->tvosd);
	}

	std::wstring username;

	if(g_env->svc.GetUsername(username) && username.empty())
	{
		m_mgr->Open(&m_menu->signup);
	}

	// m_mgr->Open(&m_menu->skype);

	m_callback = new Homesys::CallbackService(this);

	HMENU hSysMenu = GetSystemMenu(m_hWnd, FALSE);

	AppendMenu(hSysMenu, MF_SEPARATOR, -1, NULL); 
	AppendMenu(hSysMenu, MF_STRING, IDM_ALWAYS_ON_TOP, Control::GetString("ALWAYS_ON_TOP"));
	CheckMenuItem(hSysMenu, IDM_ALWAYS_ON_TOP, (SetAlwaysOnTop() ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

	m_mr = new Homesys::MediaRendererWrapper(m_hWnd, WM_UPNP_MEDIA_RENDERER);

	return true;
}

void HomesysWindow::OnDestroy()
{
	ClosePlayers();

	delete m_callback;

	m_callback = NULL;

	delete m_mr;

	m_mr = NULL;

	__super::OnDestroy();
}

void HomesysWindow::OnTimer(UINT id)
{
	if(id == m_timer_fast)
	{
		{
			Homesys::ParentalSettings parental;

			bool changed = false;

			{
				CAutoLock cAutoLock(&Control::m_lock);

				if(!g_env->parental.enabled && g_env->parental.inactivity > 0)
				{
					if(GetIdleTime() >= g_env->parental.inactivity * 1000 * 60)
					{
						g_env->parental.enabled = true;

						parental = g_env->parental;

						changed = true;
					}
				}
			}

			if(changed)
			{
				g_env->svc.SetParentalSettings(parental);
			}
		}

		//

		CComPtr<IGenericPlayer> player;

		{
			CAutoLock cAutoLock(&Control::m_lock);

			if(!g_env->players.empty())
			{
				player = g_env->players.front();

				player->Poke();
			}
		}

		if(player != NULL)
		{
			g_env->AutoSeek();

			REFERENCE_TIME current = 0;
			REFERENCE_TIME start = 0;
			REFERENCE_TIME stop = 0;

			player->GetCurrent(current);
			player->GetAvailable(start, stop);

			OAFilterState state = -1;

			if(FAILED(player->GetState(state)))
			{
				state = -1;
			}
/*
			bool recording = false;

			Homesys::Preset preset;
			Homesys::Channel channel;

			if(CComQIPtr<ITunerPlayer> t = player)
			{
				recording = g_env->svc.IsRecording(t->GetTunerId());

				g_env->svc.GetCurrentPreset(t->GetTunerId(), preset);

				if(preset.channelId)
				{
					g_env->svc.GetChannel(preset.channelId, channel);
				}

				if(0)
				{
					Homesys::TunerStat ts;

					if(g_env->svc.GetTunerStat(t->GetTunerId(), ts))
					{
						wprintf(L"f=%d p=%d l=%d s=%d q=%d\n", ts.freq, ts.present, ts.locked, ts.strength, ts.strength);
					}
				}
			}
*/
			CAutoLock cAutoLock(&Control::m_lock);

			g_env->ps.current = current;
			g_env->ps.start = start;
			g_env->ps.stop = stop;

			if(state >= 0)
			{
				g_env->ps.state = state;
			}
/*
			if(CComQIPtr<ITunerPlayer> t = player)
			{
				g_env->ps.tuner.recording = recording;
				g_env->ps.tuner.preset = preset;
				g_env->ps.tuner.channel = channel;
			}
*/
		}
		else
		{
			g_env->ps.current = 0;
			g_env->ps.start = 0;
			g_env->ps.stop = 0;
			g_env->ps.state = State_Stopped;
		}
	}
	else if(id == m_timer_slow)
	{
		GUID tunerId = GUID_NULL;

		bool active = false;

		{
			CAutoLock cAutoLock(&Control::m_lock);

			if(!g_env->players.empty())
			{
				CComPtr<IGenericPlayer> p = g_env->players.front();

				if(CComQIPtr<ITunerPlayer> t = p)
				{
					tunerId = t->GetTunerId();

					active = g_env->ps.state == State_Running && GetIdleTime() < 30 * 60 * 1000;
				}
			}
		}

		if(tunerId != GUID_NULL)
		{
			g_env->svc.SetPrimaryTuner(tunerId, active);
		}
/*
		{
			std::list<Homesys::Program> programs;

			if(g_env->svc.GetFavoritePrograms(10, programs))
			{
				for(auto i = programs.begin(); i != programs.end(); i++)
				{
					wprintf(L"%s %s\n", i->start.Format(L"%H:%M:%S"), i->episode.movie.title.c_str());
				}
			}
		}
*/
	
		{
			CAutoLock cAutoLock(&Control::m_lock);

			if(m_mgr != NULL)
			{
				if(dynamic_cast<MenuOSD*>(m_mgr->GetControl()))
				{
					UINT active = 0;

					if(SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, (PVOID)&active, 0))
					{
						SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, 0, 0, SPIF_SENDWININICHANGE);
						SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, active, 0, SPIF_SENDWININICHANGE);
					}

					active = 0;

					if(SystemParametersInfo(SPI_GETPOWEROFFACTIVE, 0, (PVOID)&active, 0))
					{
						SystemParametersInfo(SPI_SETPOWEROFFACTIVE, 0, 0, SPIF_SENDWININICHANGE);
						SystemParametersInfo(SPI_SETPOWEROFFACTIVE, active, 0, SPIF_SENDWININICHANGE);
					}
				}
			}
		}
	}

	__super::OnTimer(id);
}

void HomesysWindow::OnPaint(DeviceContext* dc)
{
	HRESULT hr;

	Vector4i cr = GetClientRect();
	Vector4i fr = cr;
	Vector2i pad;

	int crh = cr.height();

	bool first = true;

	for(auto i = g_env->players.begin(); i != g_env->players.end(); i++, first = false)
	{
		CComPtr<IGenericPlayer> player = *i;

		CComPtr<IDirect3DTexture9> vt;
		Vector2i ar;
		Vector4i src;

		hr = player->GetVideoTexture(&vt, ar, src);

		Texture* t = NULL;
		
		if(hr == E_ACCESSDENIED)
		{
			vt = NULL;

			t = dc->GetTexture(L"kidclose.png");

			if(t != NULL)
			{
				ar = Vector2i(1, 1);
				src = Vector4i(0, 0, t->GetWidth(), t->GetHeight());

				hr = S_OK;
			}
		}
		else
		{
			if(SUCCEEDED(hr))
			{
				t = new Texture9(vt);
			}
			else
			{
				ar = Vector2i(1, 1);
			}
		}

		//if(t != NULL)
		{
			bool inside = (m_arpreset & 4) == 0;

			switch(m_arpreset & 3)
			{
			case 0: break;
			case 1: ar = fr.rsize().br; break;
			case 2: ar = Vector2i(4, 3); break;
			case 3: ar = Vector2i(16, 9); break;
			}

			Vector4i r = dc->FitRect(ar, fr, inside);

			player->SetVideoPosition(r);

			if(t != NULL)
			{
				if(!m_fullrange)
				{
					t->SetFlag(Texture::NotFullRange);
				}

				if(0)
				{
					t->SetFlag(Texture::StereoImage);
				
					//r = Vector4i(0, 0, 1920, 1200);//.deflate(Vector4i(1920 / 8, 1200 / 8).xyxy());
				}

				dc->Draw(t, r, src);
			}
			else
			{
				if(player->HasVideo() == S_FALSE)
				{
					PaintAudio(player, dc);
				}
			}

			CComPtr<IDirect3DTexture9> subtexture;

			bool bbox;

			if(SUCCEEDED(player->GetSubtitleTexture(&subtexture, cr.br, r, bbox)))
			{
				Texture* t = new Texture9(subtexture);

				t->SetFlag(Texture::AlphaComposed);

				if(bbox) dc->Draw(r.inflate(5), 0x80000000);

				dc->Draw(t, r);

				delete t;
			}

			if(first)
			{
				if(m_arpreset_changed > 0)
				{
					if(clock() - m_arpreset_changed < 3000)
					{
						std::wstring str;

						switch(m_arpreset & 3)
						{
						case 0: str = Control::GetString("AR_AUTO"); break;
						case 1: str = Control::GetString("AR_STRETCHED"); break;
						case 2: str = Control::GetString("AR_43"); break;
						case 3: str = Control::GetString("AR_169"); break;
						}

						str = Util::Format(L"%s\n", Control::GetString("AR")) + str;

						if(!inside) str += Util::Format(L" (%s)", Control::GetString("AR_SURROUNDING"));

						dc->TextFace = FontType::Normal;
						dc->TextColor = Color::White;
						dc->TextHeight = 50 * crh / 1080;
						dc->TextAlign = Align::Middle | Align::Center;
						dc->TextStyle = TextStyles::Bold | TextStyles::Outline;

						dc->Draw(str.c_str(), cr);
					}
					else
					{
						m_arpreset_changed = 0;
					}
				}
				else if(m_fullrange_changed > 0)
				{
					if(clock() - m_fullrange_changed < 3000)
					{
						std::wstring str;

						if(m_fullrange) str = Control::GetString("LEVELS_FULLRANGE");
						else str = Control::GetString("LEVELS_TVSCALE");

						str = Util::Format(L"%s\n", Control::GetString("LEVELS")) + str;

						dc->TextFace = FontType::Normal;
						dc->TextColor = Color::White;
						dc->TextHeight = 50 * crh / 1080;
						dc->TextAlign = Align::Middle | Align::Center;
						dc->TextStyle = TextStyles::Bold | TextStyles::Outline;

						dc->Draw(str.c_str(), cr);
					}
					else
					{
						m_fullrange_changed = 0;
					}
				}
			}
		}

		if(vt != NULL)
		{
			delete t;
		}

		if(first)
		{
			fr = cr;
			fr.left = fr.right - fr.width() / 4;
			fr.bottom = fr.top + fr.height() / 4;
			pad.x = -cr.width() / 50;
			pad.y = +cr.height() / 50;
			fr += Vector4i(pad).xyxy();
		}
		else
		{
			fr += Vector4i(0, fr.height() + pad.y).xyxy();
		}

		if(!m_pip) break;
	}

	if(m_debug > 0)
	{
		if(!g_env->players.empty())
		{
			CComPtr<IGenericPlayer> player = g_env->players.front();

			CComPtr<IFilterGraph> pFG;

			player->GetFilterGraph(&pFG);

			Vector4i r = GetClientRect();

			r.left = 30 * crh / 1080;
			r.top = 50 * crh / 1080;
			r.bottom = 80 * crh / 1080;

			dc->TextFace = FontType::Normal;
			dc->TextHeight = r.height();
			dc->TextColor = Color::White;
			dc->TextAlign = Align::Left | Align::Top;
			dc->TextStyle = TextStyles::Bold | TextStyles::Outline;

			Vector4i rowsize = Vector4i(0, r.height() + 5).xyxy();

			if(CComQIPtr<ITunerPlayer> tuner = player)
			{
				Homesys::TunerStat& stat = g_env->ps.tuner.stat;

				std::wstring s;
				s = Util::Format(L"[Tuner] P: %d, L: %d, F: %d KHz, S: %d, Q: %d", stat.present, stat.present, stat.freq, stat.strength, stat.quality);
				if(stat.received >= 0) s += Util::Format(L", R: %lld KB", stat.received >> 10);
				if(stat.scrambled) s += L" (scrambled)";
					
				dc->Draw(s.c_str(), r);					
					
				r += rowsize;
			}

			Foreach(pFG, [&] (IBaseFilter* pBF) -> HRESULT
			{
				CFilterInfo fi;

				pBF->QueryFilterInfo(&fi);

				std::wstring s = L"[Filter] ";
				s += fi.achName;

				TextEntry* te = dc->Draw(s.c_str(), r);
					
				r += rowsize;
					
				if(m_debug > 1)
				{
					::Foreach(pBF, [&] (IPin* pPin) -> HRESULT
					{
						CMediaType mt;

						if(SUCCEEDED(pPin->ConnectionMediaType(&mt)))
						{
							PIN_DIRECTION dir;
								
							pPin->QueryDirection(&dir);

							s = (dir == PINDIR_INPUT ? L" [In] " : L" [Out] ") + CMediaTypeEx(mt).ToString();

							dc->Draw(s.c_str(), r);
					
							r += rowsize;
						}

						return S_CONTINUE;
					});
				}

				CComQIPtr<IBufferInfo> pBI = pBF;

				if(pBI != NULL)
				{
					std::wstring s;

					for(int i = 0, j = pBI->GetCount(); i < j; i++)
					{
						int samples = 0, size = 0;

						if(SUCCEEDED(pBI->GetStatus(i, samples, size)))
						{
							s += Util::Format(L" [Buff %d] %d/%d", i, samples, size);
						}
					}

					if(!s.empty())
					{
						te = dc->Draw(s.c_str(), r);

						r += rowsize;
					}
				}

				return S_CONTINUE;
			});
		}
	}
}

void HomesysWindow::OnPaintBusy(DeviceContext* dc)
{
	dc->TextAlign = Align::Center | Align::Middle;
	dc->TextHeight = 60;
	dc->TextFace = FontType::Normal;
	dc->TextColor = Color::Text1;
	dc->TextStyle = TextStyles::Bold | TextStyles::Shadow;

	dc->Draw(Control::GetString("PLEASE_WAIT"), GetClientRect());
}

bool HomesysWindow::OnInput(const KeyEventParam& p)
{
	CAutoLock cAutoLock(&Control::m_lock);

	if(m_mgr != NULL)
	{
		Control* c = m_mgr->GetControl();

		if(c != NULL && c->ClickThrough)
		{
			for(auto i = g_env->players.begin(); i != g_env->players.end(); i++)
			{
				if(p.cmd.app != 0)
				{
					(*i)->OnMessage(WM_APPCOMMAND, 0, (LPARAM)(p.cmd.app << 16));
				}
				else if(p.cmd.key != 0)
				{
					(*i)->OnMessage(WM_KEYDOWN, p.cmd.key, 0);
				}
			}
		}
	}

	if(__super::OnInput(p))
	{
		return true;
	}

	if(p.cmd.remote != 0) 
	{
		if(m_mgr != NULL && m_mgr->IsOpen(&m_menu->main))
		{
			std::list<Control*> l;

			switch(p.cmd.remote)
			{
			case RemoteKey::MyTV:
				l.push_back(&m_menu->tv);
				break;
			case RemoteKey::MyPictures:
				l.push_back(&m_menu->image);
				break;
			case RemoteKey::RecordedTV:
				l.push_back(&m_menu->tv);
				l.push_back(&m_menu->recordings);
				break;
			case RemoteKey::Guide:
				l.push_back(&m_menu->tv);
				l.push_back(&m_menu->epg);
				break;
			case RemoteKey::LiveTV:
				l.push_back(&m_menu->tv);
				l.push_back(&m_menu->tvosd);
				break;
			case RemoteKey::DVDMenu:
				l.push_back(&m_menu->dvd);
				break;
			case RemoteKey::Yellow:
				g_env->AspectRatioEvent(NULL);
				return true;
			case RemoteKey::Blue:
				g_env->PIPEvent(NULL);
				return true;
			}

			if(!l.empty() && m_mgr->GetControl() != l.back())
			{
				m_mgr->Open(&m_menu->main, true);

				for(auto i = l.begin(); i != l.end(); i++)
				{
					m_mgr->Open(*i);
				}

				return true;
			}
		}
	}
	else
	{
		switch(p.cmd.key)
		{
		case VK_F5:
			if(p.mods == 0) g_env->MuteEvent(NULL);
			return true;
		case VK_F6:
			if(p.mods == 0) g_env->FullRangeEvent(NULL);
			return true;
		/*
		case VK_F7:
			if(p.mods == 0) g_env->AspectRatioEvent(NULL);
			return true;
		case VK_F8:
			if(p.mods == 0) g_env->PIPEvent(NULL);
			return true;
		*/
		case VK_F9:
			PrintScreen((p.mods & KeyEventParam::Ctrl) != 0);
			return true;
		case VK_F10:
		case VK_F11:
			ToggleFullscreen((p.mods & KeyEventParam::Alt) == 0);
			return true;
		case VK_RETURN:
			if((p.mods & (KeyEventParam::Alt | KeyEventParam::Shift)) != 0) {ToggleFullscreen(true); return true;}
			break;
		case VK_HOME:
			m_mgr->Open(&m_menu->main, true);
			break;
			/*
		case VK_VOLUME_MUTE:
			g_env->Muted = !g_env->Muted;
			return true;
		case VK_VOLUME_DOWN:
			g_env->Volume = g_env->Volume - 0.1f; 
			return true;
		case VK_VOLUME_UP:
			g_env->Volume = g_env->Volume + 0.1f; 
			return true;
			*/
		}

		switch(p.cmd.chr)
		{
		case 'd':
			m_debug = (m_debug + 1) % 3;
			return true;
		}

		switch(p.cmd.app)
		{
		case APPCOMMAND_VOLUME_DOWN: 
			g_env->Volume = g_env->Volume - 0.1f; 
			break;
		case APPCOMMAND_VOLUME_UP: 
			g_env->Volume = g_env->Volume + 0.1f; 
			break;
		case APPCOMMAND_VOLUME_MUTE: 
			g_env->Muted = !g_env->Muted; 
			break;
		}
	}

	return false;
}

bool HomesysWindow::OnInput(const MouseEventParam& p)
{
	CAutoLock cAutoLock(&Control::m_lock);

	if(m_mgr != NULL)
	{
		Control* c = m_mgr->GetControl();

		if(c != NULL && c->ClickThrough)
		{
			for(auto i = g_env->players.begin(); i != g_env->players.end(); i++)
			{
				if(p.cmd == MouseEventParam::Move)
				{
					(*i)->OnMessage(WM_MOUSEMOVE, 0, MAKELPARAM(p.point.x, p.point.y));
				}
				else if(p.cmd == MouseEventParam::Down)
				{
					(*i)->OnMessage(WM_LBUTTONDOWN, 0, MAKELPARAM(p.point.x, p.point.y));
				}
			}
		}
	}

	if(__super::OnInput(p))
	{
		if(MouseEventParam::Wheel == p.cmd)
		{
			int i = 0;
		}

		return true;
	}

	switch(p.cmd)
	{
	case MouseEventParam::MUp:
		ToggleFullscreen(false);		
		return true;
	case MouseEventParam::Wheel:
		g_env->Volume = g_env->Volume + 0.1f * p.delta;
		break;
	}

	return false;
}

void HomesysWindow::OnBeforeResetDevice()
{
	for(auto i = g_env->players.begin(); i != g_env->players.end(); i++)
	{
		(*i)->ResetDevice(true);
	}
}

void HomesysWindow::OnAfterResetDevice()
{
	for(auto i = g_env->players.begin(); i != g_env->players.end(); i++)
	{
		(*i)->ResetDevice(false);
	}
}

void HomesysWindow::OnDropFiles(const std::list<std::wstring>& files)
{
	CAutoLock cAutoLock(&Control::m_lock);
	
	std::wstring path;
	std::list<std::wstring> subs;

	for(auto i = files.begin(); i != files.end(); i++)
	{
		if(LPCWSTR ext = PathFindExtension(i->c_str()))
		{
			if(GenericPlayer::IsSubtitleExt(ext))
			{
				subs.push_back(*i);

				continue;
			}
		}

		if(path.empty())
		{
			path = *i;
		}
	}

	if(!path.empty())
	{
		g_env->PlayFileEvent(NULL, path.c_str());
	}

	if(!g_env->players.empty() && !subs.empty())
	{
		CComPtr<IGenericPlayer> p = g_env->players.front();

		if(CComQIPtr<ISubtitleFilePlayer> sp = p)
		{
			int count_before = 0;
			int count_after = 0;

			p->GetTrackCount(TrackType::Subtitle, count_before);

			for(auto i = subs.begin(); i != subs.end(); i++)
			{
				sp->AddSubtitle(i->c_str());
			}

			p->GetTrackCount(TrackType::Subtitle, count_after);

			if(count_before < count_after)
			{
				p->SetTrack(TrackType::Subtitle, count_before);

				g_env->InfoEvent(NULL);
			}
		}
	}
}

void HomesysWindow::OnDeviceChange(WPARAM wParam)
{
	if(wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		m_menu->dvd.Reload();

		// TODO: BuildBrowseTree();
	}
	else if(wParam == DBT_DEVNODES_CHANGED)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		// TODO: m_menu.reg.Activate();
	}
}

LRESULT HomesysWindow::OnPowerBroadcast(WPARAM wParam)
{
	if(wParam == PBT_APMQUERYSUSPEND)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		if(m_mgr != NULL)
		{
			if(dynamic_cast<MenuOSD*>(m_mgr->GetControl()))
			{
				return BROADCAST_QUERY_DENY;
			}
		}
	}
	else if(wParam == PBT_APMSUSPEND)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		if(m_mgr != NULL && m_mgr->IsOpen(&m_menu->main))
		{
			if(m_mgr->GetControl() != &m_menu->main)
			{
				m_mgr->Open(&m_menu->main, true);
			}
		}

		ClosePlayers();
	}
	else if(wParam == PBT_APMRESUMESUSPEND)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		g_env->svc.IsOnline();
	}

	return TRUE;
}

LRESULT HomesysWindow::OnGraphNotify(WPARAM wParam, LPARAM lParam)
{
	IGenericPlayer* player = (IGenericPlayer*)lParam;

	CheckPointer(player, E_POINTER);

	bool found = false;

	for(auto i = g_env->players.begin(); i != g_env->players.end(); i++)
	{
		CComPtr<IGenericPlayer> p = *i;

		if(player == p)
		{
			found = true;
			break;
		}
	}

	if(!found)
	{
		return E_INVALIDARG;
	}

	CAutoLock cAutoLock(&Control::m_lock);

	long code;
	LONG_PTR p1, p2;

	while(S_OK == player->GetEvent(&code, &p1, &p2))
	{
		bool dvd = false;

		switch(code)
		{
		case EC_COMPLETE:
			player->Pause(); // player->Seek(0);
			break;
		case EC_DVD_DOMAIN_CHANGE:
			g_env->ps.dvd.domain = (DVD_DOMAIN)p1;
			dvd = true;
			break;
		case EC_DVD_TITLE_CHANGE:
			g_env->ps.dvd.title = (DWORD)p1;
			dvd = true;
			break;
		case EC_DVD_CHAPTER_START:
			g_env->ps.dvd.chapter = (DWORD)p1;
			dvd = true;
			break;
		case EC_DVD_ANGLE_CHANGE:
			g_env->ps.dvd.angle = (DWORD)p2;
			dvd = true;
			break;
		case EC_DVD_AUDIO_STREAM_CHANGE:
			g_env->ps.dvd.audio = (DWORD)p1;
			dvd = true;
			break;
		case EC_DVD_SUBPICTURE_STREAM_CHANGE:
			g_env->ps.dvd.subpicture = p2 ? (DWORD)p1 : -1;
			dvd = true;
			break;
		case EC_DVD_CURRENT_HMSF_TIME:
			// m_playerstatus.pos = HMSF2RT(*(DVD_HMSF_TIMECODE*)&p1, 0);
			// dvd = true;
			break;
		case EC_DVD_ERROR:
			g_env->ps.dvd.error = p1;
			dvd = true;
			wprintf(L"EC_DVD_ERROR %d %d\n", p1, p2);
			break;
		case EC_DVD_WARNING:
			dvd = true;
			wprintf(L"EC_DVD_WARNING %d %d\n", p1, p2);
			break;
		case EC_VIDEO_SIZE_CHANGED:
			break;

		case EC_USER:
			
			if(p1 == EC_USER_MAGIC)
			{
				if(p2 == EC_USER_PAUSE) player->Suspend();
				if(p2 == EC_USER_RESUME) player->Resume();
			}

			break;

		default:
			// wprintf(L"EC_???? 0x%08x 0x%08x 0x%08x\n", code, p1, p2);
			break;
		}

		player->FreeEvent(code, p1, p2);

		if(EC_COMPLETE == code)
		{
			CComQIPtr<ITunerPlayer> t = player;

			if(t == NULL)
			{
				player->SetEOF();

				g_env->EndFileEvent(NULL);
			}

			break;
		}

		if(dvd)
		{
			g_env->InfoEvent(NULL);
		}
	}

	return S_OK;
}

LRESULT HomesysWindow::OnReactivateTuner(WPARAM wParam, LPARAM lParam)
{
	CAutoLock cAutoLock(&Control::m_lock);

	((ITunerPlayer*)lParam)->Reactivate();

	g_env->Volume = g_env->Volume;

	return 0;
}

void HomesysWindow::OnUserInputMessage(Homesys::UserInputType type, int value)
{
	CAutoLock cAutoLock(&Control::m_lock);

	if(type == Homesys::VirtualKey)
	{
		KeyEventParam p;

		memset(&p, 0, sizeof(p));

		p.cmd.key = value;

		OnInput(p);
	}
	else if(type == Homesys::VirtualChar)
	{
		char utf8[5];

		memcpy(utf8, &value, 4);

		utf8[4] = 0;

		std::wstring s = Util::UTF8To16(utf8);

		if(s.empty()) return;

		KeyEventParam p;

		memset(&p, 0, sizeof(p));

		p.cmd.chr = s[0];

		OnInput(p);
	}
	else if(type == Homesys::AppCommand)
	{
		KeyEventParam p;

		memset(&p, 0, sizeof(p));

		p.cmd.app = value;

		OnInput(p);
	}
	else if(type == Homesys::RemoteKey)
	{
		KeyEventParam p;

		memset(&p, 0, sizeof(p));

		p.cmd.remote = value;

		OnInput(p);
	}
	else if(type == Homesys::MouseKey)
	{
		MouseEventParam p;

		memset(&p, 0, sizeof(p));

		switch(value)
		{
		case WM_LBUTTONDOWN: p.cmd = MouseEventParam::Down; break;
		case WM_LBUTTONUP: p.cmd = MouseEventParam::Up; break;
		case WM_LBUTTONDBLCLK: p.cmd = MouseEventParam::Dbl; break;
		case WM_RBUTTONDOWN: p.cmd = MouseEventParam::RDown; break;
		case WM_RBUTTONUP: p.cmd = MouseEventParam::RUp; break;
		case WM_RBUTTONDBLCLK: p.cmd = MouseEventParam::RDbl; break;
		case WM_MBUTTONDOWN: p.cmd = MouseEventParam::MDown; break;
		case WM_MBUTTONUP: p.cmd = MouseEventParam::MUp; break;
		case WM_MBUTTONDBLCLK: p.cmd = MouseEventParam::MDbl; break;
		}

		if(p.cmd != MouseEventParam::None)
		{
			GetCursorPos((POINT*)&p.point);

			ScreenToClient(m_hWnd, (POINT*)&p.point);

			Vector4i r = GetClientRect();

			if(p.point.x >= r.left && p.point.x < r.right && p.point.x >= r.top && p.point.y < r.bottom)
			{
				OnInput(p);
			}
		}
	}
	else if(type == Homesys::MousePosition)
	{
		float x = (float)(WORD)(value & 0xffff) / 0xffff;
		float y = (float)(WORD)(value >> 16) / 0xffff;

		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		POINT pt = {0, 0};
		GetMonitorInfo(MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY), &mi);

		MouseEventParam p;

		memset(&p, 0, sizeof(p));

		p.cmd = MouseEventParam::Move;
		p.point.x = (int)(x * (mi.rcMonitor.right - mi.rcMonitor.left) + mi.rcMonitor.left);
		p.point.y = (int)(y * (mi.rcMonitor.bottom - mi.rcMonitor.top) + mi.rcMonitor.top);

		SetCursorPos(p.point.x, p.point.y);

		// OnInput(p);
	}
}

bool HomesysWindow::OnSysCommand(UINT source, UINT id, HANDLE handle)
{
	if(source == 0) // menu
	{
		if(id == IDM_ALWAYS_ON_TOP)
		{
			HMENU hSysMenu = GetSystemMenu(m_hWnd, FALSE);

			bool b = !SetAlwaysOnTop();

			CheckMenuItem(hSysMenu, IDM_ALWAYS_ON_TOP, (b ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

			SetAlwaysOnTop(b ? 1 : 0);

			return true;
		}
	}

	return false;
}

void HomesysWindow::OnUPnPMediaRenderer(WPARAM wParam, LPARAM lParam)
{
	CAutoLock cAutoLock(&Control::m_lock);

	std::wstring s;

	switch(wParam)
	{
	case ID_UPNP_GET_POS:

		*(__int64*)lParam = g_env->ps.current;

		break;

	case ID_UPNP_GET_DUR:

		*(__int64*)lParam = g_env->ps.stop - g_env->ps.start;

		break;

	case ID_UPNP_GET_STATE:

		if(!g_env->players.empty())
		{
			switch(g_env->ps.state)
			{
			case State_Stopped: 
				s = L"STOPPED";
				break;
			case State_Paused: 
				s = g_env->ps.recording.id != GUID_NULL ? L"PAUSED_RECORDING" : L"PAUSED_PLAYBACK";
				break;
			case State_Running: 
				s = g_env->ps.recording.id != GUID_NULL ? L"RECORDING" : L"PLAYING";
				break;
			}
		}
		else
		{
			s = L"NO_MEDIA_PRESENT";
		}

		*(std::wstring*)lParam = s;

		break;

	case ID_UPNP_OPEN:

		g_env->PlayFileEvent(NULL, (LPCWSTR)lParam);

		break;

	case ID_UPNP_STOP:

		g_env->StopEvent(NULL);

		break;

	case ID_UPNP_PAUSE:

		g_env->PauseEvent(NULL);

		break;

	case ID_UPNP_PLAY:

		g_env->PlayEvent(NULL);

		break;

	case ID_UPNP_PREV:

		g_env->PrevEvent(NULL);

		break;

	case ID_UPNP_NEXT:

		g_env->NextEvent(NULL);

		break;

	case ID_UPNP_SEEK:

		wprintf(L"TODO: Seek(%s)\n", (LPCWSTR)lParam);

		break;

	case ID_UPNP_GET_VOLUME:

		*(float*)lParam = g_env->Volume;

		break;

	case ID_UPNP_GET_MUTE:

		*(bool*)lParam = g_env->Muted;

		break;

	case ID_UPNP_SET_VOLUME:

		g_env->Volume = *(float*)lParam;

		break;

	case ID_UPNP_SET_MUTE:

		g_env->Muted = *(bool*)lParam;

		break;
	}
}

// DXUI event handlers

bool HomesysWindow::OnFileInfo(Control* c)
{
	SetWindowText(m_title.c_str());

	return true;
}

bool HomesysWindow::OnPlayFile(Control* c, std::wstring file)
{
	if(!file.empty())
	{
		if(SUCCEEDED(OpenFile(file.c_str())))
		{
			OnVideoClicked(NULL);

			return true;
		}
	}

	return false;
}

bool HomesysWindow::OnOpenFile(Control* c, std::wstring file)
{
	if(SUCCEEDED(OpenFile(file.c_str())))
	{
		return true;
	}

	return false;
}

bool HomesysWindow::OnPlayerClose(Control* c)
{
	ClosePlayers();

	return true;
}

bool HomesysWindow::OnMute(Control* c)
{
	g_env->Muted = !g_env->Muted;

	return true;
}

bool HomesysWindow::OnAspectRatio(Control* c)
{
	m_arpreset = (m_arpreset + 1) & 7;
	m_arpreset_changed = clock();

	Util::Config(CONFIG_KEY, L"Settings").SetInt(L"AspectRatio", m_arpreset);

	return true;
}

bool HomesysWindow::OnPIP(Control* c)
{
	m_pip = !m_pip;

	Util::Config(CONFIG_KEY, L"Settings").SetInt(L"PIP", m_pip);

	return true;
}

bool HomesysWindow::OnFullRange(Control* c)
{
	m_fullrange = !m_fullrange;
	m_fullrange_changed = clock();

	Util::Config(CONFIG_KEY, L"Settings").SetInt(L"FullRange", m_fullrange);

	return true;
}

bool HomesysWindow::OnVideoClicked(Control* c)
{
	CComPtr<IGenericPlayer> player = g_env->players.front();

	if(CComQIPtr<ITunerPlayer> t = player)
	{
		GUID tunerId = t->GetTunerId();

		Homesys::TunerReg tr;

		if(g_env->svc.GetTuner(tunerId, tr))
		{
			m_mgr->Open(tr.type == Homesys::TunerReg::DVBF ? &m_menu->webtvosd : &m_menu->tvosd);
		}

		return true;
	}

	if(CComQIPtr<IDVDPlayer>(player))
	{
		m_mgr->Open(&m_menu->fileosd);

		return true;
	}

	if(CComQIPtr<IFilePlayer>(player))
	{
		m_mgr->Open(&m_menu->fileosd);

		return true;
	}

	if(CComQIPtr<IImagePlayer>(player))
	{
		m_mgr->Open(&m_menu->fileosd); // TODO

		return true;
	}

	return false;
}

bool HomesysWindow::OnRegisterUser(Control* c)
{
	m_mgr->Open(&m_menu->signup);

	return true;
}

bool HomesysWindow::OnRecordClicked(Control* c)
{
	m_menu->record.CurrentProgramId = (int)(UINT_PTR)c->UserData;

	m_mgr->Open(&m_menu->record);

	return true;
}

bool HomesysWindow::OnChannelDetails(Control* c)
{
	m_menu->epgchannel.ChannelId = (int)(UINT_PTR)c->UserData;

	m_mgr->Open(&m_menu->epgchannel);

	return true;
}

bool HomesysWindow::OnModifyRecording(Control* c, GUID recordingId)
{
	m_menu->recordingmodify.RecordingId = recordingId;

	m_mgr->Open(&m_menu->recordingmodify);

	return true;
}

bool HomesysWindow::OnTeletext(Control* c)
{
	if(!g_env->players.empty())
	{
		CComPtr<IGenericPlayer> p = g_env->players.front();

		bool haspage = false;

		CComPtr<IFilterGraph> g;

		if(SUCCEEDED(p->GetFilterGraph(&g)))
		{
			ForeachInterface<ITeletextRenderer>(g, [&] (IBaseFilter* pBF, ITeletextRenderer* pTR) -> HRESULT
			{
				haspage = pTR->HasPage() == S_OK;

				return S_OK;
			});
		}

		if(haspage)
		{
			m_mgr->Open(&m_menu->teletext);
		}
	}

	return true;
}

bool HomesysWindow::OnEditText(Control* c)
{
	if(m_mgr->GetControl() != &m_menu->edittext)
	{
		m_menu->edittext.Target = c;

		m_mgr->Open(&m_menu->edittext);
	}

	return true;
}

bool HomesysWindow::OnMenuMainClicked(Control* c, int id)
{
	bool online = g_env->svc.IsOnline();

	switch(id)
	{
	case MenuMain::TV: 
		m_mgr->Open(&m_menu->tv); 
		return true;
	
	case MenuMain::DVD: 
		m_mgr->Open(&m_menu->dvd); 
		return true;

	case MenuMain::BrowseFiles: 
		m_mgr->Open(&m_menu->file); 
		return true;

	case MenuMain::BrowsePictures: 
		m_mgr->Open(&m_menu->image); 
		return true;

	case MenuMain::SettingsGeneral: 
		m_mgr->Open(&m_menu->settings); 
		return true;

	case MenuMain::SettingsPersonal: 
		m_mgr->Open(&m_menu->personal); 
		return true;

	case MenuMain::SettingsRegistration: 
		m_mgr->Open(&m_menu->signup); 
		return true;

	case MenuMain::SettingsVideo:
		m_mgr->Open(&m_menu->video); 
		return true;

	case MenuMain::SettingsAudio:
		m_mgr->Open(&m_menu->audio); 
		return true;

	case MenuMain::SettingsSubtitle:
		m_mgr->Open(&m_menu->subtitle); 
		return true;

	case MenuMain::SettingsHotLink:
		m_mgr->Open(&m_menu->hotlink); 
		return true;

	case MenuMain::SettingsParental:
		m_mgr->Open(&m_menu->parental); 
		return true;

	case MenuMain::Exit: 
		PostMessage(m_hWnd, WM_CLOSE, 0, 0); 
		return true;
	}

	return false;
}

bool HomesysWindow::OnMenuTVClicked(Control* c, int id)
{
	switch(id)
	{
	case MenuTV::TV: 
		m_mgr->Open(&m_menu->tvosd);
		return true;
	case MenuTV::WebTV: 
		m_mgr->Open(&m_menu->webtvosd); 
		return true;
	case MenuTV::EPG: 
		m_mgr->Open(&m_menu->epg); 
		return true;
	case MenuTV::Recordings:
		m_mgr->Open(&m_menu->recordings); 
		return true;
	case MenuTV::Schedule: 
		m_mgr->Open(&m_menu->recordingmodify); 
		return true;
	case MenuTV::Wishlist: 
		m_mgr->Open(&m_menu->wishlist); 
		return true;
	case MenuTV::Search: 
		m_mgr->Open(&m_menu->epgsearch); 
		return true;
	case MenuTV::Tuners:
		m_mgr->Open(&m_menu->tuner); 
		return true;
	case MenuTV::SmartCards:
		m_mgr->Open(&m_menu->smartcard); 
		return true;
	case MenuTV::Tuning: 
		m_mgr->Open(&m_menu->tuning); 
		return true;
	case MenuTV::Presets: 
		m_mgr->Open(&m_menu->presets); 
		return true;
	case MenuTV::PresetsOrder: 
		m_mgr->Open(&m_menu->presetsorder); 
		return true;
	case MenuTV::Recommend: 
		// TODO: m_mgr->Open(&m_menu.programrelated); 
		return true;
	}

	return false;
}

bool HomesysWindow::OnMenuTVActivating(Control* c)
{
	if(c == &m_menu->webtvosd)
	{
		OpenTV(false, true);
	}
	else if(c == &m_menu->presets)
	{
		OpenTV(true, true);
	}
	else
	{
		OpenTV(true, false);
	}

	return true;
}

bool HomesysWindow::OnMenuDVDClicked(Control* c, int id)
{
	wchar_t drive = (wchar_t)id;

	switch(DirectShow::GetDriveType(drive))
	{
	case DirectShow::Drive_DVDVideo:

		if(SUCCEEDED(OpenFile(Util::Format(L"%c:\\", drive).c_str())))
		{
			m_mgr->Open(&m_menu->fileosd);

			return true;
		}

		break;

	case DirectShow::Drive_Audio:

		m_menu->audiocd.Location = drive;

		m_mgr->Open(&m_menu->audiocd);

		return true;
	}

	return false;
}

// Homesys::ICallback

void HomesysWindow::OnUserInput(Homesys::UserInputType type, int value)
{
	PostMessage(m_hWnd, WM_USERINPUT, (WPARAM)type, (LPARAM)value);
}

void HomesysWindow::OnCurrentPresetChanged(const GUID& tunerId)
{
	CAutoLock cAutoLock(&Control::m_lock);

	for(auto i = g_env->players.begin(); i != g_env->players.end(); i++)
	{
		CComPtr<IGenericPlayer> p = *i;

		if(CComQIPtr<ITunerPlayer> t = p)
		{
			if(t->GetTunerId() == tunerId)
			{
				t->Update();

				break;
			}
		}
	}
}

//

void HomesysWindow::PaintAudio(IGenericPlayer* player, DeviceContext* dc)
{
	if(Texture* t = g_env->GetAudioTexture(player, dc))
	{
		Vector4i cr = GetClientRect();

		float w = (float)cr.width();
		float h = (float)cr.height();

		Vertex* v = (Vertex*)_aligned_malloc(sizeof(Vertex) * 6, 16);

		Vector2i c(0xffffffff, 0);

		v[0].c = c;
		v[0].p = Vector4(0.0f, 0.0f, 0.5f, 1.0f);
		v[0].t = Vector2(0, 0);

		v[1].c = c;
		v[1].p = Vector4(w, 0.0f, 0.5f, 1.0f);
		v[1].t = Vector2(1, 0);

		v[2].c = c;
		v[2].p = Vector4(0.0f, h, 0.5f, 1.0f);
		v[2].t = Vector2(0, 1);

		v[3].c = c;
		v[3].p = Vector4(w, 0.0f, 0.5f, 1.0f);
		v[3].t = Vector2(1, 0);

		v[4].c = c;
		v[4].p = Vector4(0.0f, h, 0.5f, 1.0f);
		v[4].t = Vector2(0, 1);

		v[5].c = c;
		v[5].p = Vector4(w, h, 0.5f, 1.0f);
		v[5].t = Vector2(1, 1);

		for(int i = 0; i < 6; i++)
		{
			v[i].p.y *= 0.5f;
		}

		dc->DrawTriangleList(v, 2, t);

		for(int i = 0; i < 6; i++)
		{
			v[i].p.y = (float)h - v[i].p.y;
		}

		dc->DrawTriangleList(v, 2, t);

		_aligned_free(v);

		dc->GetDevice()->Recycle(t);
	}

	/*
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

			if(m_fft.fmax.size() != buff.size())
			{
				m_fft.fmax.resize(buff.size());
				m_fft.ttl.resize(buff.size());

				for(int i = 0; i < buff.size(); i++)
				{
					m_fft.fmax[i] = buff[i];
					m_fft.ttl[i] = 0;
				}
			}

			float volume = 0;

			if(S_OK == pASF->GetRecentFrequencies(buff.data(), buff.size(), volume))
			{
				volume = std::min<float>(volume * 3, 1.0f);
				
				//

				Device* dev = dc->GetDevice();

				Texture* t = dev->CreateRenderTarget(1024, 1024, false);

				Texture* rt = dc->SetTarget(t);

				w = t->GetWidth();
				h = t->GetHeight();

				dc->Draw(Vector4i(0, 0, w, h), 0x20000000);

				//

				for(int i = 0; i < buff.size(); i++)
				{
					if(m_fft.fmax[i] < buff[i])
					{
						m_fft.fmax[i] = buff[i];
						m_fft.ttl[i] = 64;
					}
				}

				Vertex* vertices = (Vertex*)_aligned_malloc(sizeof(Vertex) * w * 2, 16);

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
						i += 2;
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
						float s = m_fft.fmax[index];

						DWORD c = 0xff000000;

						DWORD cc = std::min<DWORD>(m_fft.ttl[index] << 2, 0xff);

						c = 0xff000000 | (cc << 16) | (cc << 8) | cc;

						vertices[j * 2 + 0].p = Vector4((float)i, (float)(h) - s - 20, 0.5f, 1.0f);
						vertices[j * 2 + 0].c.x = vertices[j * 2 + 0].c.y = c;
						vertices[j * 2 + 1].p = Vector4((float)i, (float)(h) - s, 0.5f, 1.0f);
						vertices[j * 2 + 1].c.x = vertices[j * 2 + 1].c.y = c;

						j++;
					}
					else
					{
						i += 2;
					}

					index_prev = index;
				}

				dc->DrawLineList(vertices, j);

				_aligned_free(vertices);

				for(int i = 0; i < buff.size(); i++)
				{
					m_fft.fmax[i] += (64 - m_fft.ttl[i]) / 4;

					if(m_fft.fmax[i] < 1 || m_fft.fmax[i] > h) 
					{
						m_fft.fmax[i] = 0;
					}

					if(m_fft.ttl[i] > 0)
					{
						m_fft.ttl[i]--;
					}
					else
					{
						//m_fft.fmax[i] = 0;
					}
				}

				//

				dc->SetTarget(rt);

				{
					float w = (float)cr.width();
					float h = (float)cr.height();

					Vertex* v = (Vertex*)_aligned_malloc(sizeof(Vertex) * 6, 16);

					Vector2i c(0xffffffff, 0);

					v[0].c = c;
					v[0].p = Vector4(0.0f, 0.0f, 0.5f, 1.0f);
					v[0].t = Vector2(0, 0);

					v[1].c = c;
					v[1].p = Vector4(w, 0.0f, 0.5f, 1.0f);
					v[1].t = Vector2(1, 0);

					v[2].c = c;
					v[2].p = Vector4(0.0f, h, 0.5f, 1.0f);
					v[2].t = Vector2(0, 1);

					v[3].c = c;
					v[3].p = Vector4(w, 0.0f, 0.5f, 1.0f);
					v[3].t = Vector2(1, 0);

					v[4].c = c;
					v[4].p = Vector4(0.0f, h, 0.5f, 1.0f);
					v[4].t = Vector2(0, 1);

					v[5].c = c;
					v[5].p = Vector4(w, h, 0.5f, 1.0f);
					v[5].t = Vector2(1, 1);

					for(int i = 0; i < 6; i++)
					{
						v[i].p.y *= 0.5f;
					}

					dc->DrawTriangleList(v, 2, t);

					for(int i = 0; i < 6; i++)
					{
						v[i].p.y = (float)h - v[i].p.y;
					}

					dc->DrawTriangleList(v, 2, t);

					_aligned_free(v);
				}

				dev->Recycle(t);
			}
		}
	}
	*/
}
