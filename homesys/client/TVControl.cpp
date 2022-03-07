#include "stdafx.h"
#include "TVControl.h"
#include "client.h"

namespace DXUI
{
	TVControl::TVControl() 
	{
		CurrentProgram.Set(&TVControl::SetCurrentProgram, this);

		ActivatedEvent.Add(&TVControl::OnActivated, this);
		DeactivatedEvent.Add(&TVControl::OnDeactivated, this);
		PaintForegroundEvent.Add(&TVControl::OnPaintForeground, this, true);
		m_up.ClickedEvent.Chain(g_env->PrevEvent);
		m_down.ClickedEvent.Chain(g_env->NextEvent);
		m_up.ClickedEvent.Add0(&TVControl::OnUpdatePreset, this);
		m_down.ClickedEvent.Add0(&TVControl::OnUpdatePreset, this);
//		m_controls.m_prev.ClickedEvent.Add0(&TVControl::OnUpdatePreset, this);
//		m_controls.m_next.ClickedEvent.Add0(&TVControl::OnUpdatePreset, this);
		m_thread.MessageEvent.Add(&TVControl::OnThreadMessage, this);
		m_thread.TimerEvent.Add(&TVControl::OnThreadTimer, this);
		m_thread.TimerPeriod = 10000;
	}
/*
	bool TVControl::Create(CRect rect, Control* parent)
	{
		if(!__super::Create(rect, parent))
			return false;

		ActivatedEvent += new EventHandler<TVControl>(this, &TVControl::OnUpdatePreset);

		return true;
	}
*/
	void TVControl::SetCurrentProgram(Homesys::Program*& value)
	{
		m_thread.Post(WM_APP);

		m_program.id = 0;

		if(value != NULL && value->id)
		{
			m_program = *value;
		}

		m_desc.CurrentProgram = &m_program;

		value = &m_program;
	}

	bool TVControl::OnActivated(Control* c, int dir)
	{
		m_thread.Create();
		
		return true;
	}

	bool TVControl::OnDeactivated(Control* c, int dir)
	{
		m_thread.Join(false);

		return true;
	}

	bool TVControl::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Vector4i r = C2W(MapClientRect(&m_logo));
		
		dc->Draw(r, 0xffffffff);
		
		Texture* t = m_ctf.GetTexture(g_env->ps.tuner.preset.channelId, dc);

		if(t != NULL)
		{
			dc->Draw(t, r.deflate(1));
		}

		return true;
	}

	bool TVControl::OnUpdatePreset(Control* c)
	{
		m_thread.PostTimer();

		return true;
	}

	bool TVControl::OnThreadMessage(Control* c, const MSG& msg)
	{
		if(msg.message == WM_APP)
		{
			m_thread.TimerPeriod = m_thread.TimerPeriod;

			return true;
		}

		return false;
	}

	bool TVControl::OnThreadTimer(Control* c, UINT64 n)
	{
		CAutoLock cAutoLock(&m_lock);

		m_program.id = 0;

		if(!g_env->players.empty())
		{
			CComPtr<IGenericPlayer> p = g_env->players.front();

			if(CComQIPtr<ITunerPlayer> t = p)
			{
				g_env->svc.GetCurrentProgram(t->GetTunerId(), 0, m_program);
			}
		}

		m_desc.CurrentProgram = &m_program;

		return true;
	}
}
