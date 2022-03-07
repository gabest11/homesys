#pragma once

#include "EPGProgram.dxui.h"

namespace DXUI
{
	class EPGProgram : public EPGProgramDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnCurrentProgramChanged(Control* c, Homesys::Program* p);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnPaintScheduleForeground(Control* c, DeviceContext* dc);

	public:
		EPGProgram();

		Property<Homesys::Program*> CurrentProgram;
		Property<bool> MoreDescription;
		Property<bool> ShowPicture;
	};
}
