#include "stdafx.h"
#include "MenuPresets.h"
#include "client.h"

namespace DXUI
{
	MenuPresets::MenuPresets() 
		: m_presets(NULL)
	{
		ActivatedEvent.Add(&MenuPresets::OnActivated, this);
		DeactivatedEvent.Add(&MenuPresets::OnDeactivated, this);
		DoneEvent.Chain(NavigateBackEvent);

		m_enabled.Checked.ChangedEvent.Add(&MenuPresets::OnEnabledChecked, this);
		m_name.TextInputEvent.Add0(&MenuPresets::OnNameTextInput, this);
		m_freq.KeyEvent.Add(&MenuPresets::OnFreqKey, this);
		m_next.ClickedEvent.Add0(&MenuPresets::OnNextClicked, this);		

		m_presetlist.PaintItemEvent.Add(&MenuPresets::OnPresetListPaintItem, this, true);
		m_presetlist.ClickedEvent.Add(&MenuPresets::OnPresetListClicked, this);
		m_presetlist.KeyEvent.Add(&MenuPresets::OnPresetListKey, this);
		m_presetlist.ItemCount.Get([&] (int& count) {count = m_presets ? m_presets->size() : 0;});
		m_presetlist.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			std::wstring s;

			if(m_presets != NULL)
			{
				s = Util::Format(L"%d. %s", p->index + 1, (*m_presets)[p->index].name.c_str());
			}

			p->text = s;

			return true;
		});

		m_channelselect.Selected.ChangedEvent.Add(&MenuPresets::OnChannelSelected, this);
		m_channelselect.m_list.ItemCount.Get([&] (int& count) {count = m_channels.size();});
		m_channelselect.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_channels[p->index].name; return true;});
		
		m_parentallevels.push_back(Control::GetString("PARENTAL_LEVEL_OFF"));
		m_parentallevels.push_back(Control::GetString("PARENTAL_LEVEL_12"));
		m_parentallevels.push_back(Control::GetString("PARENTAL_LEVEL_16"));
		m_parentallevels.push_back(Control::GetString("PARENTAL_LEVEL_18"));
		m_parentallevels.push_back(Control::GetString("PARENTAL_LEVEL_FULL"));

		m_parentalselect.Selected.ChangedEvent.Add(&MenuPresets::OnParentalSelected, this);
		m_parentalselect.m_list.ItemCount.Get([&] (int& count) {count = m_parentallevels.size();});
		m_parentalselect.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_parentallevels[p->index]; return true;});
		
		m_tunerselect.Selected.ChangedEvent.Add(&MenuPresets::OnTunerSelected, this);
		m_tunerselect.m_left.ClickedEvent.Chain(g_env->PrevPlayerEvent);
		m_tunerselect.m_right.ClickedEvent.Chain(g_env->NextPlayerEvent);
		m_tunerselect.m_list.ItemCount.Get([&] (int& count) {count = m_tuners.size();});
		m_tunerselect.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			p->text = Util::Format(L"%d. %s", p->index + 1, m_tuners[p->index].name.c_str());
			return true;
		});
	}

	bool MenuPresets::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_tuners.clear();
			m_channels.clear();
			m_presets = NULL;

			for(auto i = g_env->players.begin(); i != g_env->players.end(); i++)
			{
				CComPtr<IGenericPlayer> p = *i;

				if(CComQIPtr<ITunerPlayer> t = p)
				{
					std::list<Homesys::Preset> presets;

					if(g_env->svc.GetPresets(t->GetTunerId(), false, presets))
					{
						Tuner tuner;

						tuner.tuner = t;
						tuner.presets.reserve(presets.size());
						tuner.lastpreset = 0;

						std::list<Homesys::TunerReg> trs;

						g_env->svc.GetTuners(trs);
						
						for(auto i = trs.begin(); i != trs.end(); i++)
						{
							if(i->id == t->GetTunerId())
							{
								tuner.name = i->name;

								break;
							}
						}

						for(auto i = presets.begin(); i != presets.end(); i++)
						{
							if(i->enabled) tuner.presets.push_back(*i);
						}

						for(auto i = presets.begin(); i != presets.end(); i++)
						{
							if(!i->enabled) tuner.presets.push_back(*i);
						}

						m_tuners.push_back(tuner);
					}
				}
			}

			Homesys::Channel c;
			c.name = Control::GetString("CHANNEL_NOT_SET");
			m_channels.push_back(c);

			std::list<Homesys::Channel> channels;
			g_env->svc.GetChannels(GUID_NULL, channels);
			m_channels.insert(m_channels.end(), channels.begin(), channels.end());

			m_tunerselect.Selected = 0;
			m_tunerselect.Visible = m_tunerselect.m_list.ItemCount >= 2;
		}

		return true;
	}

	bool MenuPresets::OnDeactivated(Control* c, int dir)
	{
		if(dir <= 0)
		{
			m_tuners.clear();
		}

		return true;
	}

	bool MenuPresets::OnPresetListPaintItem(Control* c, PaintItemParam* p)
	{
		if(p->selected) 
		{
			p->dc->Draw(p->rect, 0x40000000);
		}

		Homesys::Preset& preset = (*m_presets)[p->index];

		c->TextBold = preset.enabled > 0;
		
		p->dc->TextStyle = c->TextStyle;

		Vector4i r = p->rect;

		r.left += 2;

		p->dc->Draw(p->text.c_str(), r);

		return true;
	}

	bool MenuPresets::OnPresetListClicked(Control* c, int index)
	{
		if(index < 0)
		{
			m_name.Text = L"";
			m_channelselect.Selected = -1;
			m_parentalselect.Selected = -1;
			m_video_text.Text = L"";
			m_enabled.Checked = false;

			return false;
		}

		const Homesys::Preset& p = (*m_presets)[index];

		m_name.Text = p.name;

		for(int i = 0, j = m_channels.size(); i < j; i++)
		{
			if(m_channels[i].id == p.channelId)
			{
				m_channelselect.Selected = i;
			}
		}

		m_parentalselect.Selected = p.rating;

		Tuner& t = m_tuners[m_tunerselect.Selected];

		t.lastpreset = index;

		t.tuner->SetPreset(p.id);

		std::wstring str = Util::Format(L"%d", p.tunereq.freq);

		m_freq.Text = str;
		m_freq.Enabled = p.tunereq.connector.type == PhysConn_Video_Tuner;

		Homesys::Preset cp;

		if(g_env->svc.GetCurrentPreset(t.tuner->GetTunerId(), cp))
		{
			if(cp.tunereq.connector.type == PhysConn_Video_Tuner)
			{
				//str.Format(L"%.2f MHz", (float)cp.tunereq.freq / 1000000);

				Homesys::TunerStat ts;
				
				if(g_env->svc.GetTunerStat(t.tuner->GetTunerId(), ts))
				{
					str = Util::Format(L"%d KHz", ts.freq);
				}
			}
			else
			{
				str = Util::Format(L"%s", cp.tunereq.connector.name);
			}
		}

		m_video_text.Text = str;

		m_enabled.Checked = !!p.enabled;

		return true;
	}

	bool MenuPresets::OnPresetListKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			int sel = m_tunerselect.Selected;

			if(sel >= 0 && sel < m_tuners.size())
			{
				const Tuner& t = m_tuners[sel];

				sel = m_presetlist.Selected;

				if(sel >= 0 && sel < m_presets->size())
				{
					const Homesys::Preset& preset = (*m_presets)[sel];

					if(p.cmd.app == APPCOMMAND_MEDIA_CHANNEL_UP)
					{
						Homesys::Preset tmp = preset;
						m_presets->erase(m_presets->begin() + sel);
						m_presets->insert(m_presets->begin() + sel - 1, tmp);
						m_presetlist.Selected = sel - 1;

						return true;
					}

					if(p.cmd.app == APPCOMMAND_MEDIA_CHANNEL_DOWN)
					{
						Homesys::Preset tmp = preset;
						m_presets->erase(m_presets->begin() + sel);
						m_presets->insert(m_presets->begin() + sel + 1, tmp);
						m_presetlist.Selected = sel + 1;
					
						return true;
					}

					// TODO

					if(p.cmd.key == VK_F3 || p.cmd.remote == RemoteKey::Yellow)
					{
						if(g_env->svc.DeletePreset(preset.id))
						{
							m_presets->erase(m_presets->begin() + sel);
							m_presetlist.Selected = std::min<int>(sel, m_presets->size() - 1);
						}

						return true;
					}
				}

				// TODO

				if(p.cmd.key == VK_F1 || p.cmd.remote == RemoteKey::Red)
				{
					Homesys::Preset p;

					if(g_env->svc.CreatePreset(t.tuner->GetTunerId(), p))
					{
						m_presets->push_back(p);
						m_presetlist.Selected = m_presets->size() - 1;
					}

					return true;
				}
			}
		}

		return false;
	}

	bool MenuPresets::OnEnabledChecked(Control* c, bool checked)
	{
		int i = m_presetlist.Selected;

		if(i >= 0)
		{
			m_presetlist.MakeVisible(i);
			
			(*m_presets)[i].enabled = checked ? i + 1 : 0;
		}

		return true;
	}

	bool MenuPresets::OnNameTextInput(Control* c)
	{
		int i = m_presetlist.Selected;

		if(i >= 0)
		{
			m_presetlist.MakeVisible(i);

			(*m_presets)[i].name = c->Text;
		}
		
		return true;
	}

	bool MenuPresets::OnChannelSelected(Control* c, int index)
	{
		int i = m_presetlist.Selected;

		if(i >= 0 && index >= 0)
		{
			m_presetlist.MakeVisible(i);

			(*m_presets)[i].channelId = m_channels[index].id;
		}

		return true;
	}

	bool MenuPresets::OnParentalSelected(Control* c, int index)
	{
		int i = m_presetlist.Selected;

		if(i >= 0 && index >= 0)
		{
			(*m_presets)[i].rating = index;
		}

		return true;
	}

	bool MenuPresets::OnTunerSelected(Control* c, int index)
	{
		if(index >= 0 && index < m_tuners.size())
		{
			m_presets = &m_tuners[index].presets;
			
			int lastpreset = m_tuners[index].lastpreset;

			m_presetlist.Selected = lastpreset;
			
			if(lastpreset >= 0 && lastpreset < m_presets->size())
			{
				m_presetlist.ClickedEvent(this, m_tuners[index].lastpreset);
			}
		}

		return true;
	}

	bool MenuPresets::OnFreqKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.key == VK_RETURN)
			{
				int i = m_presetlist.Selected;

				if(i >= 0)
				{
					m_presetlist.MakeVisible(i);

					Homesys::Preset& p = (*m_presets)[i];

					p.tunereq.freq = _wtoi(m_freq.Text->c_str());

					g_env->svc.UpdatePreset(p);

					m_presetlist.Selected = i;
				}

				return true;
			}
		}

		return false;
	}

	bool MenuPresets::OnNextClicked(Control* c)
	{
		for(auto i = m_tuners.begin(); i != m_tuners.end(); i++)
		{
			const Tuner& t = *i;

			std::list<Homesys::Preset> presets;
			std::list<int> ids;

			for(auto i = t.presets.begin(); i != t.presets.end(); i++)
			{
				if(i->enabled)
				{
					presets.push_back(*i);
					ids.push_back(i->id);
				}
			}

			if(!g_env->svc.SetPresets(t.tuner->GetTunerId(), ids))
			{
				return false;
			}
			
			if(!g_env->svc.UpdatePresets(presets))
			{
				return false;
			}
		}

		DoneEvent(this);

		return true;
	}
}