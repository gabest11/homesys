#pragma once

#include "MenuEditText.dxui.h"

namespace DXUI
{
	class MenuEditText : public MenuEditTextDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnTargetChanged(Control* c, Control* t);
		bool OnNextClicked(Control* c);
		bool OnTextChanged(Control* c, std::wstring text);

	public:
		MenuEditText();

		Property<Control*> Target;
	};
}
