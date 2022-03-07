#pragma once

#include "ProgressBar.dxui.h"

namespace DXUI
{
	class ProgressBar : public ProgressBarDXUI
	{
		bool OnPaintBackground(Control* c, DeviceContext* dc);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnMouse(Control* c, const MouseEventParam& p);

		bool m_drag;

	public:
		ProgressBar();

		Property<float> Start;
		Property<float> Stop;
		Property<float> Position;

		Event<float> DragReleasedEvent;
	};
}
