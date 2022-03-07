#pragma once

#include "MenuEPG.dxui.h"

namespace DXUI
{
	class MenuEPG : public MenuEPGDXUI
	{
		bool OnProgramClicked(Control* c, Homesys::Program* p);
		bool OnRed(Control* c);
		bool OnGreen(Control* c);
		bool OnBlue(Control* c);

	public:
		MenuEPG();

		bool Create(const Vector4i& rect, Control* parent);
	};
}
