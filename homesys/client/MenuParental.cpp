#include "stdafx.h"
#include "MenuParental.h"
#include "client.h"

namespace DXUI
{
	MenuParental::MenuParental()
	{
		ActivatedEvent.Add(&MenuParental::OnActivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		m_password.TextInputEvent.Add0(&MenuParental::OnPasswordTextInput, this);
		m_password.Text.ChangedEvent.Add0(&MenuParental::OnUpdateNext, this);
		m_onoff.Selected.ChangedEvent.Add0(&MenuParental::OnUpdateNext, this);
		m_next.ClickedEvent.Add0(&MenuParental::OnNext, this);

		m_onoffitems.push_back(Control::GetString("OFF"));
		m_onoffitems.push_back(Control::GetString("ON"));
		
		m_onoff.m_list.ItemCount.Get([&] (int& count) {count = m_onoffitems.size();});
		m_onoff.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_onoffitems[p->index]; return true;});

		m_levelitems.push_back(Control::GetString("0YO"));
		m_levelitems.push_back(Control::GetString("12YO"));
		m_levelitems.push_back(Control::GetString("16YO"));
		m_levelitems.push_back(Control::GetString("18YO"));
		
		m_level.m_list.ItemCount.Get([&] (int& count) {count = m_levelitems.size();});
		m_level.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_levelitems[p->index]; return true;});

		m_inactivity.m_list.ItemCount.Get([&] (int& count) {count = m_inactivityitems.size();});
		m_inactivity.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			int n = m_inactivityitems[p->index];

			if(n == 0) p->text = L"-";
			else if(n < 60) p->text = Util::Format(L"%d m", n);
			else p->text = Util::Format(L"%d h", n / 60);

			return true;
		});
	}

	bool MenuParental::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_unlocked = false;

			g_env->svc.GetParentalSettings(m_ps);

			m_password.Text = L"";
			m_onoff.Selected = m_ps.enabled ? 1 : 0;
			m_level.Selected = m_ps.rating;

			static int presets[] = {0, 5, 10, 30, 60, 120, 180};

			m_inactivityitems.clear();

			m_inactivity.Selected = -1;

			for(int i = 0; i < countof(presets); i++)
			{
				m_inactivityitems.push_back(presets[i]);

				if(presets[i] >= m_ps.inactivity && m_inactivity.Selected < 0)
				{
					m_inactivity.Selected = i;
				}
			}

			OnPasswordTextInput(&m_password);

			m_prev.Focused = true;
		}

		return true;
	}

	bool MenuParental::OnPasswordTextInput(Control* c)
	{
		std::wstring password = m_password.Text;

		if(!m_unlocked)
		{
			m_unlocked = m_ps.password == password;

			m_onoff.Enabled = m_unlocked;
			m_level.Enabled = m_unlocked;
			m_inactivity.Enabled = m_unlocked;
		}

		OnUpdateNext(&m_password);

		return true;
	}

	bool MenuParental::OnUpdateNext(Control* c)
	{
		std::wstring password = m_password.Text;

		m_inactivity.Visible = m_onoff.Selected == 0;
		m_next.Enabled = m_unlocked && (!password.empty() || m_onoff.Selected == 0);

		return true;
	}

	bool MenuParental::OnNext(Control* c)
	{
		m_ps.password = m_password.Text;
		m_ps.enabled = m_onoff.Selected == 1;
		m_ps.rating = m_level.Selected;
		m_ps.inactivity = m_inactivityitems[m_inactivity.Selected];

		g_env->svc.SetParentalSettings(m_ps);

		g_env->parental = m_ps;

		DoneEvent(this);

		return true;
	}
}

