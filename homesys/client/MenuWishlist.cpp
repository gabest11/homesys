#include "stdafx.h"
#include "MenuWishlist.h"
#include "client.h"

namespace DXUI
{
	MenuWishlist::MenuWishlist()
	{
		ActivatedEvent.Add(&MenuWishlist::OnActivated, this);
		DeactivatedEvent.Add(&MenuWishlist::OnDeactivated, this);

		m_channel_list.ItemCount.Get([&] (int& count) {count = m_channels.size();});
		m_channel_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_channels[p->index].name; return true;});

		m_wishlist_list.BlueEvent.Add0(&MenuWishlist::OnDelete, this);
		m_wishlist_list.ItemCount.Get([&] (int& count) {count = m_recordings.size();});
		m_wishlist_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool
		{
			const Homesys::WishlistRecording& r = m_recordings[p->index];

			std::list<std::wstring> sl;
			
			if(!r.title.empty()) sl.push_back(Util::Format(L"%s: %s", Control::GetString("TITLE"), r.title.c_str()));
			if(!r.desc.empty()) sl.push_back(Util::Format(L"%s: %s", Control::GetString("DESCRIPTION"), r.desc.c_str()));
			if(!r.actor.empty()) sl.push_back(Util::Format(L"%s: %s", Control::GetString("ACTOR"), r.actor.c_str()));
			if(!r.director.empty()) sl.push_back(Util::Format(L"%s: %s", Control::GetString("DIRECTOR"), r.director.c_str()));
			if(r.channelId > 0) {auto i = m_channel_map.find(r.channelId); if(i != m_channel_map.end()) sl.push_back(i->second);}
			
			p->text = Util::Implode(sl, L", "); 
			
			return true;
		});

		m_wishlist_list.ClickedEvent.Add([&] (Control* c, int index) -> bool
		{
			const Homesys::WishlistRecording& r = m_recordings[index];

			m_title2.Text = r.title;
			m_desc.Text = r.desc;
			m_actor.Text = r.actor;
			m_director.Text = r.director;
			m_channel_list.Selected = 0;
			
			for(int i = 0; i < m_channels.size(); i++)
			{
				if(m_channels[i].id == r.channelId)
				{
					m_channel_list.Selected = i;

					break;
				}
			}

			return true;
		});

		m_add.ClickedEvent.Add0(&MenuWishlist::OnConfirmAdd, this);

		m_confirm.m_yes.ClickedEvent.Add0(&MenuWishlist::OnAdd, this);

		m_program_list.ItemCount.Get([&] (int& count) {count = m_programs.size();});
		m_program_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			setlocale(LC_TIME, ".ACP");

			std::wstring s = m_programs[p->index].start.Format(L"%b %#d. %H:%M - ");
			
			setlocale(LC_TIME, "English");

			p->text = s + m_programs[p->index].episode.movie.title;

			if(!m_programs[p->index].episode.title.empty())
			{
				p->text = p->text + L" (" + m_programs[p->index].episode.title + L")";
			}

			auto i = m_channel_map.find(m_programs[p->index].channelId); 
			
			if(i != m_channel_map.end()) 
			{
				p->text = i->second + L", " + p->text;
			}

			return true;
		});

		m_program_list.ClickedEvent.Add([&] (Control* c, int index) -> bool
		{
			m_program_list.UserData = m_programs[index].id; 
			
			g_env->RecordClickedEvent(&m_program_list); 
		
			return true;
		});
	}

	bool MenuWishlist::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_title2.Text = L"";
			m_desc.Text = L"";
			m_actor.Text = L"";
			m_director.Text = L"";

			m_recordings.clear();

			std::list<Homesys::WishlistRecording> recordings;

			if(g_env->svc.GetWishlistRecordings(recordings))
			{
				for(auto i = recordings.begin(); i != recordings.end(); i++)
				{
					m_recordings.push_back(*i);
				}
			}

			m_channel_map.clear();

			std::list<Homesys::Channel> channels;

			std::map<int, Homesys::Channel*> channel_map;

			if(g_env->svc.GetChannels(channels))
			{
				for(auto i = channels.begin(); i != channels.end(); i++)
				{
					m_channel_map[i->id] = i->name;

					channel_map[i->id] = &*i;
				}
			}

			m_channels.clear();

			Homesys::Channel c;

			c.name = Control::GetString("WISHLIST_ANY_CHANNEL");

			m_channels.push_back(c);

			std::list<Homesys::TunerReg> tuners;

			if(g_env->svc.GetTuners(tuners))
			{
				for(auto i = tuners.begin(); i != tuners.end(); i++)
				{
					Homesys::TunerReg& tr = *i;

					std::list<Homesys::Preset> presets;

					if(g_env->svc.GetPresets(tr.id, true, presets))
					{
						for(auto i = presets.begin(); i != presets.end(); i++)
						{
							Homesys::Preset& p = *i;

							if(p.channelId > 0)
							{
								auto i = channel_map.find(p.channelId);

								if(i != channel_map.end())
								{
									m_channels.push_back(*i->second);

									channel_map.erase(i);
								}
							}
						}
					}
				}
			}

			m_programs.clear();
		}

		return true;
	}

	bool MenuWishlist::OnDeactivated(Control* c, int dir)
	{
		if(dir <= 0)
		{
			m_recordings.clear();
			m_channel_map.clear();
			m_channels.clear();
			m_programs.clear();
		}

		return true;
	}

	bool MenuWishlist::OnConfirmAdd(Control* c)
	{
		Homesys::WishlistRecording r;

		r.title = m_title2.Text;
		r.desc = m_desc.Text;
		r.actor = m_actor.Text;
		r.director = m_director.Text;
		r.channelId = m_channel_list.Selected >= 0 ? m_channels[m_channel_list.Selected].id : 0;

		m_programs.clear();

		std::list<Homesys::Program> programs;

		if(g_env->svc.GetProgramsForWishlist(r, programs))
		{
			m_programs.insert(m_programs.end(), programs.begin(), programs.end());
		}

		m_program_list.Visible = !m_programs.empty();

		m_confirm.Show();

		return true;
	}

	bool MenuWishlist::OnAdd(Control* c)
	{
		Homesys::WishlistRecording r;

		r.title = m_title2.Text;
		r.desc = m_desc.Text;
		r.actor = m_actor.Text;
		r.director = m_director.Text;
		r.channelId = m_channel_list.Selected >= 0 ? m_channels[m_channel_list.Selected].id : 0;

		GUID id;

		if(g_env->svc.RecordByWishlist(r, id))
		{
			int index = -1;

			m_recordings.clear();

			std::list<Homesys::WishlistRecording> recordings;

			if(g_env->svc.GetWishlistRecordings(recordings))
			{
				for(auto i = recordings.begin(); i != recordings.end(); i++)
				{
					m_recordings.push_back(*i);

					if(i->id == id) 
					{
						index = m_recordings.size() - 1;
					}
				}
			}

			m_wishlist_list.Selected = index;
		}

		return true;
	}

	bool MenuWishlist::OnDelete(Control* c)
	{
		int i = m_wishlist_list.Selected;

		if(i >= 0 && i < m_recordings.size())
		{
			if(g_env->svc.DeleteWishlistRecording(m_recordings[i].id))
			{
				m_recordings.erase(m_recordings.begin() + i);

				if(i >= m_recordings.size())
				{
					m_wishlist_list.Selected = i - 1;
				}
			}
		}

		return true;
	}
}