#include "stdafx.h"
#include "MenuRecordingModify.h"
#include "client.h"

namespace DXUI
{
	MenuRecordingModify::MenuRecordingModify()
	{
		ActivatedEvent.Add(&MenuRecordingModify::OnActivated, this);
		DeactivatedEvent.Add(&MenuRecordingModify::OnDeactivated, this);
		m_next.ClickedEvent.Add0(&MenuRecordingModify::OnNext, this);
		RecordingId.ChangedEvent.Add(&MenuRecordingModify::OnRecordingIdChanged, this);
		m_start.CurrentDateTime.ChangedEvent.Add(&MenuRecordingModify::OnCurrentDateTimeChanged, this);
		m_presetselect.Selected.ChangedEvent.Add(&MenuRecordingModify::OnPresetChanged, this);
		m_thread.TimerEvent.Add(&MenuRecordingModify::OnThreadTimer, this);
		m_thread.TimerPeriod = 500;

		static const int delay[] = {0, 1, 2, 3, 5, 10, 15, 20, 25, 30, 45, 60, 90, 120, 180, 240};

		for(int i = 0; i < countof(delay); i++)
		{
			m_delay.push_back(delay[i]);
		}

		auto f = [&] (Control* c, ItemTextParam* p) -> bool
		{
			if(delay > 0) p->text = Util::Format(L"%d %s", m_delay[p->index], Control::GetString("MINUTES")); 
			else p->text = Control::GetString("DELAY_NONE");
			return true;		
		};

		m_startDelay.m_list.ItemCount.Get([&] (int& count) {count = m_delay.size();});
		m_startDelay.m_list.ItemTextEvent.Add(f);
		m_stopDelay.m_list.ItemCount.Get([&] (int& count) {count = m_delay.size();});
		m_stopDelay.m_list.ItemTextEvent.Add(f);
		m_presetselect.m_list.ItemCount.Get([&] (int& count) {count = m_presets.size();});
		m_presetselect.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_presets[p->index].name; return true;});
	}

	void MenuRecordingModify::UpdateProgram()
	{
		m_program = Homesys::Program();
	}

	bool MenuRecordingModify::OnActivated(Control* c, int dir)
	{
		m_thread.Create();

		if(dir >= 0)
		{
			RecordingId = RecordingId;
		}

		return true;
	}

	bool MenuRecordingModify::OnDeactivated(Control* c, int dir)
	{
		m_thread.Join(false);

		m_program = Homesys::Program();

		if(dir <= 0)
		{
			RecordingId = GUID_NULL;
		}

		return true;
	}

	bool MenuRecordingModify::OnNext(Control* c)
	{
		Homesys::Recording r;

		r.id = RecordingId;
		r.name = m_name.Text;
		r.start = m_start.CurrentDateTime;
		r.stop = m_stop.CurrentDateTime;
		r.startDelay = -60 * m_delay[m_startDelay.Selected];
		r.stopDelay = +60 * m_delay[m_stopDelay.Selected];

		if(m_presetselect.Visible && m_presetselect.Selected >= 0)
		{
			r.preset.id = m_presets[m_presetselect.Selected].id;

			if(r.id == GUID_NULL)
			{
				g_env->svc.RecordByPreset(r.preset.id, r.start, r.stop, r.id);
			}
		}

		g_env->svc.UpdateRecording(r);

		Close();

		return true;
	}

	bool MenuRecordingModify::OnRecordingIdChanged(Control* c, GUID recordingId)
	{
		m_presets.clear();

		std::list<Homesys::TunerReg> trs;

		if(g_env->svc.GetTuners(trs))
		{
			for(auto i = trs.begin(); i != trs.end(); i++)
			{
				std::list<Homesys::Preset> presets;

				if(g_env->svc.GetPresets(i->id, true, presets))
				{
					m_presets.insert(m_presets.end(), presets.begin(), presets.end());
				}
			}
		}

		Homesys::Recording r;

		r.id = recordingId;

		if(r.id == GUID_NULL)
		{
			m_name.Text = Control::GetString("NEW_RECORDING");

			CTime start = CTime::GetCurrentTime() + CTimeSpan(0, 0, 59, 59);
			CTime stop = start + CTimeSpan(0, 2, 0, 0);

			r.start = CTime(start.GetYear(), start.GetMonth(), start.GetDay(), start.GetHour(), 0, 0);
			r.stop = CTime(stop.GetYear(), stop.GetMonth(), stop.GetDay(), stop.GetHour(), 0, 0);

			r.startDelay = -60;
			r.stopDelay = +300;

			if(!g_env->players.empty())
			{
				CComPtr<IGenericPlayer> p = g_env->players.front();

				if(CComQIPtr<ITunerPlayer> t = p)
				{
					g_env->svc.GetCurrentPreset(t->GetTunerId(), r.preset);
				}
			}
		}
		else
		{
			g_env->svc.GetRecording(recordingId, r);

			m_name.Text = r.name;
		}

		m_start.CurrentDateTime = r.start;
		m_stop.CurrentDateTime = r.stop;

		m_startDelay.Selected = 0;
		m_stopDelay.Selected = 0;

		for(int i = m_delay.size() - 1; i >= 0; i--)
		{
			if(-(int)r.startDelay.GetTotalMinutes() <= m_delay[i])
			{
				m_startDelay.Selected = i;
			}

			if((int)r.stopDelay.GetTotalMinutes() <= m_delay[i])
			{
				m_stopDelay.Selected = i;
			}
		}

		m_presetselect.Visible = false;
		m_presetselect.Selected = -1;

		m_channelname.Visible = false;
		m_channelname.Text = _T("");

		if(r.preset.id != 0)
		{
			m_presetselect.Visible = true;

			for(int i = 0; i < m_presets.size(); i++)
			{
				if(m_presets[i].id == r.preset.id)
				{
					m_presetselect.Selected = i;

					break;
				}
			}
		}
		else if(r.program.id != 0)
		{
			m_channelname.Visible = true;

			Homesys::Channel channel;

			if(g_env->svc.GetChannel(r.program.channelId, channel))
			{
				m_channelname.Text = channel.name;
			}
		}

		return true;
	}

	bool MenuRecordingModify::OnThreadTimer(Control* c, UINT64 n)
	{
		int channelId = 0;

		CTime start;

		{
			CAutoLock cAutoLock(&m_lock);

			if(m_program.id != 0)
			{
				return true;
			}

			if(m_presetselect.Visible && m_presetselect.Selected >= 0)
			{
				channelId = m_presets[m_presetselect.Selected].channelId;
			}

			start = m_start.CurrentDateTime;
		}

		std::list<Homesys::Program> programs;

		if(channelId != 0)
		{
			g_env->svc.GetPrograms(GUID_NULL, channelId, start, start, programs);
		}

		CAutoLock cAutoLock(&m_lock);

		if(programs.empty())
		{
			__super::m_program.CurrentProgram = NULL;
		}
		else
		{
			m_program = programs.back();

			__super::m_program.CurrentProgram = &m_program;
		}

		return true;
	}

	bool MenuRecordingModify::OnCurrentDateTimeChanged(Control* c, CTime t)
	{
		UpdateProgram();

		return true;
	}

	bool MenuRecordingModify::OnPresetChanged(Control* c, int index)
	{
		UpdateProgram();

		return true;
	}
}