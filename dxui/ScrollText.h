#pragma once

#include "Scrolltext.dxui.h"

namespace DXUI
{
	class ScrollText : public ScrollTextDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnKey(Control* c, const KeyEventParam& p);

	public:
		ScrollText();
	};

	class ScrollTextNF : public ScrollText
	{
	public:
		ScrollTextNF() {CanFocus = 0;}
	};
}
