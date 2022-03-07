#pragma once

#include "List.dxui.h"
#include "Collection.h"

namespace DXUI
{
	class List : public ListDXUI, public Collection
	{
		bool OnSliderPositionChanged(Control* c, float pos);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnPaintItem(Control* c, PaintItemParam* p);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnFocusSet(Control* c);

		float m_pos;

	protected:
		int HitTest(const Vector2i& p);
		Vector4i GetItemRect(int i);

	public:
		List();

		void MakeVisible(int i, bool first = false);

		Property<int> ItemHeight;
		Property<float> ItemRows;

		Event<PaintItemParam*> PaintItemEvent;
		Event<int> ClickedEvent;
		Event<float> OverflowEvent;
	};
}
