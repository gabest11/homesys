#include "stdafx.h"
#include "MenuTeletext.h"
#include "client.h"
#include "../../DirectShow/TeletextFilter.h"

namespace DXUI
{
	MenuTeletext::MenuTeletext() 
	{
		ActivatedEvent.Add(&MenuTeletext::OnActivated, this);
		KeyEvent.Add(&MenuTeletext::OnKey, this);
	}

	bool MenuTeletext::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_teletext.Focused = true;
		}

		return true;
	}

	bool MenuTeletext::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.remote == RemoteKey::Teletext)
			{
				Close();

				return true;
			}
		}

		return false;
	}
}
