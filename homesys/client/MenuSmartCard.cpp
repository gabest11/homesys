#include "stdafx.h"
#include "MenuSmartCard.h"
#include "client.h"

namespace DXUI
{
	MenuSmartCard::MenuSmartCard()
	{
		ActivatedEvent.Add(&MenuSmartCard::OnActivated, this);
		DeactivatedEvent.Add(&MenuSmartCard::OnDeactivated, this);
		RedEvent.Add0(&MenuSmartCard::OnRed, this);
	}

	MenuSmartCard::~MenuSmartCard()
	{
	}

	bool MenuSmartCard::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			OnRed(this);
		}

		return true;
	}

	bool MenuSmartCard::OnDeactivated(Control* c, int dir)
	{
		if(dir < 0)
		{
			m_cards.clear();

			m_desc.Text = L"";
		}

		return true;
	}

	bool MenuSmartCard::OnRed(Control* c)
	{
		m_cards.clear();

		std::list<Homesys::SmartCardDevice> cards;

		if(g_env->svc.GetSmartCards(cards))
		{
			m_cards.insert(m_cards.end(), cards.begin(), cards.end());
		}

		std::wstring s;

		for(auto i = m_cards.begin(); i != m_cards.end(); i++)
		{
			s += Util::Format(L"[b u]{%s}\\n", DeviceContext::Escape(i->name.c_str()).c_str());
				
			if(i->inserted)
			{
				s += Util::Format(L"%s (%04x)\\n", DeviceContext::Escape(i->system.name.c_str()).c_str(), i->system.id);

				if(!i->serial.empty())
				{
					s += Util::Format(L"%s\\n", DeviceContext::Escape(i->serial.c_str()).c_str());
				}

				for(auto j = i->subscriptions.begin(); j != i->subscriptions.end(); j++)
				{
					s += L"[i] {";

					s += Util::Format(L"%s ", DeviceContext::Escape(j->name.c_str()).c_str());
						
					for(auto k = j->date.begin(); k != j->date.end(); k++)
					{
						CString t = k->Format(L"%Y-%m-%d");

						s += Util::Format(L"%s ", DeviceContext::Escape(t).c_str());
					}

					s += L"}\\n";
				}
			}
			else
			{
				s += L"-\\n";
			}

			s += Util::Format(L"\\n");
		}

		m_desc.Text = s;

		return true;
	}
}