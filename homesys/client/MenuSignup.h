#pragma once

#include "MenuSignup.dxui.h"

namespace DXUI
{
	class MenuSignup : public MenuSignupDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnNext(Control* c);

	public:
		MenuSignup();
	};
}
