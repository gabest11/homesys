#include "stdafx.h"
#include "Dialog.h"

namespace DXUI
{
	Dialog::Dialog()
		: m_host(NULL)
	{
		KeyEvent.Add(&Dialog::OnKey, this);
		MouseEvent.Add(&Dialog::OnMouse, this);

		BackgroundImage = L"DialogBackground.png";
		BackgroundMargin = Vector4i(22, 22, 22, 22);
		Visible = false;
	}

	void Dialog::Show(Control* host)
	{
		if(!Visible)
		{
			m_host = host != NULL ? host : Parent;

			Visible = true;
			Focused = true;
			Captured = true;

			ToFront();
		}
	}

	void Dialog::Hide()
	{
		if(Visible)
		{
			Visible = false;
			Captured = false;

			if(m_host != NULL)
			{
				m_host->Focused = true;
			}
		}
	}

	void Dialog::Close()
	{
		Hide();

		HideEvent(this);
	}

	bool Dialog::OnDeactivated(Control* c, int dir)
	{
		Visible = false;

		return true;
	}

	bool Dialog::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.key == VK_ESCAPE || p.cmd.key == VK_BACK || p.cmd.app == APPCOMMAND_BROWSER_BACKWARD)
			{
				Close();

				return true;
			}
		}

		return false;
	}
	
	bool Dialog::OnMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::RDown)
		{
			Close();

			return true;
		}

		return false;
	}

	bool Dialog::OnClose(Control* c)
	{
		Close();

		return true;
	}
}