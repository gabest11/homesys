#pragma once

#include "Control.h"
#include "Collection.h"
#include "Animation.h"

namespace DXUI
{
	class Table : public Control, public Collection
	{
		void GetThumbWidth(int& value);
		void GetThumbHeight(int& value);

		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnPaintItemBackground(Control* c, PaintItemBackgroundParam* p);
		bool OnPaintItem(Control* c, PaintItemParam* p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnSelectedChanged(Control* c, int index);

		Animation m_anim;
		int m_anim_index;

	protected:
		int HitTest(const Vector2i& p);

	public:
		Table();

		Property<int> TopLeft;
		Property<int> ThumbWidth;
		Property<int> ThumbHeight;
		Property<int> ThumbGapX;
		Property<int> ThumbGapY;
		Property<int> ThumbCountH;
		Property<int> ThumbCountV;
		Property<int> SelectedSize;

		Event<PaintItemBackgroundParam*> PaintItemBackgroundEvent;
		Event<PaintItemParam*> PaintItemEvent;
		Event<int> ClickedEvent;
	};
}
