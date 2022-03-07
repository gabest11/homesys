#pragma once

#include "HotLinkStrip.dxui.h"
#include "Environment.h"

namespace DXUI
{
	class HotLinkStrip : public HotLinkStripDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnPaintForeground(Control* c, DeviceContext* dc);

		void Step(int dir);
		void Open();
		void PaintHotLink(HotLink* hl, const Vector4& r, bool selected, DeviceContext* dc);
		void GetInfoRect(int i, Vector4i& r);

		Animation m_anim;
		clock_t m_current;
		clock_t m_target;

	public:
		HotLinkStrip();

		Property<float> Interactive;
	};
}
