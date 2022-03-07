#include "stdafx.h"
#include "MenuPresetsOrder.h"
#include "client.h"

namespace DXUI
{
	MenuPresetsOrder::MenuPresetsOrder() 
		: m_tuner(NULL)
	{
		ActivatedEvent.Add(&MenuPresetsOrder::OnActivated, this);
		DeactivatedEvent.Add(&MenuPresetsOrder::OnDeactivated, this);
		DoneEvent.Chain(NavigateBackEvent);
		RedEvent.Add0(&MenuPresetsOrder::OnSortByCurrent, this);
		YellowEvent.Add0(&MenuPresetsOrder::OnSortByName, this);
		BlueEvent.Add0(&MenuPresetsOrder::OnSortByRank, this);

		m_next.ClickedEvent.Add0(&MenuPresetsOrder::OnNextClicked, this);		

		m_src_list.PaintItemEvent.Add(&MenuPresetsOrder::OnPresetListPaintItem, this, true);
		m_src_list.ClickedEvent.Add(&MenuPresetsOrder::OnPresetListClicked, this);
		m_src_list.ItemCount.Get([&] (int& count) {count = m_tuner != NULL ? m_tuner->src.size() : 0;});
		m_src_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			std::wstring s;

			if(m_tuner != NULL)
			{
				s = Util::Format(L"%d. %s", p->index + 1, m_tuner->src[p->index].name.c_str());
			}

			p->text = s;

			return true;
		});

		m_dst_list.ClickedEvent.Add(&MenuPresetsOrder::OnPresetListClicked, this);
		m_dst_list.KeyEvent.Add(&MenuPresetsOrder::OnPresetListKey, this);
		m_dst_list.RedEvent.Add0(&MenuPresetsOrder::OnMoveUp, this);
		m_dst_list.GreenEvent.Add0(&MenuPresetsOrder::OnMoveDown, this);
		m_dst_list.ItemCount.Get([&] (int& count) {count = m_tuner != NULL ? m_tuner->dst.size() : 0;});
		m_dst_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			std::wstring s;

			if(m_tuner != NULL)
			{
				s = Util::Format(L"%d. %s", p->index + 1, m_tuner->dst[p->index].name.c_str());
			}

			p->text = s;

			return true;
		});
		
		m_tunerselect.Selected.ChangedEvent.Add(&MenuPresetsOrder::OnTunerSelected, this);
		m_tunerselect.m_list.ItemCount.Get([&] (int& count) {count = m_tuners.size();});
		m_tunerselect.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			p->text = m_tuners[p->index].name;

			return true;
		});
	}

	bool MenuPresetsOrder::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_rank.clear();
			m_tuners.clear();
			m_tuner = NULL;

			std::list<Homesys::Channel> channels;

			g_env->svc.GetChannels(GUID_NULL, channels);

			for(auto i = channels.begin(); i != channels.end(); i++)
			{
				if(i->rank > 0) 
				{
					m_rank[i->id] = i->rank;
				}
			}

			std::list<Homesys::TunerReg> tuners;

			g_env->svc.GetTuners(tuners);

			for(auto i = tuners.begin(); i != tuners.end(); i++)
			{
				std::list<Homesys::Preset> presets;

				if(g_env->svc.GetPresets(i->id, false, presets))
				{
					Tuner t;

					t.id = i->id;
					t.name = i->name;

					for(auto i = presets.begin(); i != presets.end(); i++)
					{
						if(i->enabled) t.src.push_back(*i);
					}

					for(auto i = presets.begin(); i != presets.end(); i++)
					{
						if(!i->enabled) t.src.push_back(*i);
					}
					/*
					for(auto i = t.src.begin(); i != t.src.end(); i++)
					{
						i->enabled = 0;
					}
					*/
					
					m_tuners.push_back(t);
				}
			}

			m_tunerselect.Selected = 0;
		}

		return true;
	}

	bool MenuPresetsOrder::OnDeactivated(Control* c, int dir)
	{
		if(dir <= 0)
		{
			m_rank.clear();
			m_tuners.clear();
			m_tuner = NULL;
		}

		return true;
	}

	bool MenuPresetsOrder::OnSortByRank(Control* c)
	{
		if(m_tuner != NULL)
		{
			m_tuner->dst.clear();

			std::vector<Homesys::Preset*> presets;

			for(auto i = m_tuner->src.begin(); i != m_tuner->src.end(); i++)
			{
				if(i->enabled)
				{
					presets.push_back(&*i);
				}
			}

			std::sort(presets.begin(), presets.end(), [&](const Homesys::Preset* a, const Homesys::Preset* b) -> bool
			{
				auto i = m_rank.find(a->channelId);
				auto j = m_rank.find(b->channelId);

				int a_rank = i != m_rank.end() ? i->second : INT_MAX;
				int b_rank = j != m_rank.end() ? j->second : INT_MAX;

				return a_rank < b_rank || a_rank == b_rank && a < b;
			});

			for(auto i = presets.begin(); i != presets.end(); i++)
			{
				Homesys::Preset* p = *i;

				//p->enabled = 1;

				m_tuner->dst.push_back(*p);
			}
		}

		return true;
	}

	bool MenuPresetsOrder::OnSortByName(Control* c)
	{
		if(m_tuner != NULL)
		{
			m_tuner->dst.clear();

			std::vector<Homesys::Preset*> presets;

			for(auto i = m_tuner->src.begin(); i != m_tuner->src.end(); i++)
			{
				if(i->enabled)
				{
					presets.push_back(&*i);
				}
			}

			std::sort(presets.begin(), presets.end(), [&](const Homesys::Preset* a, const Homesys::Preset* b) -> bool
			{
				return wcscmp(a->name.c_str(), b->name.c_str()) < 0;
			});

			for(auto i = presets.begin(); i != presets.end(); i++)
			{
				Homesys::Preset* p = *i;

				//p->enabled = 1;

				m_tuner->dst.push_back(*p);
			}
		}

		return true;
	}

	bool MenuPresetsOrder::OnSortByCurrent(Control* c)
	{
		if(m_tuner != NULL)
		{
			m_tuner->dst.clear();

			std::vector<Homesys::Preset*> presets;

			for(auto i = m_tuner->src.begin(); i != m_tuner->src.end(); i++)
			{
				i->enabled = 1;

				m_tuner->dst.push_back(*i);
			}
		}

		return true;
	}

	bool MenuPresetsOrder::OnMoveUp(Control* c)
	{
		if(m_tuner != NULL)
		{
			int index = m_dst_list.Selected;

			if(index > 0 && index < m_tuner->dst.size())
			{
				Homesys::Preset tmp = m_tuner->dst[index];
				m_tuner->dst.erase(m_tuner->dst.begin() + index);
				m_tuner->dst.insert(m_tuner->dst.begin() + index - 1, tmp);
				m_dst_list.Selected = index - 1;
			}
		}

		return true;
	}

	bool MenuPresetsOrder::OnMoveDown(Control* c)
	{
		if(m_tuner != NULL)
		{
			int index = m_dst_list.Selected;

			if(index >= 0 && index < m_tuner->dst.size() - 1)
			{
				Homesys::Preset tmp = m_tuner->dst[index];
				m_tuner->dst.erase(m_tuner->dst.begin() + index);
				m_tuner->dst.insert(m_tuner->dst.begin() + index + 1, tmp);
				m_dst_list.Selected = index + 1;
			}
		}

		return true;
	}

	bool MenuPresetsOrder::OnPresetListPaintItem(Control* c, PaintItemParam* p)
	{
		if(p->selected) 
		{
			p->dc->Draw(p->rect, 0x40000000);
		}

		Homesys::Preset& preset = m_tuner->src[p->index];

		c->TextBold = preset.enabled > 0;

		p->dc->TextStyle = c->TextStyle;

		Vector4i r = p->rect;

		r.left += 2;

		p->dc->Draw(p->text.c_str(), r);

		return true;
	}

	bool MenuPresetsOrder::OnPresetListClicked(Control* c, int index)
	{
		if(m_tuner != NULL)
		{
			if(c == &m_src_list)
			{
				if(index >= 0 && index < m_tuner->src.size())
				{
					Homesys::Preset& p = m_tuner->src[index];

					for(auto i = m_tuner->dst.begin(); i != m_tuner->dst.end(); i++)
					{
						if(i->id == p.id)
						{
							return false;
						}
					}

					p.enabled = 1;

					m_tuner->dst.push_back(p);

					m_dst_list.Selected = m_tuner->dst.size() - 1;
				}
			}
			else if(c == &m_dst_list)
			{
				if(index >= 0 && index < m_tuner->dst.size())
				{
					const Homesys::Preset& p = m_tuner->dst[index];

					for(auto i = m_tuner->src.begin(); i != m_tuner->src.end(); i++)
					{
						if(i->id == p.id)
						{
							i->enabled = 0;
						}
					}

					m_tuner->dst.erase(m_tuner->dst.begin() + index);

					if(index >= m_tuner->dst.size())
					{
						index--;
					}

					m_dst_list.Selected = index;
				}
			}
		}

		return true;
	}

	bool MenuPresetsOrder::OnPresetListKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			switch(p.cmd.app)
			{
			case APPCOMMAND_MEDIA_CHANNEL_UP: return RedEvent(c);
			case APPCOMMAND_MEDIA_CHANNEL_DOWN: return GreenEvent(c);
			}
		}

		return false;
	}

	bool MenuPresetsOrder::OnTunerSelected(Control* c, int index)
	{
		if(index >= 0 && index < m_tuners.size())
		{
			m_tuner = &m_tuners[index];

			m_src_list.Selected = 0;
			m_dst_list.Selected = 0;
		}

		return true;
	}

	bool MenuPresetsOrder::OnNextClicked(Control* c)
	{
		for(auto i = m_tuners.begin(); i != m_tuners.end(); i++)
		{
			const Tuner& t = *i;

			std::list<int> ids;

			for(auto i = t.dst.begin(); i != t.dst.end(); i++)
			{
				if(i->enabled)
				{
					ids.push_back(i->id);
				}
			}

			if(ids.empty())
			{
				continue;
			}

			if(!g_env->svc.SetPresets(t.id, ids))
			{
				return false;
			}
		}

		DoneEvent(this);

		return true;
	}
}