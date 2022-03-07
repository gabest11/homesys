#pragma once

#include "MenuAudioSettings.dxui.h"

namespace DXUI
{
	class MenuAudioSettings : public MenuAudioSettingsDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnNext(Control* c);

	public:
		MenuAudioSettings();
	};
}
