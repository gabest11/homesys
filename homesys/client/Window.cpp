#include "stdafx.h"
#include "window.h"
#include "client.h"

namespace DXUI
{
	Event<> Window::NavigateBackEvent;
	Event<> Window::ExitEvent;

	Window::Window() 
	{
		PaintBeginEvent.Add0(&Window::OnPaintBegin, this);
		PaintForegroundEvent.Add(&Window::OnPaintForeground, this, true);
		ActivatedEvent.Add(&Window::OnActivated, this);
		DeactivatedEvent.Add(&Window::OnDeactivated, this);
		KeyEvent.Add(&Window::OnKey, this);
		MouseEvent.Add(&Window::OnMouse, this);
		ClosedEvent.Chain(NavigateBackEvent);
		m_banner_yes.ClickedEvent.Add0(&Window::OnBannerYes, this);
		m_banner_no.ClickedEvent.Add0(&Window::OnBannerNo, this);

		m_closekey.insert(VK_ESCAPE);
		m_closekey.insert(VK_BACK);
	}

	void Window::ShowBanner()
	{
		m_banner.ToFront();

		if(!m_banner.Visible)
		{
			m_banner.Visible = true;
			m_banner_anim.Set(1, 0, 500);
			m_banner_anim.SetSegment(1);
		}

		Show();
	}

	void Window::HideBanner()
	{
		if(m_banner.Captured)
		{
			m_banner.Captured = false;
		}

		if(m_banner.Visible)
		{
			m_banner.Visible = false;
		}
	}

	void Window::ToggleBanner()
	{
		if(m_banner.Visible)
		{
			float target = 0;

			if(m_banner_container.Visible)
			{
				m_banner.Captured = false;
				m_banner_container.Visible = false;

				target = 0;
			}
			else
			{
				m_banner.Captured = true;
				m_banner_container.Visible = true;
				m_banner_no.Focused = true;

				target = 1;
			}

			m_banner_anim.Set(m_banner_anim, target, 400);
		}
	}

	bool Window::OnPaintBegin(Control* c)
	{
		const Homesys::Recording& r = g_env->ps.recording;

		if(r.id != GUID_NULL)
		{
			int secs = ((r.start + r.startDelay) - CTime::GetCurrentTime()).GetTotalSeconds();

			if(secs > 0 && secs <= 60)
			{
				std::wstring str;

				if(r.state == Homesys::RecordingState::Scheduled)
				{
					str = Util::Format(Control::GetString("RECORDING_STARTS"), secs, r.start.GetHour(), r.start.GetMinute(), r.name.c_str());
				}
				else if(r.state == Homesys::RecordingState::Warning)
				{
					str = Util::Format(Control::GetString("PROGRAM_STARTS"), r.name.c_str(), secs, r.start.GetHour(), r.start.GetMinute());
				}

				m_banner_text.Text = str;

				if(r.state == Homesys::RecordingState::Scheduled)
				{
					str = L"Leállítja az időzített felvételt?";
				}
				else if(r.state == Homesys::RecordingState::Warning)
				{
					str = L"Figyelmeztetés eltüntetése?";
				}

				m_banner_question.Text = str;

				ShowBanner();
			}
		}
		else
		{
			HideBanner();
		}

		if(m_banner.Visible)
		{
			float f = m_banner_anim;

			int top = 58;
			int bottom = 150;

			Vector4i r = m_banner.Rect;

			r.top = 0;
			r.bottom = (int)(f * (bottom - top) + top);

			m_banner.Move(r);

			m_banner.AnchorRect = Vector4i(Anchor::Left, Anchor::Top, Anchor::Right, Anchor::Top);
		}

		return true;
	}

	bool Window::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		return true;
	}

	bool Window::OnActivated(Control* c, int dir)
	{
		std::wstring str = Text;

		if(!str.empty())
		{
			// TODO: GetVFD()->SetText(str, NULL);
		}

		return true;
	}

	bool Window::OnDeactivated(Control* c, int dir)
	{
		return true;
	}

	bool Window::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(m_closekey.find(p.cmd.key) != m_closekey.end() || p.cmd.app == APPCOMMAND_BROWSER_BACKWARD)
			{
				Close();

				return true;
			}

			if(p.cmd.app == APPCOMMAND_MEDIA_STOP)
			{
				g_env->StopEvent(this);

				return true;
			}

			if(p.cmd.app == APPCOMMAND_MEDIA_PAUSE)
			{
				g_env->PauseEvent(this);

				return true;
			}

			if(p.cmd.app == APPCOMMAND_MEDIA_PLAY)
			{
				g_env->PlayEvent(this);

				return true;
			}

			if(p.cmd.app == APPCOMMAND_MEDIA_PLAY_PAUSE)
			{
				g_env->PlayPauseEvent(this);

				return true;
			}	

			if(p.cmd.app == APPCOMMAND_MEDIA_REWIND)
			{
				g_env->SkipBackwardEvent(this);

				return true;
			}

			if(p.cmd.app == APPCOMMAND_MEDIA_FAST_FORWARD)
			{
				g_env->SkipForwardEvent(this);

				return true;
			}

			if(p.cmd.remote == RemoteKey::Details || p.cmd.key == VK_F12)
			{
				ToggleBanner();

				return true;
			}
		}

		return false;
	}

	bool Window::OnMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::RDown)
		{
			Close();

			return true;
		}

		return false;
	}

	bool Window::OnBannerYes(Control* c)
	{
		if(g_env->ps.recording.id != GUID_NULL)
		{
			if(g_env->svc.DeleteRecording(g_env->ps.recording.id))
			{
				g_env->ps.recording.id = GUID_NULL;

				ToggleBanner();
			}
		}

		return true;
	}

	bool Window::OnBannerNo(Control* c)
	{
		ToggleBanner();

		return true;
	}
}
