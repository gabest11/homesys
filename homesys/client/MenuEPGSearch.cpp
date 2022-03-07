#include "stdafx.h"
#include "MenuEPGSearch.h"
#include "client.h"

namespace DXUI
{
	MenuEPGSearch::MenuEPGSearch()
	{
		ActivatedEvent.Add(&MenuEPGSearch::OnActivated, this);
		DeactivatedEvent.Add(&MenuEPGSearch::OnDeactivated, this);

		m_sortby_title.ClickedEvent.Add0(&MenuEPGSearch::OnSortClicked, this);
		m_sortby_channel.ClickedEvent.Add0(&MenuEPGSearch::OnSortClicked, this);
		m_sortby_start.ClickedEvent.Add0(&MenuEPGSearch::OnSortClicked, this);
		m_sortby_query.ClickedEvent.Add0(&MenuEPGSearch::OnSortClicked, this);

		m_search_types.push_back(Homesys::ProgramSearch::ByTitle);
		m_search_types.push_back(Homesys::ProgramSearch::ByMovieType);
		m_search_types.push_back(Homesys::ProgramSearch::ByDirector);
		m_search_types.push_back(Homesys::ProgramSearch::ByActor);
		m_search_types.push_back(Homesys::ProgramSearch::ByYear);

		m_findby.Selected.ChangedEvent.Add0([&] (Control* c) -> bool {return m_refresh.ClickedEvent(c);});
		m_findby.m_list.ItemCount.Get([&] (int& count) {count = m_search_types.size();});
		m_findby.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool
		{
			switch(m_search_types[p->index])
			{
			case Homesys::ProgramSearch::ByTitle: p->text = Control::GetString("TITLE"); break;
			case Homesys::ProgramSearch::ByMovieType: p->text = Control::GetString("GENRE"); break;
			case Homesys::ProgramSearch::ByDirector: p->text = Control::GetString("DIRECTOR"); break;
			case Homesys::ProgramSearch::ByActor: p->text = Control::GetString("ACTOR"); break;
			case Homesys::ProgramSearch::ByYear: p->text = Control::GetString("YEAR"); break;
			}

			return true;
		});

		m_query.ReturnKeyEvent.Chain(m_refresh.ClickedEvent);
		m_query.TextInputEvent.Chain(m_refresh.ClickedEvent);

		m_refresh.ClickedEvent.Add0([&] (Control* c) -> bool {m_changed = true; return true;});
		m_refresh.Enabled.Get([&] (bool& enabled) {enabled = !m_query.Text->empty();});
		m_refresh.Visible = false; // TODO

		m_list.ItemCount.Get([&] (int& count) {count = m_search_results.size();});
		m_list.PaintItemEvent.Add(&MenuEPGSearch::OnListPaintItem, this, true);
		m_list.ClickedEvent.Add(&MenuEPGSearch::OnListClicked, this);

