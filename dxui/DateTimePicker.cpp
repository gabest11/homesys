#include "stdafx.h"
#include "DateTimePicker.h"
#include "../util/String.h"

namespace DXUI
{
	DateTimePicker::DateTimePicker()
		: m_pos(0)
	{
		PaintForegroundEvent.Add(&DateTimePicker::OnPaintForeground, this, true);
		KeyEvent.Add(&DateTimePicker::OnKey, this);
		MouseEvent.Add(&DateTimePicker::OnMouse, this);

		CurrentDateTime = CTime::GetCurrentTime();
	}

	void DateTimePicker::Adjust(int dir)
	{
		CTimeSpan ts(0);

		switch(m_pos)
		{
		case 0: ts = CTimeSpan(dir, 0, 0, 0); break;
		case 1: ts = CTimeSpan(0, dir, 0, 0); break;
		case 2: ts = CTimeSpan(0, 0, dir, 0); break;
		case 3: ts = CTimeSpan(0, 0, 0, dir); break;
		}

		CurrentDateTime = (CTime)CurrentDateTime + ts;
	}

	bool DateTimePicker::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		CTime t = CurrentDateTime;

		std::wstring str;

		if(Focused)
		{
			if(Captured)
			{
				std::wstring s[4];

				s[0] = t.Format(L"%Y-%m-%d");
				s[1] = t.Format(L"%H");
				s[2] = t.Format(L"%M");
				s[3] = t.Format(L"%S");

				s[m_pos] = Util::Format(L"[{font.color {r: %d; g: %d; b: %d; a: %d;};}]{%s}", 
					(Color::Text2 >> 0) & 0xff,
					(Color::Text2 >> 8) & 0xff,
					(Color::Text2 >> 16) & 0xff,
					(Color::Text2 >> 24) & 0xff,
					s[m_pos].c_str());

				str = s[0] + L" " + s[1] + L":" + s[2] + L":" + s[3];
			}
			else
			{
				std::wstring s = t.Format(L"%Y-%m-%d %H:%M:%S");

				str = Util::Format(L"[{font.color {r: %d; g: %d; b: %d; a: %d;};}]{%s}", 
					(Color::Text2 >> 0) & 0xff,
					(Color::Text2 >> 8) & 0xff,
					(Color::Text2 >> 16) & 0xff,
					(Color::Text2 >> 24) & 0xff,
					s.c_str());
			}
		}
		else
		{
			str = t.Format(L"%Y-%m-%d %H:%M:%S");
		}

		dc->Draw(str.c_str(), WindowRect);

		return true;
	}

	bool DateTimePicker::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(Captured)
			{
				if(p.cmd.key == VK_LEFT || p.cmd.app == APPCOMMAND_BROWSER_BACKWARD)
				{
					m_pos = (m_pos - 1 + 4) % 4;

					return true;
				}

				if(p.cmd.key == VK_RIGHT || p.cmd.app == APPCOMMAND_BROWSER_FORWARD)
				{
					m_pos = (m_pos + 1) % 4;

					return true;
				}

				if(p.cmd.key == VK_UP)
				{
					Adjust(+1);

					return true;
				}

				if(p.cmd.key == VK_DOWN)
				{
					Adjust(-1);

					return true;
				}

				if(p.cmd.key == VK_ESCAPE)
				{
					Captured = false;

					return true;
				}
			}

			if(p.cmd.key == VK_RETURN)
			{
				Captured = !Captured;

				return true;
			}
		}

		return false;
	}

	bool DateTimePicker::OnMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::Dbl && Hovering)
		{
			Captured = !Captured;

			return true;
		}

		if(p.cmd == MouseEventParam::Wheel && Captured)
		{
			Adjust((int)ceil(p.delta));

			return true;
		}

		return false;
	}
}

