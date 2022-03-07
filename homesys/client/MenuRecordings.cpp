#include "stdafx.h"
#include "MenuRecordings.h"
#include "client.h"

namespace DXUI
{
	MenuRecordings::MenuRecordings()
	{
		for(int i = 0; i < countof(m_sort); i++) m_sort[i] = 0;

		m_sort[1] = 1;

		ActivatedEvent.Add(&MenuRecordings::OnActivated, this);
		DeactivatedEvent.Add(&MenuRecordings::OnDeactivated, this);

		m_list.PaintItemEvent.Add(&MenuRecordings::OnListPaintItem, this, true);
		m_list.Selected.ChangedEvent.Add(&MenuRecordings::OnListSelected, this);
		m_list.ClickedEvent.Add(&MenuRecordings::OnListClicked, this);
		m_list.ItemCount.Get([&] (int& count) {count = m_items[m_tab.Selected].size();});
		m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_items[m_tab.Selected][p->index].name; return true;});
		m_list.RedEvent.Add0(&MenuRecordings::OnListRedEvent, this);
		m_list.GreenEvent.Add0(&MenuRecordings::OnListGreenEvent, this);

		m_dialog.m_play.ClickedEvent.Add0(&MenuRecordings::OnRecordingPlay, this);
		m_dialog.m_edit.ClickedEvent.Add0(&MenuRecordings::OnRecordingEdit, this);
		m_dialog.m_delete.ClickedEvent.Add0(&MenuRecordings::OnRecordingDelete, this);
		m_dialog.m_modify.ClickedEvent.Add0(&MenuRecordings::OnRecordingModify, this);

		m_tab.Selected.ChangedEvent.Add(&MenuRecordings::OnTabChanged, this);
		m_tab.ItemCount.Get([&] (int& count) {count = 3;});
		m_tab.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			switch(p->index)
			{
			case 0: p->text = ::DXUI::Control::GetString("RECORDINGS_COMPLETED"); break;
			case 1: p->text = ::DXUI::Control::GetString("RECORDINGS_SCHEDULED"); break;
			case 2: p->text = ::DXUI::Control::GetString("RECORDINGS_ERROR"); break;
			default: return false;
			}

			p->text += Util::Format(L" (%d)", m_items[p->index].size());
			
			return true;
		});

		m_tab.Selected = 0;

		m_info.Text.Get([&] (std::wstring& value) 
		{
			value.clear();

			if(m_list.Selected >= 0 && m_tab.Selected >= 0)
			{
				Homesys::Recording& r = m_items[m_tab.Selected][m_list.Selected];

				if(r.path.empty()) return;

				CTimeSpan ts = r.stop - r.start;

				if(ts.GetTotalMinutes() > 0)
				{
					value = Util::Format(L"%d %s %d %s", 
						ts.GetHours(), DXUI::Control::GetString("HOURS"), 
						ts.GetMinutes(), DXUI::Control::GetString("MINUTES"));
				}
				else 
				{
					value = Util::Format(L"%d %s", 
						ts.GetSeconds(), DXUI::Control::GetString("SECONDS"));
				}

				WIN32_FIND_DATA fd;

				memset(&fd, 0, sizeof(fd));

				HANDLE h = FindFirstFile(r.path.c_str(), &fd);

				if(h != NULL)
				{
					FindClose(h);

					LARGE_INTEGER size;
					
					size.HighPart = fd.nFileSizeHigh;
					size.LowPart = fd.nFileSizeLow;

					value += L", " + Util::FormatSize(size.QuadPart);
				}
			}
		});
	}

	bool MenuRecordings::Create(const Vector4i& rect, Control* parent)
	{
		if(!__super::Create(rect, parent))
			return false;

		Vector4i r = ClientRect;

		int x = (r.left + r.right) / 2;
		int y = (r.top + r.bottom) / 2;

		r.left = x - 200;
		r.right = x + 200;
		r.top = y - 130;
		r.bottom = y + 130;

		m_dialog.Create(r, this);
		m_dialog.AnchorRect = Vector4i(Anchor::Center, Anchor::Top, Anchor::Center, Anchor::Top);

		return true;
	}

	void MenuRecordings::SortRecordings(int col)
	{
		std::vector<Homesys::Recording>& items = m_items[col];

		int sort = m_sort[col];

		std::map<UINT64, CTime> rgstart;

		if((sort & 2) == 0)
		{
			for(auto i = items.begin(); i != items.end(); i++)
			{
				const Homesys::Recording& rec = *i;

				UINT64 h = rec.GetMovieGroupHash();

				if(h != 0)
				{
					auto i = rgstart.find(h);

					if(i != rgstart.end())
					{
						if(rec.state == Homesys::RecordingState::Scheduled)
						{
							if(i->second > rec.start)
							{
								i->second = rec.start;
							}
						}
						else
						{
							if(i->second < rec.start)
							{
								i->second = rec.start;
							}
						}
					}
					else
					{
						rgstart[h] = rec.start;
					}
				}
			}
		}

		auto f = [&] (const Homesys::Recording& a, const Homesys::Recording& b) -> bool 
		{
			CTime ta = a.start;
			CTime tb = b.start;

			if((sort & 2) == 0)
			{
				UINT64 ha = a.GetMovieGroupHash();
				UINT64 hb = b.GetMovieGroupHash();

				if(ha && hb && ha == hb)
				{
					return (sort & 1) != 0 ? a.start < b.start : a.start > b.start;
				}

				auto i = rgstart.find(ha);

				if(i != rgstart.end())
				{
					ta = i->second;
				}

				i = rgstart.find(hb);

				if(i != rgstart.end())
				{
					tb = i->second;
				}
			}

			return (sort & 1) != 0 ? ta < tb : ta > tb;
		};

		std::sort(items.begin(), items.end(), f);
	}

	bool MenuRecordings::OnActivated(Control* c, int dir)
	{
		m_channels.clear();

		std::list<Homesys::Channel> channels;

		if(g_env->svc.GetChannels(GUID_NULL, channels))
		{
			for(auto i = channels.begin(); i != channels.end(); i++)
			{
				m_channels[i->id] = *i;
			}
		}

		if(dir >= 0)
		{
			Util::Config cfg(L"Homesys", L"Recordings");

			for(int i = 0; i < countof(m_sort); i++) 
			{
				m_sort[i] = cfg.GetInt(Util::Format(L"Sort%d", i).c_str(), i == 1 ? 1 : 0);
			}

			m_tab.Selected = 0;

			for(int i = 0; i < sizeof(m_items) / sizeof(m_items[0]); i++)
			{
				m_items[i].clear();
			}

			std::list<Homesys::Recording> recordings;

			if(g_env->svc.GetRecordings(recordings))
			{
				int rating = g_env->parental.GetRating();

				for(int i = 0; i < sizeof(m_items) / sizeof(m_items[0]); i++)
				{
					m_items[i].reserve(recordings.size());
				}

				for(auto i = recordings.begin(); i != recordings.end(); i++)
				{
					if(i->state != Homesys::RecordingState::Deleted && i->rating < rating)
					{
						switch(i->state)
						{
						default:
						case Homesys::RecordingState::Aborted:
						case Homesys::RecordingState::Finished:
							m_items[0].push_back(*i);
							break;
						case Homesys::RecordingState::InProgress:
						case Homesys::RecordingState::Scheduled:
						case Homesys::RecordingState::Prompt:
							m_items[1].push_back(*i);
							break;
						case Homesys::RecordingState::Missed:
						case Homesys::RecordingState::Error:
						case Homesys::RecordingState::Overlapping:
							m_items[2].push_back(*i);
							break;
						}
					}
				}

				for(int i = 0; i < sizeof(m_items) / sizeof(m_items[0]); i++)
				{
					SortRecordings(i);
				}
			}

			if(m_list.Selected < 0) m_list.Selected = 0;
			else m_list.Selected = m_list.Selected;
		}

		return true;
	}

	bool MenuRecordings::OnDeactivated(Control* c, int dir)
	{
		if(dir <= 0)
		{
			for(int i = 0; i < sizeof(m_items) / sizeof(m_items[0]); i++)
			{
				m_items[i].clear();
			}
		}

		m_channels.clear();

		return true;
	}

	bool MenuRecordings::OnTabChanged(Control* c, int index)
	{
		m_list.Selected = 0;

		return true;
	}

	bool MenuRecordings::OnListPaintItem(Control* c, PaintItemParam* p)
	{
		std::vector<Homesys::Recording>& items = m_items[m_tab.Selected];

		if(p->selected) 
		{
			p->dc->Draw(p->rect, 0x40000000);
		}

		int sort = m_sort[m_tab.Selected];

		bool subitem = false;

		std::wstring col[3];

		if((sort & 2) == 0 && p->index > 0)
		{
			UINT64 ha = items[p->index - 1].GetMovieGroupHash();
			UINT64 hb = items[p->index].GetMovieGroupHash();

			if(ha && hb && ha == hb)
			{
				subitem = true;
			}
		}

		col[0] += items[p->index].name;

		const Homesys::Recording& rec = items[p->index];

		int channelId = rec.channelId;

		if(rec.program.id)
		{
			channelId = rec.program.channelId;
		}
		else if(rec.preset.id)
		{
			col[2] = rec.preset.name;

			channelId = rec.preset.channelId;
		}

		std::wstring t1 = CTime::GetCurrentTime().Format(L"%Y-%m-%d");
		std::wstring t2 = rec.start.Format(L"%Y-%m-%d");

		col[1] = std::wstring(rec.start.Format(t1 == t2 ? L"%H:%M" : L"%Y-%m-%d"));

		Vector4i r = p->rect;

		int w = r.width();
		int h = r.height();

		Vector4i lr = Vector4i(r.left, r.top, r.left + 50, r.bottom).deflate(1);

		p->dc->Draw(lr, 0xffffffff);
		
		auto i = m_channels.find(channelId);

		if(i != m_channels.end())
		{
			const Homesys::Channel& channel = i->second;

			col[2] = channel.name;

			Texture* t = m_ctf.GetTexture(channel.id, p->dc);

			if(t != NULL)
			{
				p->dc->Draw(t, p->dc->FitRect(t->GetSize(), lr.deflate(2)));
			}
		}

		p->dc->TextHeight = h - 10;

		Vector4i bbox;
		
		TextEntry* te = p->dc->Draw(L"0000-00-00", p->rect, Vector2i(0, 0), true);

		if(te != NULL)
		{
			r.left = p->rect.left + (subitem ? 100 : 55);
			r.right = p->rect.right - te->bbox.width(); 

			p->dc->TextHeight = h - (subitem ? 15 : 5);
			p->dc->TextAlign = Align::Left | Align::Middle;
			p->dc->Draw(col[0].c_str(), r);

			r.left = p->rect.right - te->bbox.width();
			r.right = p->rect.right; 

			Texture* t = m_rstf.GetTexture(rec.state, p->dc);

			if(t != NULL)
			{
				Vector4i r2 = r;

				r2.left = r.left - r.height() - 5;
				r2.right = r.left - 5;

				p->dc->Draw(t, r2);
			}

			p->dc->TextHeight = h - (subitem ? 15 : 10);
			p->dc->TextAlign = Align::Center | Align::Middle;
			p->dc->Draw(col[1].c_str(), r);
		}

		return true;
	}

	bool MenuRecordings::OnListSelected(Control* c, int index)
	{
		std::vector<Homesys::Recording>& items = m_items[m_tab.Selected];

		std::wstring path;

		Homesys::Program program;

		if(index >= 0)
		{
			path = items[index].path;
			program = items[index].program;
		}

		m_program = program;

		__super::m_program.CurrentProgram = m_program.id ? &m_program : NULL;

		if(index >= 0)
		{
			Homesys::RecordingState::Type state = items[index].state;

			m_dialog.m_modify.Enabled = 
				state == Homesys::RecordingState::Scheduled || 
				state == Homesys::RecordingState::Warning || 
				state == Homesys::RecordingState::InProgress || 
				state == Homesys::RecordingState::Overlapping;

			CTimeSpan ts = items[index].stop - items[index].start;

			printf("%lld\n", ts.GetTotalSeconds());
		}
		else
		{
			m_dialog.m_modify.Enabled = false;
		}

		return true;
	}

	bool MenuRecordings::OnListClicked(Control* c, int index)
	{
		bool exists = PathFileExists(m_items[m_tab.Selected][index].path.c_str()) != FALSE;

		m_dialog.m_play.Enabled = exists;
		m_dialog.m_edit.Enabled = exists;

		m_dialog.Show(&m_list);

		return true;
	}

	bool MenuRecordings::OnListRedEvent(Control* c)
	{
		int& sort = m_sort[m_tab.Selected];

		sort ^= 1;

		Util::Config cfg(L"Homesys", L"Recordings");

		cfg.SetInt(Util::Format(L"Sort%d", (int)m_tab.Selected).c_str(), sort);

		for(int i = 0; i < sizeof(m_items) / sizeof(m_items[0]); i++)
		{
			SortRecordings(i);
		}

		return true;
	}

	bool MenuRecordings::OnListGreenEvent(Control* c)
	{
		int& sort = m_sort[m_tab.Selected];

		sort ^= 2;

		Util::Config cfg(L"Homesys", L"Recordings");

		cfg.SetInt(Util::Format(L"Sort%d", (int)m_tab.Selected).c_str(), sort);

		for(int i = 0; i < sizeof(m_items) / sizeof(m_items[0]); i++)
		{
			SortRecordings(i);
		}

		return true;
	}

	bool MenuRecordings::OnRecordingPlay(Control* c)
	{
		if(m_list.Selected >= 0)
		{
			g_env->PlayFileEvent(this, m_items[m_tab.Selected][m_list.Selected].path);
		}

		return true;
	}

	bool MenuRecordings::OnRecordingEdit(Control* c)
	{
		if(m_list.Selected >= 0)
		{
			g_env->EditFileEvent(this, m_items[m_tab.Selected][m_list.Selected].path);
		}

		return true;
	}

	bool MenuRecordings::OnRecordingDelete(Control* c)
	{
		if(m_list.Selected >= 0)
		{
			int index = m_list.Selected;

			m_list.Selected = -1; // release player and file

			std::vector<Homesys::Recording>& items = m_items[m_tab.Selected];

			if(g_env->svc.DeleteRecording(items[index].id))
			{
				items.erase(items.begin() + index);

				if(index >= items.size())
				{
					index = items.size() - 1;
				}

				m_list.Selected = index;

				m_list.MakeVisible(index);
			}
		}

		return true;
	}

	bool MenuRecordings::OnRecordingModify(Control* c)
	{
		if(m_list.Selected >= 0)
		{
			g_env->ModifyRecording(this, m_items[m_tab.Selected][m_list.Selected].id);
		}

		return true;
	}

}