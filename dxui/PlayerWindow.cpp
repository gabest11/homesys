#include "StdAfx.h"
#include "PlayerWindow.h"
#include "../util/String.h"
#include "../util/Config.h"
#include <dwmapi.h>

typedef HRESULT (WINAPI *DwmEnableMMCSSPtr)(BOOL b);

namespace DXUI
{
	PlayerWindow::PlayerWindow(LPCWSTR config, ResourceLoader* loader, DWORD flags)
		: CUnknown(L"PlayerWindow", NULL)
		, m_config(config)
		, m_loader(loader)
		, m_flags(flags)
		, m_fullscreen(false)
		, m_exclusive(false)
		, m_dev(NULL)
		, m_dc(NULL)
		, m_mgr(NULL)
		, m_blocked(false)
		, m_input_clock(clock())
		, m_mouse_clock(INT_MAX)
		, m_mouse_pos(0, 0)
		, m_mouse_timer(0)
		, m_timer(1)
		, m_activated(true)
		, m_minimized(false)
		, m_mods(0)
	{
		AddRef();

		m_flip = CreateEvent(NULL, FALSE, FALSE, NULL);

		m_renderer.InitEvent.Add0(&PlayerWindow::OnFlip, this);

		memset(&m_last_vk.code, 0, sizeof(m_last_vk));
		/*
		if(HMODULE h = LoadLibrary(L"dwmapi.dll"))
		{
			DwmEnableMMCSSPtr p = (DwmEnableMMCSSPtr)GetProcAddress(h, "DwmEnableMMCSS");

			if(p != NULL) p(TRUE);
		}*/
	}

	PlayerWindow::~PlayerWindow()
	{
		m_renderer.Join();

		CloseHandle(m_flip);

		ASSERT(m_mgr == NULL);
		ASSERT(m_dc == NULL);
		ASSERT(m_dev == NULL);

		delete m_mgr;
		delete m_dc;
		delete m_dev;
		/*
		if(HMODULE h = LoadLibrary(L"dwmapi.dll"))
		{
			DwmEnableMMCSSPtr p = (DwmEnableMMCSSPtr)GetProcAddress(h, "DwmEnableMMCSS");

			if(p != NULL) p(FALSE);
		}*/
	}