		m_thread.TimerEvent.Add0(&MenuEPGSearch::OnThreadTimer, this);
		m_thread.TimerPeriod = 500;
	}

	bool MenuEPGSearch::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_query.Text = L"";
			m_query.Focused = true;
			m_findby.Selected = 0;

			m_search_results.clear();
			m_channels.clear();

			std::list<Homesys::Channel> channels;

			if(g_env->svc.GetChannels(channels))
			{
				for(auto i = channels.begin(); i != channels.end(); i++)
				{
					m_channels[i->id] = *i;
				}
			}
		}

		m_thread.Create();

		return true;
	}

	bool MenuEPGSearch::OnDeactivated(Control* c, int dir)
	{
		m_thread.Join(false);

		if(dir <= 0)
		{
			m_search_results.clear();
			m_channels.clear();
		}

		return true;
	}

	bool MenuEPGSearch::OnSortClicked(Control* c)
	{
		std::vector<Homesys::ProgramSearchResult*> psr;

		for(auto i = m_search_results.begin(); i != m_search_results.end(); i++)
		{
			psr.push_back(&*i);
		}

		static std::tr1::function<bool(const Homesys::ProgramSearchResult* a, const Homesys::ProgramSearchResult* b)> f[] = 
		{
			bind(&MenuEPGSearch::CompareByTitle, _1, _2),
			bind(&MenuEPGSearch::CompareByChannel, _1, _2),
			bind(&MenuEPGSearch::CompareByTime, _1, _2),
			bind(&MenuEPGSearch::CompareByQuery, _1, _2),
		};

		std::sort(psr.begin(), psr.end(), f[m_search.GetCheckedByGroup(0)->UserData]);

		std::vector<Homesys::ProgramSearchResult> tmp;
		
		for(auto i = psr.begin(); i != psr.end(); i++)
		{
			tmp.push_back(**i);
		}

		m_search_results.clear();
		m_search_results.swap(tmp);

		m_list.Selected = 0;

		return true;
	}

	bool MenuEPGSearch::OnListPaintItem(Control* c, PaintItemParam* p)
	{
		if(p->selected) 
		{
			p->dc->Draw(p->rect, 0x40000000);
		}

		Homesys::ProgramSearchResult& psr = m_search_results[p->index];

		Vector4i r = p->rect;

		int w = r.width();
		int h = r.height();

		Vector4i lr = Vector4i(r.left, r.top, r.left + 50, r.bottom).deflate(1);

		p->dc->Draw(lr, 0xffffffff);

		lr = lr.deflate(2);

		{
			Texture* t = m_ctf.GetTexture(psr.program.channelId, p->dc);

			if(t != NULL)
			{
				p->dc->Draw(t, DeviceContext::FitRect(t->GetSize(), lr));
			}
		}

		p->dc->TextHeight = h - 10;

		TextEntry* te = p->dc->Draw(L"0000-00-00 00:00", p->rect, Vector2i(0, 0), true);

		if(te != NULL)
		{
			r.left = p->rect.left + 55;
			r.right = p->rect.right - te->bbox.width() - 5; 

			std::wstring title = psr.program.episode.movie.title;
			std::wstring matches = Util::Implode(psr.matches, L"|");

			if(title != matches)
			{
				if(psr.matches.size() > 1) Util::Replace(matches, L"|", L", ");
				
				title += L" (" + matches + L")";
			}

			p->dc->TextHeight = h - 5;
			p->dc->TextAlign = Align::Left | Align::Middle;
			
			p->dc->Draw(title.c_str(), r);

			r.left = p->rect.right - te->bbox.width();
			r.right = p->rect.right; 

			std::wstring str = psr.program.start.Format(
				CTime::GetCurrentTime().Format(L"%Y-%m-%d") == psr.program.start.Format(L"%Y-%m-%d")
				? L"ma %H:%M" : L"%Y-%m-%d %H:%M");

			p->dc->TextHeight = h - 10;
			p->dc->TextAlign = Align::Center | Align::Middle;

			p->dc->Draw(str.c_str(), r);
		}

		return true;
	}

	bool MenuEPGSearch::OnListClicked(Control* c, int index)
	{
		UserData = m_search_results[index].program.id;

		g_env->RecordClickedEvent(this);

		return true;
	}

	bool MenuEPGSearch::OnThreadTimer(Control* c)
	{
		std::wstring query;
		Homesys::ProgramSearch::Type type;
		std::list<Homesys::ProgramSearchResult> psr;

		{
			CAutoLock cAutoLock(&m_lock);

			if(!m_changed) return true;

			m_changed = false;

			m_search_results.clear();

			query = m_query.Text;
			type = m_findby.Selected >= 0 ? m_search_types[m_findby.Selected] : Homesys::ProgramSearch::ByTitle;

			m_busy.Visible = true;
		}

		g_env->svc.SearchPrograms(query.c_str(), type, psr);

		{
			CAutoLock cAutoLock(&m_lock);

			m_search_results.clear();
			m_search_results.reserve(psr.size());

			for(auto i = psr.begin(); i != psr.end(); i++)
			{
				Homesys::ProgramSearchResult& r = *i;

				if(m_channels.find(r.program.channelId) != m_channels.end())
				{
					m_search_results.push_back(r);
				}
			}
			/*
			if(!m_search_results.empty())
			{
				m_list.Focused = true;
			}
			*/
			OnSortClicked(NULL);

			m_busy.Visible = false;
		}

		return true;
	}
	
	bool MenuEPGSearch::CompareByChannel(const Homesys::ProgramSearchResult* a, const Homesys::ProgramSearchResult* b)
	{
		if(a->program.channelId != b->program.channelId) 
		{
			return a->program.channelId < b->program.channelId;
		}

		if(int i = wcsicmp(a->program.episode.movie.title.c_str(), b->program.episode.movie.title.c_str()))
		{
			return i < 0;
		}

		if(int i = wcsicmp(a->matches.front().c_str(), b->matches.front().c_str()))
		{
			return i < 0;
		}

		if(a->program.start != b->program.start) 
		{
			return a->program.start.GetTime() < b->program.start.GetTime();
		}

		if(a->program.stop != b->program.stop) 
		{
			return a->program.stop.GetTime() < b->program.stop.GetTime();
		}

		return false;
	}

	bool MenuEPGSearch::CompareByTitle(const Homesys::ProgramSearchResult* a, const Homesys::ProgramSearchResult* b)
	{
		if(int i = wcsicmp(a->program.episode.movie.title.c_str(), b->program.episode.movie.title.c_str()))
		{
			return i < 0;
		}

		if(int i = wcsicmp(a->matches.front().c_str(), b->matches.front().c_str()))
		{
			return i < 0;
		}

		if(a->program.start != b->program.start) 
		{
			return a->program.start.GetTime() < b->program.start.GetTime();
		}

		if(a->program.stop != b->program.stop) 
		{
			return a->program.stop.GetTime() < b->program.stop.GetTime();
		}

		if(a->program.channelId != b->program.channelId) 
		{
			return a->program.channelId < b->program.channelId;
		}

		return false;
	}

	bool MenuEPGSearch::CompareByTime(const Homesys::ProgramSearchResult* a, const Homesys::ProgramSearchResult* b)
	{
		if(a->program.start != b->program.start) 
		{
			return a->program.start.GetTime() < b->program.start.GetTime();
		}

		if(a->program.stop != b->program.stop) 
		{
			return a->program.stop.GetTime() < b->program.stop.GetTime();
		}

		if(int i = wcsicmp(a->program.episode.movie.title.c_str(), b->program.episode.movie.title.c_str()))
		{
			return i < 0;
		}

		if(int i = wcsicmp(a->matches.front().c_str(), b->matches.front().c_str()))
		{
			return i < 0;
		}

		if(a->program.channelId != b->program.channelId) 
		{
			return a->program.channelId < b->program.channelId;
		}

		return false;
	}

	bool MenuEPGSearch::CompareByQuery(const Homesys::ProgramSearchResult* a, const Homesys::ProgramSearchResult* b)
	{
		if(int i = wcsicmp(a->matches.front().c_str(), b->matches.front().c_str()))
		{
			return i < 0;
		}

		if(int i = wcsicmp(a->program.episode.movie.title.c_str(), b->program.episode.movie.title.c_str()))
		{
			return i < 0;
		}

		if(a->program.start != b->program.start) 
		{
			return a->program.start.GetTime() < b->program.start.GetTime();
		}

		if(a->program.stop != b->program.stop) 
		{
			return a->program.stop.GetTime() < b->program.stop.GetTime();
		}

		if(a->program.channelId != b->program.channelId) 
		{
			return a->program.channelId < b->program.channelId;
		}

		return false;
	}
}