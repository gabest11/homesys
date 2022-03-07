#pragma once

#include "EPGView.h"

namespace DXUI
{
	class EPGViewCompact : public EPGView
	{
		bool OnPaintOverlayForeground(Control* c, DeviceContext* dc);

		void PaintFlag(CTime t, DeviceContext* dc, bool indicator);

	public:
		EPGViewCompact();
	};
}
