#pragma once

#include "StatusBar.dxui.h"

namespace DXUI
{
	class StatusBar : public StatusBarDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnPaintForegroundLeft(Control* c, DeviceContext* dc);

	public:
		StatusBar();
	};
}
