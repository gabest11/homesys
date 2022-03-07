#pragma once

#include "Control.h"

namespace DXUI
{
	class Button : public Control
	{
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);

	public:
		Button();

		void Click();

		Event<> ClickedEvent;
	};

	class ButtonNF : public Button
	{
	public:
		ButtonNF() {CanFocus = 0;}
	};
}
