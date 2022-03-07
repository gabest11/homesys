#pragma once

#include "Device9.h"
#include "DeviceContext9.h"
#include "ResourceLoader.h"
#include "Thread.h"
#include "WindowManager.h"
#include "../util/Window.h"

#define WM_GRAPHNOTIFY (WM_APP + 0)
#define WM_REACTIVATE_TUNER (WM_APP + 1)
#define WM_RESETDEVICE (WM_APP + 2)
#define WM_FAKE_APPCOMMAND (WM_APP + 3)
#define WM_PLAYERWINDOW_LAST (WM_APP + 3)

namespace DXUI
{
	class PlayerWindow 
		: public Util::Window
		, public CUnknown
		, public IDropTarget
	{
		DeviceContext* m_dc;
		std::wstring m_config;
		Thread m_renderer;
		bool m_fullscreen;
		bool m_exclusive;
		RECT m_rect;
		bool m_blocked;
		clock_t m_input_clock;
		clock_t m_mouse_clock;
		Vector2i m_mouse_pos;
		UINT m_mouse_timer;
		bool m_activated;
		bool m_minimized;
		DWORD m_mods;
		struct {DWORD code, mods;} m_last_vk;

		bool OnFlip(Control* c);
		void OnResetDevice(int mode);

	protected:
		DWORD m_flags;
		HANDLE m_flip;
		Device* m_dev;
		WindowManager* m_mgr;
		ResourceLoader* m_loader;
		int m_timer;

		LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
		
		virtual bool OnCreate();
		virtual void OnDestroy();
		virtual void OnSize(int type, int width, int height) {}
		virtual void OnTimer(UINT id);
		virtual bool OnSetCursor(UINT nHitTest);
		virtual bool OnCopyData(COPYDATASTRUCT* data);
		virtual void OnDropFiles(HDROP hDrop);
		virtual bool OnRawInput(DWORD vid, DWORD pid, const BYTE* buff, DWORD size, DWORD code, DWORD mods);
		virtual bool OnInput(const KeyEventParam& p);
		virtual bool OnInput(const MouseEventParam& p);
		virtual void OnPaint(DeviceContext* dc) {}
		virtual void OnPaintBusy(DeviceContext* dc) {}
		virtual void OnBeforeResetDevice() {}
		virtual void OnAfterResetDevice() {}
		virtual void OnDropFiles(const std::list<std::wstring>& files) {}
		virtual Vector4i GetWindowManagerRect(WindowManager* mgr, Vector4i& r) {return r;}

		void ToggleFullscreen(bool exclusive);
		bool SetAlwaysOnTop(int value = -1); // < 0 => default
		
		clock_t GetIdleTime() const {return clock() - m_input_clock;}

	public:
		PlayerWindow(LPCWSTR config, ResourceLoader* loader, DWORD flags);
		virtual ~PlayerWindow();

		DECLARE_IUNKNOWN;
		STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

		// IDropTarget

		STDMETHODIMP DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
		STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
		STDMETHODIMP DragLeave();
		STDMETHODIMP Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
	};
}