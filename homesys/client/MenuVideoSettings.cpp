#include "stdafx.h"
#include "MenuVideoSettings.h"
#include "client.h"
#include "../../DirectShow/FGManager.h"

namespace DXUI
{
	MenuVideoSettings::MenuVideoSettings() 
	{
		ActivatedEvent.Add(&MenuVideoSettings::OnActivated, this);
		DeactivatedEvent.Add(&MenuVideoSettings::OnDeactivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		m_next.ClickedEvent.Add0(&MenuVideoSettings::OnNext, this);
	}

	bool MenuVideoSettings::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			Util::Config cfg(L"Homesys", L"Decoder");

			m_h264.Checked = cfg.GetInt(L"H264", 1) != 0;
			m_dxva.Checked = cfg.GetInt(L"DXVA", 1) != 0;
		}

		return true;
	}

	bool MenuVideoSettings::OnDeactivated(Control* c, int dir)
	{
		return true;
	}

	bool MenuVideoSettings::OnNext(Control* c)
	{
		Util::Config cfg(L"Homesys", L"Decoder");

		cfg.SetInt(L"H264", m_h264.Checked ? 1 : 0);
		cfg.SetInt(L"DXVA", m_dxva.Checked ? 1 : 0);

		DoneEvent(this);
		
		return true;
	}
}