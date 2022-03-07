#include "stdafx.h"
#include "MenuTV.h"
#include "client.h"

using namespace Homesys;

namespace DXUI
{
	MenuTV::MenuTV() 
	{
		ActivatedEvent.Add(&MenuTV::OnActivated, this);
		DeactivatedEvent.Add(&MenuTV::OnDeactivated, this);
		
		m_recordings.push_back(MenuList::Item(Control::GetString("BROWSE"), Recordings, L""));
		m_recordings.push_back(MenuList::Item(Control::GetString("SCHEDULE"), Schedule, L""));
		m_recordings.push_back(MenuList::Item(Control::GetString("WISHLIST"), Wishlist, L""));
		m_recordings.push_back(MenuList::Item(Control::GetString("BACK"), None, L"back.png", &m_root));
		
		m_presetselect.Selected.ChangedEvent.Add(&MenuTV::OnPresetSelected, this);
		m_presetselect.m_list.ItemCount.Get([&] (int& count) {count = m_presets.size();});
		m_presetselect.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_presets[p->index].name; return true;});

		m_thread.TimerEvent.Add(&MenuTV::OnThreadTimer, this);
		m_thread.TimerPeriod = 5000;
	}

	bool MenuTV::OnActivated(Control* c, int dir)
	{
		m_root.clear();
		m_settings.clear();

		std::list<Homesys::TunerReg> tuners;

		g_env->svc.GetTuners(tuners);

		m_root.push_back(MenuList::Item(Control::GetString("TV_OSD"), TV, L""));

		m_root.push_back(MenuList::Item(Control::GetString("EPG"), EPG, L""));
		m_root.push_back(MenuList::Item(Control::GetString("RECORDINGS"), Recordings, L"", &m_recordings));
		m_root.push_back(MenuList::Item(Control::GetString("SEARCH"), Search, L""));
		// TODO: m_root.push_back(MenuList::Item(Control::GetString("EPG_RECOMMEND"), Recommend, L""));

		for(auto i = tuners.begin(); i != tuners.end(); i++)
		{
			if(i->type == TunerDevice::DVBF)
			{
				m_root.push_back(MenuList::Item(Control::GetString("WEBTV_OSD"), WebTV, L""));

				break;
			}
		}

		if(!g_env->parental.enabled)
		{
			m_root.push_back(MenuList::Item(Control::GetString("SETTINGS"), Settings, L"", &m_settings));

			m_settings.push_back(MenuList::Item(Control::GetString("TUNERS"), Tuners, L""));
			m_settings.push_back(MenuList::Item(Control::GetString("SMARTCARDS"), SmartCards, L""));

			if(!tuners.empty())
			{
				m_settings.push_back(MenuList::Item(Control::GetString("TUNING"), Tuning, L""));
			}

			m_settings.push_back(MenuList::Item(Control::GetString("CHANNELS"), Presets, L""));
			m_settings.push_back(MenuList::Item(Control::GetString("CHANNEL_ORDER"), PresetsOrder, L""));
			m_settings.push_back(MenuList::Item(Control::GetString("BACK"), None, L"back.png", &m_root));
		}

		if(dir >= 0)
		{
			m_list.Items = &m_root;
			m_list.Selected = 0;
			m_list.Focused = true;
		}

		m_presets.clear();

		if(!g_env->players.empty())
		{
			CComPtr<IGenericPlayer> p = g_env->players.front();
			
			if(CComQIPtr<ITunerPlayer> t = p)
			{
				int presetId = 0;

				g_env->svc.GetCurrentPresetId(t->GetTunerId(), presetId);

				std::list<Preset> presets;

				g_env->svc.GetPresets(t->GetTunerId(), true, presets);

				m_presets.insert(m_presets.end(), presets.begin(), presets.end());

				for(int i = 0; i < m_presets.size(); i++)
				{
					if(m_presets[i].id == presetId)
					{
						m_presetselect.Selected = i;

						break;
					}
				}
			}
		}

		m_thread.Create();

		return true;
	}

	bool MenuTV::OnDeactivated(Control* c, int dir)
	{
		m_thread.Join(false);

		return true;
	}

	bool MenuTV::OnPresetSelected(Control* c, int index)
	{
		if(index >= 0)
		{
			if(!g_env->players.empty())
			{
				CComPtr<IGenericPlayer> p = g_env->players.front();

				if(CComQIPtr<ITunerPlayer> t = p)
				{
					int presetId = 0;

					g_env->svc.GetCurrentPresetId(t->GetTunerId(), presetId);

					if(presetId != m_presets[index].id)
					{
						t->SetPreset(m_presets[index].id);
					}

					m_thread.PostTimer();
				}
			}
		}

		return true;
	}

	bool MenuTV::OnThreadTimer(Control* c, UINT64 n)
	{
		Program program;

		if(!g_env->players.empty())
		{
			CComPtr<IGenericPlayer> p = g_env->players.front();

			if(CComQIPtr<ITunerPlayer> t = p)
			{
				if(g_env->svc.GetCurrentProgram(t->GetTunerId(), 0, program))
				{
					m_program = program;
				}
			}
		}

		CAutoLock cAutoLock(&m_lock);

		__super::m_program.CurrentProgram = m_program.id ? &m_program : NULL;

		return true;
	}
}