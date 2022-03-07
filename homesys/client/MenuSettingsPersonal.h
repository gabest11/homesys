#pragma once

#include "MenuSettingsPersonal.dxui.h"

namespace DXUI
{
	class MenuSettingsPersonal : public MenuSettingsPersonalDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnNext(Control* c);

	public:
		MenuSettingsPersonal();
	};
}