	STDMETHODIMP PlayerWindow::NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
	    return 
			riid == __uuidof(IDropTarget) ? GetInterface((IDropTarget*)this, ppv) :
			__super::NonDelegatingQueryInterface(riid, ppv);
	}

	bool PlayerWindow::OnFlip(Control* c)
	{
//FILE* fp = _wfopen(L"c:\\flip.txt", L"wt");

		DWORD timeout = 10;

		while(m_renderer != NULL)
		{
			DWORD res = WaitForSingleObject(m_flip, timeout);

			switch(res)
			{
			case WAIT_OBJECT_0: timeout = 80; break;
			case WAIT_TIMEOUT: timeout = 10; break;
			}

			Vector4i r = GetClientRect(); // TODO: allow different targets

			if(r.rempty() || m_minimized)
			{
				Sleep(1);

				continue;
			}

			bool locked = false;

			clock_t start = clock();

			while(clock() - start < (!m_blocked ? 1000 : 20))
			{
				if(Control::m_lock.TryLock())
				{
					locked = true;

					break;
				}

				Sleep(1);
			}

			m_blocked = !locked;

clock_t t1, t2, t3, t4, t5, t6, t7;

t1 = clock() - start; start += t1;

			CAutoLock cAutoLock(m_dc);

t2 = clock() - start; start += t2;

			bool lost = false;

			if(!m_dev->IsLost())
			{
				Texture* bb = m_dev->GetBackbuffer();

				if(bb != NULL && (bb->GetWidth() != r.width() || bb->GetHeight() != r.height()))
				{
printf("*2 resize %p %d %d\n", bb, r.width(), r.height());

					m_dev->Resize(r.width(), r.height());
					
					bb = m_dev->GetBackbuffer();

printf("*3 GetBackbuffer %p\n", bb);

					m_dev->ClearRenderTarget(bb, 0);
				}

				if(bb != NULL)
				{
					r = Vector4i(0, 0, bb->GetWidth(), bb->GetHeight());
					
t3 = clock() - start; start += t3;
					
					m_dev->BeginScene();
					
t4 = clock() - start; start += t4;

					if(m_mgr != NULL) 
					{
						Control* c = m_mgr->GetControl();

						if(c == NULL || c->Transparent)
						{
							OnPaint(m_dc);
						}

t5 = clock() - start; start += t5;

						m_mgr->OnPaint(r, m_dc, locked);
					}

t6 = clock() - start; start += t6;

					if(!locked)
					{
						OnPaintBusy(m_dc);
					}

					m_dev->EndScene();
				}

				m_dc->Flip();

t7 = clock() - start; start += t7;

				if((m_flags & 2) != 0)
				{
					static clock_t base = 0;
					clock_t now = clock();
					printf("[%d] %d %d (%d %d %d %d %d %d %d)\n", res, now, now - base, t1, t2, t3, t4, t5, t6, t7);
					base = now;
				}

				if(bb != NULL)
				{
					m_dev->ClearRenderTarget(bb, 0);
				}
			}
			else
			{
				lost = true;
			}

			if(locked)
			{
				Control::m_lock.Unlock();
			}

			if(lost)
			{
				Sleep(500);

printf("*6 WM_RESETDEVICE\n");
				PostMessage(m_hWnd, WM_RESETDEVICE, 0, 0);
			}
		}

//if(fp != NULL) fclose(fp);

		return false;
	}

	void PlayerWindow::OnResetDevice(int mode)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		CAutoLock cAutoLock2(m_dc);

		m_dc->FlushCache();

		Control::DC = NULL;

		OnBeforeResetDevice();

		bool canReset;

		bool lost = m_dev->IsLost(canReset);

		if(!lost || canReset) // changing mode || reseting a lost device
		{
			m_dev->Reset(mode);
		}

		if(!m_dev->IsLost())
		{
			Control::DC = m_dc;

			OnAfterResetDevice();
		}
	}

	void PlayerWindow::ToggleFullscreen(bool exclusive)
	{
		if(GetSystemMetrics(SM_REMOTESESSION))
		{
			exclusive = false;
		}

		OSVERSIONINFO osver;

		osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		
		if(GetVersionEx(&osver) && osver.dwPlatformId == VER_PLATFORM_WIN32_NT 
			&& (osver.dwMajorVersion > 6 || osver.dwMajorVersion == 6 && osver.dwMinorVersion >= 1))
		{
			//exclusive = false;
		}

		Util::Config cfg(m_config.c_str(), L"Settings");

		Vector4i r;

		HWND insertAfter = HWND_NOTOPMOST;

		if(cfg.GetInt(L"AlwaysOnTop", 0) != 0)
		{
			insertAfter = HWND_TOPMOST;
		}

		if(m_fullscreen)
		{
			ShowFrame();

			r = Vector4i::load<false>(&m_rect);
		}
		else
		{
			GetWindowRect(m_hWnd, &m_rect);

			HideFrame();

			MONITORINFO mi;
			mi.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

			r = Vector4i::load<false>(&mi.rcMonitor);

			insertAfter = HWND_TOPMOST;
		}

		SetWindowPos(m_hWnd, insertAfter, r.left, r.top, r.width(), r.height(), 0);

		m_fullscreen = !m_fullscreen;

		if(!m_fullscreen && m_exclusive || m_fullscreen && exclusive)
		{
			OnResetDevice(m_fullscreen ? Device::Fullscreen : Device::Windowed);
		}

		m_exclusive = exclusive;
		
		cfg.SetInt(L"Fullscreen", m_fullscreen);
		cfg.SetInt(L"Exclusive", m_exclusive);
	}

	bool PlayerWindow::SetAlwaysOnTop(int value)
	{
		Util::Config cfg(m_config.c_str(), L"Settings");

		if(value < 0)
		{
			value = cfg.GetInt(L"AlwaysOnTop", 0);
		}

		bool b = value > 0;

		if(!m_fullscreen)
		{
#ifndef _DEBUG
			SetWindowPos(m_hWnd, b ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#endif
		}

		cfg.SetInt(L"AlwaysOnTop", b ? 1 : 0);

		return b;
	}

	// Window message handlers

	LRESULT PlayerWindow::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_CREATE:
			OnCreate();
			break;
		case WM_DESTROY:
			OnDestroy();
			break;
		case WM_ERASEBKGND:
			SetEvent(m_flip);
			return TRUE;
		case WM_SIZE:
			OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
			SetEvent(m_flip);
			break;
		case WM_RESETDEVICE:
			if(m_activated) OnResetDevice(Device::DontCare);
			return TRUE;
		case WM_TIMER:
			OnTimer((UINT)wParam);
			break;
		case WM_SETCURSOR:
			if(OnSetCursor(LOWORD(lParam))) return TRUE;
			break;
		case WM_COPYDATA:
			if(OnCopyData((COPYDATASTRUCT*)lParam)) return TRUE;
			break;
		case WM_DROPFILES:
			OnDropFiles((HDROP)wParam);
			return 0;
		case WM_MOUSEACTIVATE:
			if(LOWORD(lParam) == HTCLIENT) return MA_ACTIVATEANDEAT;
			break;
		case WM_ACTIVATE:

			m_activated = (wParam & 0xffff) != 0;
			m_minimized = (wParam >> 16) != 0;

			if(m_activated)
			{
				m_mods = 0;

				if(GetAsyncKeyState(VK_SHIFT) & 0x8000) m_mods |= KeyEventParam::Shift;
				if(GetAsyncKeyState(VK_CONTROL) & 0x8000) m_mods |= KeyEventParam::Ctrl;
				if(GetAsyncKeyState(VK_MENU) & 0x8000) m_mods |= KeyEventParam::Alt;
			}

			if(!m_activated && m_fullscreen && !m_exclusive)
			{
				POINT pt = {0, 0};

				HMONITOR h1 = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
				HMONITOR h2 = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

				printf("%p %p\n", h1, h2);

				if(h1 == h2) 
				{
					ToggleFullscreen(false); 
				}
			}

			break;
		case WM_ACTIVATEAPP:
			break;
		}

		if(message >= WM_KEYFIRST && message <= WM_KEYLAST
		|| message >= WM_MOUSEFIRST && message <= WM_MOUSELAST
		|| message == WM_APPCOMMAND
		|| message == WM_FAKE_APPCOMMAND
		|| message == WM_INPUT)
		{
			bool skip = false;

			switch(message)
			{
			case WM_APPCOMMAND:
				printf("WM_APPCOMMAND %08x %08x\n", wParam, lParam);
				break;
			case WM_FAKE_APPCOMMAND: 
				printf("WM_FAKE_APPCOMMAND %08x %08x\n", wParam, lParam);
				break;
			case WM_INPUT: 
				printf("WM_INPUT %08x %08x\n", wParam, lParam);
				break;
			default:
				if(message >= WM_KEYFIRST && message <= WM_KEYLAST) 
					printf("WM_KEY %d %08x %08x\n", message, wParam, lParam);
				if(message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
					//printf("WM_MOUSE %d %08x %08x\n", message, wParam, lParam);
				break;
			}

			//

			switch(message)
			{
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				if(wParam == VK_SHIFT) {m_mods |= KeyEventParam::Shift; skip = true;}
				else if(wParam == VK_CONTROL) {m_mods |= KeyEventParam::Ctrl; skip = true;}
				else if(wParam == VK_MENU) {m_mods |= (HIWORD(lParam) & KF_EXTENDED) ? KeyEventParam::AltGr : KeyEventParam::Alt; skip = true;}
				else m_last_vk.code = wParam;
				m_last_vk.mods = m_mods;
				printf("mods %d\n", m_mods);
				break;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				if(wParam == VK_SHIFT) {m_mods &= ~KeyEventParam::Shift; skip = true;}
				if(wParam == VK_CONTROL) {m_mods &= ~KeyEventParam::Ctrl; skip = true;}
				if(wParam == VK_MENU) {m_mods &= ~((HIWORD(lParam) & KF_EXTENDED) ? KeyEventParam::AltGr : KeyEventParam::Alt); skip = true;}
				printf("mods %d\n", m_mods);
				break;
			}

			// TODO

			if(0) switch(message)
			{
			case WM_APPCOMMAND:
				PostMessage(m_hWnd, WM_FAKE_APPCOMMAND, wParam, lParam);
				return 0;
			case WM_FAKE_APPCOMMAND:
				message = WM_APPCOMMAND;
				break;
			}

			//

			m_input_clock = clock();

			if(message == WM_MOUSEMOVE)
			{
				Vector2i p(LOWORD(lParam), HIWORD(lParam));

				if(abs(m_mouse_pos.x - p.x) + abs(m_mouse_pos.y - p.y) >= 8)
				{
					m_mouse_pos = p;
					m_mouse_clock = clock();

					SetCursor(LoadCursor(0, IDC_ARROW)); // TODO: arrow?
				}
				else
				{
					skip = true;
				}
			}

			if(!skip)
			{
				if(message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
				{
					MouseEventParam p;

					p.cmd = MouseEventParam::None;
					p.flags = wParam;
					p.point = Vector2i(LOWORD(lParam), HIWORD(lParam));

					switch(message)
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
					case WM_MOUSEMOVE: p.cmd = MouseEventParam::Move; break;
					case WM_MOUSEWHEEL: 
						p.cmd = MouseEventParam::Wheel; 
						p.flags = GET_KEYSTATE_WPARAM(wParam);
						p.delta = 1.0f * GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
						break;
					}

					if(OnInput(p)) return 0;
				}
				else if(message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_CHAR || message == WM_SYSCHAR)
				{
					KeyEventParam p;

					p.count = (lParam >> 16) & 0xffff;
					p.flags = lParam & 0xffff;
					p.mods = m_mods;
			
					if(message == WM_CHAR || message == WM_SYSCHAR)
					{
						p.cmd.chr = wParam;
					}
					else
					{
						p.cmd.key = wParam;
					}

					if((p.mods & KeyEventParam::Ctrl) != 0)
					{
						if(message == WM_KEYDOWN || message == WM_KEYUP)
						{
							switch(wParam)
							{
							case '1': p.cmd.key = VK_F1; p.mods &= ~KeyEventParam::Ctrl; break;
							case '2': p.cmd.key = VK_F2; p.mods &= ~KeyEventParam::Ctrl; break;
							case '3': p.cmd.key = VK_F3; p.mods &= ~KeyEventParam::Ctrl; break;
							case '4': p.cmd.key = VK_F4; p.mods &= ~KeyEventParam::Ctrl; break;
							case '5': p.cmd.key = VK_F5; p.mods &= ~KeyEventParam::Ctrl; break;
							case '6': p.cmd.key = VK_F6; p.mods &= ~KeyEventParam::Ctrl; break;
							case '7': p.cmd.key = VK_F7; p.mods &= ~KeyEventParam::Ctrl; break;
							case '8': p.cmd.key = VK_F8; p.mods &= ~KeyEventParam::Ctrl; break;
							case '9': p.cmd.key = VK_F9; p.mods &= ~KeyEventParam::Ctrl; break;
							case '0': p.cmd.key = VK_F10; p.mods &= ~KeyEventParam::Ctrl; break;
							}
						}
					}

					if(p.mods == 0)
					{
						switch(p.cmd.key)
						{
						case VK_F1: p.cmd.remote = RemoteKey::Red; break;
						case VK_F2: p.cmd.remote = RemoteKey::Green; break;
						case VK_F3: p.cmd.remote = RemoteKey::Yellow; break;
						case VK_F4: p.cmd.remote = RemoteKey::Blue; break;
						}
					}
					else if(p.mods == KeyEventParam::Shift)
					{
						// motorola remote

						switch(p.cmd.key)
						{
						case VK_F3: p.cmd.remote = RemoteKey::Red; break;
						case VK_F4: p.cmd.remote = RemoteKey::Green; break;
						case VK_F5: p.cmd.remote = RemoteKey::Yellow; break;
						case VK_F6: p.cmd.remote = RemoteKey::Blue; break;
						}
					}
					else if(p.mods == KeyEventParam::Ctrl)
					{
						// motorola remote

						switch(p.cmd.key)
						{
						case 'D': p.cmd.remote = RemoteKey::Details; break;
						}
					}
					else if((p.mods & KeyEventParam::Shift) != 0 && (p.mods & KeyEventParam::Ctrl) != 0)
					{
						// motorola remote

						switch(p.cmd.key)
						{
						case 'B': 
							p = KeyEventParam();
							p.cmd.app = APPCOMMAND_MEDIA_REWIND;
							printf("B\n");
							break;
						case 'F': 
							p = KeyEventParam();
							p.cmd.app = APPCOMMAND_MEDIA_FAST_FORWARD;
							printf("F\n");
							break;
						}
					}
					
					if(p.cmd.key == 'R' && (p.mods & KeyEventParam::Ctrl))
					{
						printf("RESET\n");

						PostMessage(m_hWnd, WM_RESETDEVICE, 0, 0);
					}
					
					if(OnInput(p)) return 0;
				}
				else if(message == WM_APPCOMMAND)
				{
					KeyEventParam p;

					p.cmd.app = GET_APPCOMMAND_LPARAM(lParam);

					// GET_DEVICE_LPARAM(lParam);
					// GET_KEYSTATE_LPARAM(lParam);

					if(OnInput(p)) return 0;
				}
				else if(message == WM_INPUT && GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
				{
					UINT size = 0;

					if(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER)) == 0)
					{
						std::vector<BYTE> buff(size);

						if(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buff.data(), &size, sizeof(RAWINPUTHEADER)) == size)
						{
							RAWINPUT* raw = (RAWINPUT*)buff.data();

							if(raw->header.dwType == RIM_TYPEHID)
							{
								RID_DEVICE_INFO info;

								memset(&info, 0, sizeof(info));

								info.cbSize = sizeof(info);

								size = sizeof(info);

								GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &info, &size);

								// 
								wprintf(L"* %04x %04x %d %d\n", info.hid.dwVendorId, info.hid.dwProductId, info.hid.usUsagePage, info.hid.usUsage);

								DWORD code = m_last_vk.code;
								DWORD mods = m_last_vk.mods;

								memset(&m_last_vk, 0, sizeof(m_last_vk));

								if(OnRawInput(info.hid.dwVendorId, info.hid.dwProductId, raw->data.hid.bRawData, raw->data.hid.dwSizeHid, code, mods))
								{
									return 0;
								}
							}
						}
					}
				}
			}
		}

		return __super::OnMessage(message, wParam, lParam);
	}

	bool PlayerWindow::OnCreate()
	{
		DragAcceptFiles(m_hWnd, TRUE);
		RegisterDragDrop(m_hWnd, this);

		m_dev = new Device9(false);

		if(!m_dev->Create(m_hWnd, true, (m_flags & 1) != 0))
		{
			return false;
		}

		m_dc = new DeviceContext9(m_dev, m_loader);

		m_mgr = new WindowManager();

		Control::DC = m_dc;

		m_renderer.Create();

		m_mouse_timer = SetTimer(m_hWnd, m_timer++, 500, NULL);

		if(GetSystemMetrics(SM_REMOTESESSION) == 0 && (m_flags & 8) == 0)
		{
			Util::Config cfg(m_config.c_str(), L"Settings");

			if(cfg.GetInt(L"Fullscreen", 1) != 0)
			{
				ToggleFullscreen(cfg.GetInt(L"Exclusive", 1) != 0);
			}
		}

		SetAlwaysOnTop();

		return true;
	}

	void PlayerWindow::OnDestroy()
	{
		m_renderer.Join(); // once this is closed we are the only thread accessing the UI (no need to lock on Control::m_lock)

		Control::DC = NULL;

		delete m_mgr;
		delete m_dc;
		delete m_dev;

		m_mgr = NULL;
		m_dc = NULL;
		m_dev = NULL;
	}

	void PlayerWindow::OnTimer(UINT id)
	{
		if(id == m_mouse_timer)
		{
			if(clock() - m_mouse_clock >= 5000)
			{
				SetCursor(NULL);
			}
		}
	}

	bool PlayerWindow::OnSetCursor(UINT nHitTest)
	{
		if(nHitTest == HTCLIENT)
		{
			SetCursor(clock() - m_mouse_clock < 5000 ? LoadCursor(0, IDC_ARROW) : NULL); // TODO: arrow?

			return true;
		}

		return false;
	}

	bool PlayerWindow::OnCopyData(COPYDATASTRUCT* data)
	{
		if(data->dwData != 0x6ABE51 || data->cbData < sizeof(DWORD))
		{
			return false;
		}

		DWORD size = *((DWORD*)data->lpData);
		WCHAR* buff = (WCHAR*)((DWORD*)data->lpData + 1);
		WCHAR* buffend = (TCHAR*)((BYTE*)buff + data->cbData - sizeof(DWORD));

		std::list<std::wstring> cmdln;

		while(size-- > 0)
		{
			std::wstring s = (WCHAR*)buff;

			cmdln.push_back(s);

			buff += s.size() + 1;
		}

		std::list<std::wstring> files;

		for(auto i = cmdln.begin(); i != cmdln.end(); )
		{
			std::wstring s = *i++;

			if(!s.empty())
			{
				if(s.size() > 1 && (s[0] == '-' || s[0] == '/'))
				{
					std::wstring sw = Util::MakeLower(s.substr(1));

					if(sw == L"uninstall" && i != cmdln.end()) 
					{
						ShellExecute(NULL, L"open", L"msiexec.exe", (L" /x " + *i).c_str(), NULL, SW_SHOWNORMAL); 
						
						PostMessage(m_hWnd, WM_CLOSE, 0, 0);
					}
				}
				else
				{
					if(files.empty()) // TODO
					{
						files.push_back(s);
					}
				}
			}
		}

		if(!files.empty())
		{
			OnDropFiles(files);
		}

		return true;
	}

	void PlayerWindow::OnDropFiles(HDROP hDrop)
	{
		SetForegroundWindow(m_hWnd);

		std::list<std::wstring> files;

		UINT n = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

		for(UINT i = 0; i < n; i++)
		{
			WCHAR buff[MAX_PATH];

			UINT res = DragQueryFile(hDrop, i, buff, MAX_PATH);

			if(res > 0)
			{
				buff[res] = 0;

				files.push_back(buff);
			}
		}

		DragFinish(hDrop);

		if(!files.empty())
		{
			OnDropFiles(files);
		}
	}

	bool PlayerWindow::OnRawInput(DWORD vid, DWORD pid, const BYTE* buff, DWORD size, DWORD code, DWORD mods)
	{
		DWORD res = RemoteKey::GetKey(vid, pid, buff, size, code, mods);
										
		if(res != 0)
		{
			KeyEventParam p;

			p.cmd.remote = res;

			return OnInput(p);
		}

		return false;
	}

	bool PlayerWindow::OnInput(const KeyEventParam& p)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		return m_mgr != NULL ? m_mgr->OnInput(p) : false;
	}

	bool PlayerWindow::OnInput(const MouseEventParam& p)
	{
		CAutoLock cAutoLock(&Control::m_lock);

		return m_mgr != NULL ? m_mgr->OnInput(p) : false;
	}

	STDMETHODIMP PlayerWindow::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
	{
		*pdwEffect = DROPEFFECT_NONE;

		FORMATETC fmt;

		memset(&fmt, 0, sizeof(fmt));

		fmt.cfFormat = RegisterClipboardFormat(L"UniformResourceLocator");
		fmt.dwAspect = DVASPECT_CONTENT;
		fmt.lindex = -1;
		fmt.tymed = TYMED_HGLOBAL;

		if(S_OK == pDataObj->QueryGetData(&fmt))
		{
			*pdwEffect = DROPEFFECT_COPY;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	STDMETHODIMP PlayerWindow::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
	{
		return S_OK;
	}

	STDMETHODIMP PlayerWindow::DragLeave()
	{
		return S_OK;
	}

	STDMETHODIMP PlayerWindow::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
	{
		HGLOBAL hGlobal = NULL;

		FORMATETC fmt;

		memset(&fmt, 0, sizeof(fmt));

		fmt.cfFormat = RegisterClipboardFormat(L"UniformResourceLocator");
		fmt.dwAspect = DVASPECT_CONTENT;
		fmt.lindex = -1;
		fmt.tymed = TYMED_HGLOBAL;

		STGMEDIUM stgMedium;

		if(SUCCEEDED(pDataObj->GetData(&fmt, &stgMedium)))
		{
			if(stgMedium.tymed == TYMED_HGLOBAL && stgMedium.pUnkForRelease == NULL)
			{
				if(const void* ptr = GlobalLock(stgMedium.hGlobal))
				{
					std::string url = (const char*)ptr;

					GlobalUnlock(stgMedium.hGlobal);

					SetForegroundWindow(m_hWnd);

					std::list<std::wstring> files;

					files.push_back(std::wstring(url.begin(), url.end()));

					OnDropFiles(files);
				}
			}

			ReleaseStgMedium(&stgMedium);
		}

		return S_OK;
	}
}
