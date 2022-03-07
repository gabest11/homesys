#pragma once

#include "TVControl.dxui.h"

namespace DXUI
{
	class TVControl : public TVControlDXUI
	{
		void SetCurrentProgram(Homesys::Program*& value);

		bool OnDeviceContextChanged(Control* c, DeviceContext* dc);
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnUpdatePreset(Control* c);
		bool OnThreadTimer(Control* c, UINT64 n);
		bool OnThreadMessage(Control* c, const MSG& msg);

		Homesys::Program m_program;
		Thread m_thread;

	public:
		TVControl();

		Property<Homesys::Program*> CurrentProgram;
	};
}
