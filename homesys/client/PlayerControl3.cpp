#include "stdafx.h"
#include "PlayerControl3.h"
#include "client.h"

namespace DXUI
{
	PlayerControl3::PlayerControl3()
	{
		m_prev.ClickedEvent.Chain(g_env->PrevPlayerEvent);
		m_next.ClickedEvent.Chain(g_env->NextPlayerEvent);
		m_rewind.ClickedEvent.Add0([] (Control* c) -> bool {return g_env->SkipBackwardEvent(c, false);});
		m_forward.ClickedEvent.Add0([] (Control* c) -> bool {return g_env->SkipForwardEvent(c, false);});
		m_stop.ClickedEvent.Chain(g_env->StopEvent);
		m_pause.ClickedEvent.Chain(g_env->PauseEvent);
		m_play.ClickedEvent.Chain(g_env->PlayEvent);
		m_playpause.ClickedEvent.Chain(g_env->PlayPauseEvent);
		m_return.ClickedEvent.Chain(g_env->ReturnEvent);
		m_record.ClickedEvent.Chain(g_env->RecordEvent);
		m_up.ClickedEvent.Chain(g_env->PrevEvent);
		m_down.ClickedEvent.Chain(g_env->NextEvent);

		MouseEvent.Add0([&] (Control* c) -> bool {if(m_anim.GetSegment() > 2) m_anim.SetSegment(0); return false;});
		Alpha.Get([&] (float& value) {value = m_anim;});
		m_prev.Alpha.Chain(&Alpha);
		m_next.Alpha.Chain(&Alpha);
		m_rewind.Alpha.Chain(&Alpha);
		m_forward.Alpha.Chain(&Alpha);
		m_play.Alpha.Chain(&Alpha);
		m_pause.Alpha.Get([&] (float& value) {value = g_env->ps.state == State_Paused ? 1.0f : m_anim;});
		m_playpause.Alpha.Chain(&Alpha);
		m_stop.Alpha.Get([&] (float& value) {value = g_env->ps.state == State_Stopped ? 1.0f : m_anim;});
		m_record.Alpha.Get([&] (float& value) {value = g_env->ps.tuner.recording ? 1.0f : m_anim;});
		m_anim.Set(0.3f, 1.0f, 300);
		m_anim.Add(1.0f, 10000);
		m_anim.Add(0.3f, 300);
	}
}
