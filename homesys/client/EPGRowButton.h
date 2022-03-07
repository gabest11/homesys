#pragma once

#include "EPGRowButton.dxui.h"

namespace DXUI
{
	class EPGRowButton : public EPGRowButtonDXUI
	{
		bool OnCurrentProgramChanged(Control* c, Homesys::Program* program);
		bool OnPaintForeground(Control* c, DeviceContext* dc);

	public:
		EPGRowButton();

		Property<Homesys::Program*> CurrentProgram;
		Property<CTime> HeaderStart;
		Property<CTime> HeaderStop;
	};
}
