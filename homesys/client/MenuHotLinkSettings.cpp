#include "stdafx.h"
#include "MenuHotLinkSettings.h"
#include "client.h"

namespace DXUI
{
	MenuHotLinkSettings::MenuHotLinkSettings() 
	{
		ActivatedEvent.Add(&MenuHotLinkSettings::OnActivated, this);
		DeactivatedEvent.Add(&MenuHotLinkSettings::OnDeactivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		m_next.ClickedEvent.Add0(&MenuHotLinkSettings::OnNext, this);

		m_timeout.m_list.ItemCount.Get([&] (int& count) {count = m_timeouts.size();});
		m_timeout.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			int t = m_timeouts[p->index];

			if(t == 0) 
			{
				p->text = Control::GetString("DISABLED");
			}
			else
			{
				std::wstring s = Control::GetString("SECONDS");

				if(t >= 60) {t /= 60; s = Control::GetString("MINUTES");}

				p->text = Util::Format(L"%d ", t) + s;
			}

			return true;
		});
	}

	bool MenuHotLinkSettings::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			Util::Config cfg(L"Homesys", L"HotLink");

			m_timeouts.clear();

			int t = cfg.GetInt(L"Timeout", 30);

			static int s_timeouts[] = {0, 10, 30, 60, 300, 600, 1800, 3600};

			for(int i = 0; i < sizeof(s_timeouts) / sizeof(s_timeouts[0]); i++)
			{
				m_timeouts.push_back(s_timeouts[i]);

				if(s_timeouts[i] == t) 
				{
					m_timeout.Selected = i;
				}
			}

			if(m_timeout.Selected == -1)
			{
				m_timeouts.push_back(t);

				m_timeout.Selected = m_timeouts.size() - 1;
			}

			m_program.Checked = cfg.GetInt(L"Program", 1) != 0;
			m_message.Checked = cfg.GetInt(L"Message", 1) != 0;
		}

		return true;
	}

	bool MenuHotLinkSettings::OnDeactivated(Control* c, int dir)
	{
		return true;
	}

	bool MenuHotLinkSettings::OnNext(Control* c)
	{
		Util::Config cfg(L"Homesys", L"HotLink");

		cfg.SetInt(L"Timeout", m_timeouts[m_timeout.Selected]);
		cfg.SetInt(L"Program", m_program.Checked ? 1 : 0);
		cfg.SetInt(L"Message", m_message.Checked ? 1 : 0);

		DoneEvent(this);

		g_env->InvalidateHotLinks();
		
		return true;
	}
}