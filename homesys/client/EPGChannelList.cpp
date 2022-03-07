#include "stdafx.h"
#include "EPGChannelList.h"
#include "client.h"

namespace DXUI
{
	EPGChannelList::EPGChannelList()
	{
		CurrentChannelId.Set(&EPGChannelList::SetCurrentChannelId, this);
		CurrentChannel.Get([&] (Homesys::Channel*& value) {value = m_channel.id ? &m_channel : NULL;});
		CurrentProgram.Get([&] (Homesys::Program*& value) {value = Selected >= 0 ? &m_items[Selected] : NULL;});
		CurrentDate.Set(&EPGChannelList::SetCurrentDate, this);

		ActivatedEvent.Add(&EPGChannelList::OnActivated, this);
		DeactivatedEvent.Add(&EPGChannelList::OnDeactivated, this);

		ItemCount.Get([&] (int& count) {count = m_items.size();});
		ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_items[p->index].episode.movie.title; return true;});
		PaintItemEvent.Add(&EPGChannelList::OnListPaintItem, this, true);
		OverflowEvent.Add(&EPGChannelList::OnListScrollOverflow, this);
		Selected.ChangingEvent.Add(&EPGChannelList::OnListSelectedChanging, this);
		m_slider.OverflowEvent.Add(&EPGChannelList::OnListScrollOverflow, this);

		ItemHeight = 60;
		TextHeight = 40;
		TextColor = Color::Black;
		TextMarginLeft = 0;
		CurrentDate = CTime::GetCurrentTime();
	}

	void EPGChannelList::Update(const CTime& date)
	{
		m_items.clear();

		Selected = -1;

		Homesys::Channel* channel = CurrentChannel;
		
		if(channel != NULL)
		{
			CTime start = CTime(date.GetYear(), date.GetMonth(), date.GetDay(), 0, 0, 0);
			CTime stop = start + CTimeSpan(1, 0, 0, 0);

			std::list<Homesys::Program> programs;

			g_env->svc.GetPrograms(GUID_NULL, channel->id, start, stop, programs);

			while(!programs.empty() && programs.front().start < start)
			{
				programs.pop_front();
			}

			m_items.resize(programs.size());

			int i = 0;
			int j = 0;

			for(auto k = programs.begin(); k != programs.end(); k++)
			{
				m_items[i++] = *k;

				if(k->start <= date)
				{
					j = i;
				}
			}

			Selected = j;

			MakeVisible(j, true);
		}
	}

	void EPGChannelList::SetCurrentChannelId(int& value)
	{
		m_channel.id = 0;

		g_env->svc.GetChannel(value, m_channel);

		Update(CTime::GetCurrentTime());
	}

	void EPGChannelList::SetCurrentDate(CTime& value)
	{
		Update(value);
	}

	bool EPGChannelList::OnActivated(Control* c, int dir)
	{
		Update(CTime::GetCurrentTime());

		return true;
	}

	bool EPGChannelList::OnDeactivated(Control* c, int dir)
	{
		m_items.clear();

		return true;
	}

	bool EPGChannelList::OnListPaintItem(Control* c, PaintItemParam* p)
	{
		TextEntry* te = p->dc->Draw(L"00:00", p->rect, Vector2i(0, 0), true);

		Vector4i item_rect;
		Vector4i time_rect;
		Vector4i title_rect;

		item_rect = p->rect.deflate(Vector4i(0, 0, 10, 0));

		time_rect = item_rect;
		time_rect.right = item_rect.left + te->bbox.width() + 5;

		title_rect = item_rect;
		title_rect.left = time_rect.right + 5;

		CTime now = CTime::GetCurrentTime();

		p->dc->TextColor = Color::Text2; 
		p->dc->Draw(m_items[p->index].start.Format(L"%H:%M"), time_rect);

		p->dc->TextColor = Color::Text1; 
		if(p->selected) DrawScrollText(p->dc, p->text.c_str(), title_rect);
		else p->dc->Draw(p->text.c_str(), title_rect);

		return true; 
	}

	bool EPGChannelList::OnListScrollOverflow(Control* c, float pos)
	{
		int dir = pos > 0 ? 1 : -1;

		CurrentDate = (CTime)CurrentDate + CTimeSpan(dir, 0, 0, 0);

		if(dir < 0)
		{
			Selected = m_items.size() - 1;
		}
		else
		{
			Selected = 0;
		}

		if(Selected >= 0)
		{
			MakeVisible(Selected);
		}
		
		return true;
	}

	bool EPGChannelList::OnListSelectedChanging(Control* c, int index)
	{
		if(index >= 0 && index < ItemCount && index != Selected)
		{
			g_env->svc.GetProgram(m_items[index].id, m_items[index]);
		}

		return true;
	}
}
