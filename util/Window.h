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

#pragma once

#include <windows.h>
#include <string>
#include "Vector.h"

namespace Util
{
	class Window
	{
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	protected:
		HWND m_hWnd;
		std::wstring m_title;

		virtual bool Init() {return true;}
		virtual void PreCreate(WNDCLASS* wc, DWORD& style, DWORD& exStyle) {}
		virtual LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

	public:
		Window();
		virtual ~Window();

		bool Create(LPCWSTR title, int w, int h, LPCWSTR className = L"HomesysUtilWindow");
		bool Attach(HWND hWnd);

		void* GetHandle() {return m_hWnd;}
		Vector4i GetClientRect();

		void SetWindowText(LPCWSTR title);

		void Show();
		void Hide();

		void ShowFrame();
		void HideFrame();
	};
}