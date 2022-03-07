#include "stdafx.h"
#include "MenuEPG.h"
#include "client.h"

namespace DXUI
{
	MenuEPG::MenuEPG()
	{
		RedEvent.Add0(&MenuEPG::OnRed, this);
		GreenEvent.Add0(&MenuEPG::OnGreen, this);
		BlueEvent.Add0(&MenuEPG::OnBlue, this);
	}

	bool MenuEPG::Create(const Vector4i& rect, Control* parent)
	{
		if(!__super::Create(rect, parent))
			return false;

		// ???

		m_program.CurrentProgram.Chain(&m_epgview.CurrentProgram);
		m_epgview.ProgramClicked.Add(&MenuEPG::OnProgramClicked, this);

		return true;
	}

	bool MenuEPG::OnRed(Control* c)
	{
		EPGRowButton* b = dynamic_cast<EPGRowButton*>(GetFocus());

		if(b == NULL)
		{
			return true;
		}

		Homesys::Program* p = b->CurrentProgram;

		if(p == NULL)
		{
			return true;
		}

		Homesys::Program program;

		if(g_env->svc.GetProgram(p->id, program))
		{
			if(program.state == Homesys::RecordingState::Warning)
			{
				std::list<Homesys::Recording> recordings;
				
				if(g_env->svc.GetRecordings(recordings))
				{
					for(auto i = recordings.begin(); i != recordings.end(); i++)
					{
						const Homesys::Recording& r = *i;

						if(r.program.id == program.id &&  r.state == program.state)
						{
							g_env->svc.DeleteRecording(r.id);
						}
					}
				}
			}
			else
			{
				GUID recordingId;

				g_env->svc.RecordByProgram(program.id, true, recordingId);
			}

			if(g_env->svc.GetProgram(p->id, program))
			{
				p->state = program.state;
			}
		}

		return true;
	}

	bool MenuEPG::OnGreen(Control* c)
	{
		CTime t = CTime::GetCurrentTime();

		t = CTime(t.GetYear(), t.GetMonth(), t.GetDay(), 20, 0, 0);

		m_epgview.Start = t;

		return true;
	}

	bool MenuEPG::OnBlue(Control* c)
	{
		g_env->svc.UpdateGuideNow(true);

		return true;
	}

	bool MenuEPG::OnProgramClicked(Control* c, Homesys::Program* p)
	{
		// TODO

		return true;
	}
}
