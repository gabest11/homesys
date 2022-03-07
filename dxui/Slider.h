#pragma once

#include "slider.dxui.h"

namespace DXUI
{
	class Slider : public SliderDXUI
	{
		void SetPosition(float& value);

		bool OnPaintBackground(Control* c, DeviceContext* dc);
		bool OnPaintBackgroundThumb(Control* c, DeviceContext* dc);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnClickedUpLeft(Control* c);
		bool OnClickedDownRight(Control* c);
		bool OnRectChanged(Control* c, const Vector4i& value);
		bool OnMarginChanged(Control* c, int margin);
		bool OnVerticalChanged(Control* c, bool vertical);

		void UpdateLayout();

		void SetVertex(Vertex* v, const Vector4i& p, const Vector4i& t);

	protected:
		bool m_drag;

	public:
		Slider();

		Property<float> Position;
		Property<float> Step;

		Event<float> ScrollEvent;
		Event<float> OverflowEvent;
	};
}
