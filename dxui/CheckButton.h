#pragma once

#include "CheckButton.dxui.h"

namespace DXUI
{
	class CheckButton : public CheckButtonDXUI
	{
		bool OnPaintBackground(Control* c, DeviceContext* dc);
		bool OnClicked(Control* c);
		bool OnChecked(Control* c, bool value);

		Animation m_anim;

	public:
		CheckButton();
	};
}
