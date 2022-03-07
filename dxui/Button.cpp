#include "stdafx.h"
#include "Button.h"

namespace DXUI
{
	Button::Button()
	{
		KeyEvent.Add(&Button::OnKey, this);
		MouseEvent.Add(&Button::OnMouse, this);
		ClickedEvent.Chain(UserActionEvent);

		CanFocus = 1;
	}

	bool Button::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.key == VK_RETURN || p.cmd.key == VK_SPACE)
			{
				Click();

				return true;
			}
		}

		return false;
	}
	
	bool Button::OnMouse(Control* c, const MouseEventParam& p)
	{
		/*
		if((p.cmd == MouseEventParam::Down || p.cmd == MouseEventParam::Dbl) && Hovering)
		{
			Captured = true;

			return true;
		}
		else if(p.cmd == MouseEventParam::Up)
		{
			Captured = false;

			if(Hovering)
			{
				Click();
			}

			return true;
		}
		*/
		if((p.cmd == MouseEventParam::Down || p.cmd == MouseEventParam::Dbl) && Hovering)
		{
			Click();

			return true;
		}

		return false;
	}

	void Button::Click()
	{
		ClickedEvent(this);
	}
}