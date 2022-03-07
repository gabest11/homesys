#include "stdafx.h"
#include "EPGRow.h"
#include "client.h"

namespace DXUI
{
	EPGRow::EPGRow()
	{
		Position.Get(&EPGRow::GetPosition, this);
		Position.Set(&EPGRow::SetPosition, this);

		ActivatedEvent.Add(&EPGRow::OnActivated, this);
		DeactivatedEvent.Add(&EPGRow::OnDeactivated, this);
		CurrentPreset.ChangedEvent.Add(&EPGRow::OnCurrentPresetChanged, this);
		Start.ChangedEvent.Add(&EPGRow::OnStartChanged, this);
		Duration.ChangedEvent.Add(&EPGRow::OnDurationChanged, this);
		KeyEvent.Add(&EPGRow::OnKey, this);
	}

	EPGRow::~EPGRow()
	{
		Reset();
	}

	void EPGRow::GetPosition(CTime& value)
	{
		EPGRowButton* b = dynamic_cast<EPGRowButton*>(GetFocus());

		__time64_t start = std::max<__time64_t>(Start->GetTime(), b->CurrentProgram->start.GetTime());
		__time64_t stop = std::min<__time64_t>(Start->GetTime() + Duration->GetTimeSpan(), b->CurrentProgram->stop.GetTime());

		value = (start + stop) / 2;
	}

	void EPGRow::SetPosition(CTime& value)
	{
		Focused = true;

		for(auto i = m_cols.begin(); i != m_cols.end(); i++)
		{
			EPGRowButton* b = *i;

			if(b->CurrentProgram->start <= value && value <= b->CurrentProgram->stop)
			{
				b->Focused = true;

				break;
			}
		}
	}

	bool EPGRow::OnActivated(Control* c, int dir)
	{
		Update();

		return true;
	}

	bool EPGRow::OnDeactivated(Control* c, int dir)
	{
		Reset();

		return true;
	}

	bool EPGRow::OnCurrentPresetChanged(Control* c, Homesys::Preset* preset)
	{
		Update();

		return true;
	}

	bool EPGRow::OnStartChanged(Control* c, CTime start)
	{
		Update();

		return true;
	}

	bool EPGRow::OnDurationChanged(Control* c, CTimeSpan duration)
	{
		Update();

		return true;
	}

	bool EPGRow::OnKey(Control* c, const KeyEventParam& p)
	{
		EPGRowButton* b = dynamic_cast<EPGRowButton*>(GetFocus());

		if(b != NULL && p.mods == 0)
		{
			if(p.cmd.key == VK_LEFT)
			{
				if(b == m_cols[0])
				{
					NavigateLeftEvent(this);

					m_cols[0]->Focused = true;

					return true;
				}
			}

			if(p.cmd.key == VK_RIGHT)
			{
				if(b == m_cols.back())
				{
					NavigateRightEvent(this);

					m_cols.back()->Focused = true;

					return true;
				}
			}

			if(p.cmd.key == VK_UP)
			{
				NavigateUpEvent(this);

				return true;
			}

			if(p.cmd.key == VK_DOWN)
			{
				NavigateDownEvent(this);

				return true;
			}
		}

		return false;
	}

	void EPGRow::Reset()
	{
		for(auto i = m_cols.begin(); i != m_cols.end(); i++)
		{
			(*i)->ClickedEvent.Unchain(RowButtonClicked);

			delete *i;
		}

		m_cols.clear();

		m_programs.clear();
	}

	void EPGRow::Update()
	{
		Reset();
		
		g_env->svc.GetPrograms(GUID_NULL, CurrentPreset->channelId, Start, (CTime)Start + (CTimeSpan)Duration, m_programs);

		__time64_t start = Start->GetTime();
		__time64_t duration = Duration->GetTimeSpan();

		if(duration > 0)
		{
			if(m_programs.empty())
			{
				Homesys::Program p;

				p.id = 0;
				p.channelId = CurrentPreset->channelId;
				p.start = start;
				p.stop = start + duration;
				p.state = Homesys::RecordingState::NotScheduled;
				p.episode.id = 0;
				p.episode.movie.id = 0;
				p.episode.movie.title = Control::GetString("NO_GUIDE");

				m_programs.push_back(p);
			}

			for(auto i = m_programs.begin(); i != m_programs.end(); )
			{
				Homesys::Program& p = *i++;

				Vector4i r(0, 0, 0, m_programbox.Height);

				__time64_t left = (p.start - Start).GetTimeSpan();
				__time64_t right = (p.stop - Start).GetTimeSpan();

				if(i == m_programs.end())
				{
					right = start + duration;
				}

				r.left = (long)(std::min<__time64_t>(std::max<__time64_t>(left, 0), duration) * m_programbox.Width / duration);
				r.right = (long)(std::min<__time64_t>(std::max<__time64_t>(right, 0), duration) * m_programbox.Width / duration);

				EPGRowButton* col = new EPGRowButton();

				col->Create(r, &m_programbox);
				col->AnchorRect = Vector4i(Anchor::Percent, Anchor::Top, Anchor::Percent, Anchor::Bottom);
				col->CurrentProgram = &p;
				col->HeaderStart = start;
				col->HeaderStop = start + duration;
				col->Focused.ChangedEvent.Add([&] (Control* c, const bool& b) -> bool {return RowButtonFocused(c, b);});
				col->ClickedEvent.Chain(RowButtonClicked);

				m_cols.push_back(col);
			}
		}

		ColsChanged(this);
	}
}
