#include "stdafx.h"
#include "PlayerControl2.h"
#include "client.h"

namespace DXUI
{
	PlayerControl2::PlayerControl2()
	{
		m_prev.ClickedEvent.Chain(g_env->PrevPlayerEvent);
		m_next.ClickedEvent.Chain(g_env->NextPlayerEvent);
		m_rewind.ClickedEvent.Add0([] (Control* c) -> bool {return g_env->SkipBackwardEvent(c, false);});
		m_forward.ClickedEvent.Add0([] (Control* c) -> bool {return g_env->SkipForwardEvent(c, false);});
		m_stop.ClickedEvent.Chain(g_env->StopEvent);
		m_pause.ClickedEvent.Chain(g_env->PauseEvent);
		m_play.ClickedEvent.Chain(g_env->PlayEvent);
		m_playpause.ClickedEvent.Chain(g_env->PlayPauseEvent);
		m_record.ClickedEvent.Chain(g_env->RecordEvent);
	}
}
