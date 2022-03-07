#pragma once

#include "radiobutton.dxui.h"

namespace DXUI
{
	class RadioButton : public RadioButtonDXUI
	{
		bool OnClicked(Control* c);
		bool OnChecked(Control* c, bool checked);

	public:
		RadioButton();
	};
}
