#pragma once

#include "MenuAudioCD.dxui.h"

namespace DXUI
{
	class MenuAudioCD : public MenuAudioCDDXUI
	{
		bool OnLocationChanged(Control* c, TCHAR drive);

	public:
		MenuAudioCD();

		Property<TCHAR> Location;
	};
}
