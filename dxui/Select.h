#pragma once

#include "Select.dxui.h"

namespace DXUI
{
	class Select : public SelectDXUI
	{
		bool OnPaintBegin(Control* c);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnLeftClicked(Control* c);
		bool OnRightClicked(Control* c);

		Animation m_anim;

	public:
		Select();
	};
}
