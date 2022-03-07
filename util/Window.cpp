/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdAfx.h"
#include "Window.h"

namespace Util
{
	Window::Window()
		: m_hWnd(NULL)
	{
	}

	Window::~Window()
	{
	}

	LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		Window* wnd = NULL;

		if(message == WM_NCCREATE)
		{
			wnd = (Window*)((LPCREATESTRUCT)lParam)->lpCreateParams;

			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)wnd);

			wnd->m_hWnd = hWnd;
		}
		else
		{
			wnd = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		}

		if(wnd == NULL)
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		return wnd->OnMessage(message, wParam, lParam);
	}

	LRESULT Window::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_CLOSE:
			Hide();
			DestroyWindow(m_hWnd);
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			break;
		}

		return DefWindowProc(m_hWnd, message, wParam, lParam);
	}

	bool Window::Create(LPCWSTR title, int w, int h, LPCWSTR className)
	{
		m_title = title;

		WNDCLASS wc;

		memset(&wc, 0, sizeof(wc));

		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = WndProc;
		wc.hInstance = GetModuleHandle(NULL);
		// TODO: wc.hIcon = ;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszClassName = className;

		DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW | WS_BORDER;
		DWORD exStyle = 0;

		PreCreate(&wc, style, exStyle);

		if(!GetClassInfo(wc.hInstance, wc.lpszClassName, &wc))
		{
			if(!RegisterClass(&wc))
			{
				return false;
			}
		}

		Vector4i r;

		GetWindowRect(GetDesktopWindow(), r);

		if(w <= 0 || h <= 0) 
		{
			w = r.width() * 2 / 3;
			h = r.width() * 2 / 4;
		}

		r.left = (r.left + r.right - w) / 2;
		r.top = (r.top + r.bottom - h) / 2;
		r.right = r.left + w;
		r.bottom = r.top + h;

		AdjustWindowRect(r, style, FALSE);

		m_hWnd = CreateWindowEx(exStyle, wc.lpszClassName, title, style, r.left, r.top, r.width(), r.height(), NULL, NULL, wc.hInstance, (LPVOID)this);

		if(!m_hWnd)
		{
			return false;
		}

		RAWINPUTDEVICE rid[4];

		rid[0].usUsagePage = 0xFFBC; 
		rid[0].usUsage = 0x88; 
		rid[0].dwFlags = 0;
		rid[0].hwndTarget = m_hWnd;

		rid[1].usUsagePage = 0x0C;
		rid[1].usUsage = 0x01;
		rid[1].dwFlags = 0;
		rid[1].hwndTarget = m_hWnd;

		rid[2].usUsagePage = 0x0C;
		rid[2].usUsage = 0x80;
		rid[2].dwFlags = 0;
		rid[2].hwndTarget = m_hWnd;

		rid[3].usUsagePage = 0xFF00;
		rid[3].usUsage = 0x01;
		rid[3].dwFlags = RIDEV_INPUTSINK;
		rid[3].hwndTarget = m_hWnd;
		/*
		rid[4].usUsagePage = 0xFF0A;
		rid[4].usUsage = 0x01;
		rid[4].dwFlags = 0;
		rid[4].hwndTarget = m_hWnd;
		*/
		RegisterRawInputDevices(rid, sizeof(rid) / sizeof(rid[0]), sizeof(rid[0]));

		{
			UINT n = 0;

			if(GetRawInputDeviceList(NULL, &n, sizeof(RAWINPUTDEVICELIST)) == 0 && n > 0)
			{
				RAWINPUTDEVICELIST* devs = new RAWINPUTDEVICELIST[n];

				GetRawInputDeviceList(devs, &n, sizeof(RAWINPUTDEVICELIST));

				for(UINT i = 0; i < n; i++)
				{
					std::wstring name;

					UINT size = 0;

					GetRawInputDeviceInfo(devs[i].hDevice, RIDI_DEVICENAME, NULL, &size);

					if(size > 0)
					{
						wchar_t* buff = new wchar_t[size + 1];

						GetRawInputDeviceInfo(devs[i].hDevice, RIDI_DEVICENAME, buff, &size);

						buff[size] = 0;

						name = buff;

						delete [] buff;
					}

					if(name.empty()) continue;
					
					size = 0;

					GetRawInputDeviceInfo(devs[i].hDevice, RIDI_DEVICEINFO, NULL, &size);

					if(size > 0)
					{
						BYTE* buff = new BYTE[size + 1];

						GetRawInputDeviceInfo(devs[i].hDevice, RIDI_DEVICEINFO, buff, &size);

						RID_DEVICE_INFO* info = (RID_DEVICE_INFO*)buff;

						wprintf(L"[%d] %s\n%d %d\n", i, name.c_str(), devs[i].dwType, info->dwType);

						if(info->dwType == 2)
						{
							wprintf(L"v=%08x u=%04x up=%04x\n", info->hid.dwVersionNumber, info->hid.usUsage, info->hid.usUsagePage);
						}

						delete [] buff;
					}
					

				}

				delete [] devs;
			}
		}

		return Init();
	}

	bool Window::Attach(HWND hWnd)
	{
		// TODO: subclass

		m_hWnd = hWnd;

		return true;
	}

	Vector4i Window::GetClientRect()
	{
		Vector4i r;

		::GetClientRect(m_hWnd, r);
		
		return r;
	}

	void Window::SetWindowText(LPCWSTR title)
	{
		::SetWindowText(m_hWnd, title);
	}

	void Window::Show()
	{
		//SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		
		SetForegroundWindow(m_hWnd);
		
		ShowWindow(m_hWnd, SW_SHOWNORMAL);
		
		UpdateWindow(m_hWnd);
	}

	void Window::Hide()
	{
		ShowWindow(m_hWnd, SW_HIDE);
	}

	void Window::ShowFrame()
	{
		SetWindowLong(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE) | (WS_CAPTION | WS_THICKFRAME));
		
		//SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		UpdateWindow(m_hWnd);
	}

	void Window::HideFrame()
	{
		SetWindowLong(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME));
		
		//SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		
		UpdateWindow(m_hWnd);
	}
}