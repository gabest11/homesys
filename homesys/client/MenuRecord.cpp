#include "stdafx.h"
#include "MenuRecord.h"
#include "client.h"

namespace DXUI
{
	MenuRecord::MenuRecord()
	{
		ActivatedEvent.Add(&MenuRecord::OnActivated, this);
		DeactivatedEvent.Add(&MenuRecord::OnDeactivated, this);

		m_menu_items.push_back(Item(Control::GetString("RECORD"), 0, L""));
		m_menu_items.push_back(Item(Control::GetString("RECORD_ON_CHANNEL"), 1, L""));
		m_menu_items.push_back(Item(Control::GetString("RECORD_EVERYWHERE"), 2, L""));
		m_menu_items.push_back(Item(Control::GetString("RECORD_CANCEL"), 3, L""));
		m_menu_items.push_back(Item(Control::GetString("RECORD_CANCEL_ALL"), 4, L""));

		m_menu_list.MenuClickedEvent.Add(&MenuRecord::OnMenuItemClicked, this);
		m_menu_list.Items = &m_menu_items;

		m_program_list.Selected.ChangedEvent.Add(&MenuRecord::OnProgramSelected, this);
		m_program_list.ItemCount.Get([&] (int& count) {count = m_program_items.size();});
		m_program_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_program_items[p->index].start.Format(L"%Y-%m-%d %H:%M"); return true;});
	}

	bool MenuRecord::Create(const Vector4i& rect, Control* parent)
	{
		if(!__super::Create(rect, parent))
			return false;

		return true;
	}

	bool MenuRecord::OnActivated(Control* c, int dir)
	{
		m_program_list.Selected = 0;

		std::list<Homesys::Channel> channels;

		if(g_env->svc.GetChannels(channels))
		{
			for(auto i = channels.begin(); i != channels.end(); i++)
			{
				m_channels[i->id] = *i;
			}
		}

		Homesys::Program program;

		if(g_env->svc.GetProgram(CurrentProgramId, program))
		{
			std::list<Homesys::Program> programs;

			if(g_env->svc.SearchProgramsByMovie(program.episode.movie.id, programs))
			{
				CTime now = CTime::GetCurrentTime();

				for(auto i = programs.begin(); i != programs.end(); i++)
				{
					const Homesys::Program& p = *i;

					auto j = m_channels.find(p.channelId);

					if(j != m_channels.end())
					{
						m_program_items.push_back(p);
					}
				}
			}

			for(int i = 0; i < m_program_items.size(); i++)
			{
				if(m_program_items[i].id == program.id)
				{
					m_program_list.Selected = i;
					m_program_list.MakeVisible(i, true);

					break;
				}
			}
		}

		m_program_list.Visible = m_program_items.size() > 1;

		return true;
	}

	bool MenuRecord::OnDeactivated(Control* c, int dir)
	{
		m_program_items.clear();
		m_channels.clear();

		return true;
	}

	bool MenuRecord::OnMenuItemClicked(Control* c, int index)
	{
		int i = m_program_list.Selected;

		if(i >= 0 && i < m_program_items.size())
		{
			Homesys::Program& p = m_program_items[i];

			switch(index)
			{
			case 0:
				g_env->ProgramScheduleEvent(this, &p);
				break;
			case 1:
				g_env->ProgramScheduleByMovieAndChannelEvent(this, &p);
				break;
			case 2:
				g_env->ProgramScheduleByMovieEvent(this, &p);
				break;
			case 3:
				g_env->ProgramCancelRecordingEvent(this, &p);
				break;
			case 4:
				g_env->ProgramCancelMovieRecordingEvent(this, &p);
				break;
			}

			for(auto i = m_program_items.begin(); i != m_program_items.end(); i++)
			{
				g_env->svc.GetProgram(i->id, *i);
			}
		}

		// TODO: update m_program_items

		return true;
	}

	bool MenuRecord::OnProgramSelected(Control* c, int index)
	{
		if(index >= 0 && index < m_program_items.size())
		{
			Homesys::Program* p = &m_program_items[index];

			m_program.CurrentProgram = p;

			std::wstring s;

			if(p != NULL && p->episode.movie.id)
			{
				s = g_env->FormatMoviePictureUrl(p->episode.movie.id);
			}

			m_image.BackgroundImage = s;
		}
		else
		{
			m_program.CurrentProgram = NULL;
		}

		return true;
	}
}
