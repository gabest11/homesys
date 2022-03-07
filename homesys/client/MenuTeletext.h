#pragma once

#include "MenuTeletext.dxui.h"

namespace DXUI
{
	class MenuTeletext : public MenuTeletextDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);

	protected:

	public:
		MenuTeletext();
	};
}
