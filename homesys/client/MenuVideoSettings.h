#pragma once

#include "MenuVideoSettings.dxui.h"

namespace DXUI
{
	class MenuVideoSettings : public MenuVideoSettingsDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnNext(Control* c);

	public:
		MenuVideoSettings();
	};
}
