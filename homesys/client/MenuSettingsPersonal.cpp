#include "stdafx.h"
#include "MenuSettingsPersonal.h"
#include "client.h"
#include "region.h"
#include "../../util/Config.h"

namespace DXUI
{
	MenuSettingsPersonal::MenuSettingsPersonal() 
	{
		ActivatedEvent.Add(&MenuSettingsPersonal::OnActivated, this);
		DeactivatedEvent.Add(&MenuSettingsPersonal::OnDeactivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		m_next.ClickedEvent.Add0(&MenuSettingsPersonal::OnNext, this);

		m_country_list.m_list.ItemCount.Get([&] (int& count) {count = g_region_count;});
		m_country_list.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = g_region[p->index].name; return true;});

		m_gender_list.m_list.ItemCount.Get([&] (int& count) {count = 2;});
		m_gender_list.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			switch(p->index)
			{
			case 0:
				p->text = ::DXUI::Control::GetString("MALE");
				break;
			case 1:
				p->text = ::DXUI::Control::GetString("FEMALE"); 
				break;
			}

			return true;
		});
	}

	bool MenuSettingsPersonal::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			Homesys::UserInfo info;
			
			g_env->svc.GetUserInfo(info);

			m_email.Text = info.email;
			m_firstname.Text = info.firstName;
			m_lastname.Text = info.lastName;
			m_address.Text = info.address;
			m_postalcode.Text = info.postalCode;
			m_phonenumber.Text = info.phoneNumber;

			m_country_list.Selected = -1;

			std::wstring region = info.country;

			if(region.empty())
			{
				g_env->svc.GetRegion(region);
			}

			for(int i = 0; i < g_region_count; i++)
			{
				if(region == g_region[i].id)
				{
					m_country_list.Selected = i;

					break;
				}
			}

			m_gender_list.Selected = -1;

			if(info.gender == L"M")
			{
				m_gender_list.Selected = 0;
			}
			else if(info.gender == L"F")
			{
				m_gender_list.Selected = 1;
			}

			m_dialog.Show();
		}

		return true;
	}

	bool MenuSettingsPersonal::OnDeactivated(Control* c, int dir)
	{
		return true;
	}

	bool MenuSettingsPersonal::OnNext(Control* c)
	{
		Homesys::UserInfo info;

		info.email = m_email.Text;
		info.firstName = m_firstname.Text;
		info.lastName = m_lastname.Text;
		info.address = m_address.Text;
		info.postalCode = m_postalcode.Text;
		info.phoneNumber = m_phonenumber.Text;

		if(m_country_list.Selected >= 0)
		{
			info.country = g_region[m_country_list.Selected].id;
		}

		if(m_gender_list.Selected >= 0)
		{
			info.gender = m_gender_list.Selected == 0 ? L"M" : L"F";
		}

		if(g_env->svc.SetUserInfo(info)) 
		{
			DoneEvent(this);

			return true;
		}

		MessageBeep(-1);

		return false;
	}
}