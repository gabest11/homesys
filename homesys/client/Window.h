#pragma once

#include "Window.dxui.h"

namespace DXUI
{
	class Window : public WindowDXUI
	{
		bool OnPaintBegin(Control* c);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnBannerYes(Control* c);
		bool OnBannerNo(Control* c);

	protected:

		void ShowBanner();
		void HideBanner();
		void ToggleBanner();

		std::set<UINT> m_closekey;
		Animation m_banner_anim;

	public:
		Window();

		virtual void Show() {}
		virtual void Hide() {}

		static Event<> NavigateBackEvent;
		static Event<> ExitEvent;
	};
}
