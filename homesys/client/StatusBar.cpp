#include "stdafx.h"
#include "StatusBar.h"
#include "client.h"

namespace DXUI
{
	class MessageQueue
	{
	public:
		std::wstring message;
		Animation anim;
		int width;
	};

	static MessageQueue s_mq;

	StatusBar::StatusBar()
	{
		ActivatedEvent.Add(&StatusBar::OnActivated, this);
		DeactivatedEvent.Add(&StatusBar::OnDeactivated, this);
		PaintForegroundEvent.Add(&StatusBar::OnPaintForeground, this, true);
		m_left.PaintForegroundEvent.Add(&StatusBar::OnPaintForegroundLeft, this, true);
	}

	bool StatusBar::OnActivated(Control* c, int dir)
	{
		return true;
	}

	bool StatusBar::OnDeactivated(Control* c, int dir)
	{
		return true;
	}

	bool StatusBar::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		std::wstring left;
		std::wstring right;
		std::wstring middle;

		CTime t = CTime::GetCurrentTime();
		
		setlocale(LC_TIME, ".ACP");
		left = t.Format(L"%Y. %B %#d. %A");
		setlocale(LC_TIME, "English");

		if(g_env->ss.connected)
		{
			right = g_env->ss.online ? L"Online" : L"Offline";

			if(!g_env->ss.machine.username.empty()) 
			{
				right += L" (" + g_env->ss.machine.username + L")";
			}
		}
		else
		{
			right = Control::GetString("SERVICE_OFFLINE");
		}

		middle = Util::Format(L"%02d:%02d", t.GetHour(), t.GetMinute());

		//

		m_left.Text = left;
		m_right.Text = right;
		m_clock.Text = middle;
		m_clock.TextColor = g_env->parental.enabled ? Color::Text2 : m_clock.TextColor;

		return true;
	}

	bool StatusBar::OnPaintForegroundLeft(Control* c, DeviceContext* dc)
	{
		if(!g_env->ss.messages.empty())
		{
			s_mq.message = g_env->ss.messages.front().message;

			g_env->ss.messages.pop();

			Vector4i bbox = Vector4i::zero();

			TextEntry* te = dc->Draw(s_mq.message.c_str(), Vector4i(0, 0, INT_MAX / 2, c->Height), true);

			if(te != NULL)
			{
				bbox = te->bbox;
			}

			s_mq.width = bbox.width();

			Vector4i r = c->ClientRect->deflate(c->TextMargin);

			int diff = s_mq.width - r.width();

			clock_t hscroll = diff > 0 ? 1000 * diff / 30 : 0;

			float f = s_mq.anim;

			int segment = s_mq.anim.GetSegment();

			int count = s_mq.anim.GetCount();
			
			if(count > 0)
			{
				s_mq.anim.SetSegmentDuration(3, hscroll);
				s_mq.anim.SetSegmentDuration(5, hscroll);

				if(segment >= 2 && segment < count - 2)
				{
					s_mq.anim.SetSegment(1);
				}
				else if(segment >= count - 2 && segment < count)
				{
					s_mq.anim.SetSegment(0, (clock_t)(f * 500));
				}
				else if(segment >= count)
				{
					s_mq.anim.SetSegment(0);
				}
			}
			else
			{
				s_mq.anim.Add(1, 500);
				s_mq.anim.Add(0, 0);
				s_mq.anim.Add(0, 3000);
				s_mq.anim.Add(1, hscroll);
				s_mq.anim.Add(1, 2000);
				s_mq.anim.Add(0, hscroll);
				s_mq.anim.Add(0, 5000);
				s_mq.anim.Add(1, 0);
				s_mq.anim.Add(0, 500);
			}
		}

		Vector4i r = c->WindowRect->deflate(c->TextMargin);

		float f = s_mq.anim;

		int segment = s_mq.anim.GetSegment();
		int count = s_mq.anim.GetCount();

		if(count >= 0 && segment < 2 || segment >= count - 2 && segment < count)
		{
			dc->Draw(c->Text->c_str(), r, Vector2i(0, (int)(f * r.height())));
			dc->Draw(s_mq.message.c_str(), r, Vector2i(0, (int)((f - 1) * r.height())));
		}
		else if(segment >= 2 && segment < count - 2)
		{
			int diff = s_mq.width - r.width();

			dc->Draw(s_mq.message.c_str(), r, Vector2i(diff > 0 ? (int)(f * diff) : 0, 0));
		}
		else
		{
			dc->Draw(c->Text->c_str(), r);
		}

		return true;
	}
}