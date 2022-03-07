#pragma once

#include "MenuHotLinkSettings.dxui.h"

namespace DXUI
{
	class MenuHotLinkSettings : public MenuHotLinkSettingsDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnNext(Control* c);

		std::vector<int> m_timeouts;

	public:
		MenuHotLinkSettings();
	};
}
