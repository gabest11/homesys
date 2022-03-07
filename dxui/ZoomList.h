#pragma once

#include "ZoomList.dxui.h"
#include "Collection.h"

namespace DXUI
{
	class ZoomList : public ZoomListDXUI, public Collection
	{
		bool OnActivated(Control* c, int dir);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnPaintItemBackground(Control* c, PaintItemBackgroundParam* p);
		bool OnPaintItem(Control* c, PaintItemParam* p);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnFocusSet(Control* c);
		bool OnHorizontalChanged(Control* c, bool hor);

		float m_pos;
		float m_scrollpos;

	protected:
		int HitTest(const Vector2i& p);
		Vector4i GetItemRect(int i);
		float GetItemHeight(int i);

	public:
		ZoomList();

		Property<int> ItemHeight;
		Property<int> ActiveItemHeight;
		Property<float> ItemRows;

		Event<PaintItemBackgroundParam*> PaintItemBackgroundEvent;
		Event<PaintItemParam*> PaintItemEvent;
		Event<int> ClickedEvent;
		Event<float> OverflowEvent;
	};
}
