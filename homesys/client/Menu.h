#pragma once

#include "Menu.dxui.h"

namespace DXUI
{
	class Menu : public MenuDXUI
	{
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnPaintBackground(Control* c, DeviceContext* dc);
		bool OnPaintLogoBackground(Control* c, DeviceContext* dc);

		Animation m_anim;
		Animation m_anim2;

	protected:
		bool Message(const KeyEventParam& p);
		bool Message(const MouseEventParam& p);

	public:
		Menu();
	};
}
