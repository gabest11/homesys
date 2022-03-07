#include "stdafx.h"
#include "Calendar.h"
#include "Localizer.h"
#include "../util/String.h"

namespace DXUI
{
	Calendar::Calendar()
		: m_pos(0, 0)
	{
		for(int i = 0; i < 6; i++)
		{
			m_grid[i] = new Day[7];
		}

		CurrentDate.Get([&] (CTime& value) {value = m_grid[m_pos.y][m_pos.x].t;});
		CurrentDate.Set(&Calendar::SetCurrentDate, this);

		KeyEvent.Add(&Calendar::OnKey, this);
		MouseEvent.Add(&Calendar::OnMouse, this);
		PaintForegroundEvent.Add(&Calendar::OnPaintForeground, this, true);

		CanFocus = 1;
		CurrentDate = CTime::GetCurrentTime();
	}

	Calendar::~Calendar()
	{
		for(int i = 0; i < 6; i++)
		{
			delete [] m_grid[i];
		}
	}

	void Calendar::SetCurrentDate(CTime& t)
	{
		for(int j = 0; j < 6; j++)
		{
			for(int i = 0; i < 7; i++)
			{
				m_grid[j][i].t = CTime();
				m_grid[j][i].r = Vector4i::zero();
			}
		}

		int year = t.GetYear();
		int month = t.GetMonth();
		int day = t.GetDay();

		CTime t2(year, month, 1, 12, 0, 0);

		int i = (t2.GetDayOfWeek() + 5) % 7;
		int j = 0;

		do
		{
			m_grid[j][i].t = t2;

			if(t2.GetDay() == day)
			{
				m_pos.x = i;
				m_pos.y = j;
			}

			if(++i == 7) {j++; i = 0;}

			t2 += CTimeSpan(1, 0, 0, 0);
		}
		while(t2.GetDay() > 1);
	}

	bool Calendar::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			const CTime& t = m_grid[m_pos.y][m_pos.x].t;

			if(p.cmd.key == VK_LEFT)
			{
				CTime t2 =  t - CTimeSpan(1, 0, 0, 0);

				if(t2.GetMonth() == t.GetMonth())
				{
					if(m_pos.x > 0)
					{
						m_pos.x--;
					}
					else
					{
						m_pos.x = 6;
						m_pos.y--;
					}
				}
				else
				{
					CurrentDate = t2;
				}

				SelectedEvent(this, m_grid[m_pos.y][m_pos.x].t);

				return true;
			}
			else if(p.cmd.key == VK_RIGHT)
			{
				CTime t2 = t + CTimeSpan(1, 0, 0, 0);

				if(t2.GetMonth() == t.GetMonth())
				{
					if(m_pos.x < 6)
					{
						m_pos.x++;
					}
					else
					{
						m_pos.x = 0;
						m_pos.y++;
					}
				}
				else
				{
					CurrentDate = t2;
				}

				SelectedEvent(this, m_grid[m_pos.y][m_pos.x].t);

				return true;
			}
			else if(p.cmd.key == VK_UP)
			{
				if(m_pos.y > 0 && m_grid[m_pos.y - 1][m_pos.x].t.GetTime() != 0) 
				{
					m_pos.y--;

					SelectedEvent(this, m_grid[m_pos.y][m_pos.x].t);

					return true;
				}
			}
			else if(p.cmd.key == VK_DOWN)
			{
				if(m_pos.y < 5 && m_grid[m_pos.y + 1][m_pos.x].t.GetTime() != 0) 
				{
					m_pos.y++;

					SelectedEvent(this, m_grid[m_pos.y][m_pos.x].t);

					return true;
				}
			}
			else if(p.cmd.key == VK_RETURN)
			{
				if(m_grid[m_pos.y][m_pos.x].t.GetTime() != 0) 
				{
					ClickedEvent(this, m_grid[m_pos.y][m_pos.x].t);
				}

				return true;
			}
		}

		return false;
	}
	
	bool Calendar::OnMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::Down || p.cmd == MouseEventParam::Dbl)
		{
			Vector2i point = C2W(p.point);

			for(int j = 0; j < 6; j++)
			{
				for(int i = 0; i < 7; i++)
				{
					const CTime& t = m_grid[j][i].t;

					if(t.GetTime() == 0) continue;

					if(m_grid[j][i].r.contains(point))
					{
						m_pos.x = i;
						m_pos.y = j;

						if(p.cmd == MouseEventParam::Down)
						{
							SelectedEvent(this, m_grid[m_pos.y][m_pos.x].t);
						}
						else
						{
							ClickedEvent(this, m_grid[m_pos.y][m_pos.x].t);
						}

						return true;
					}
				}
			}
		}

		return false;
	}

	bool Calendar::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Texture* hdr = dc->GetTexture(L"ListBackground.png");

		Vector4i wr = WindowRect;

		int w = wr.width() / 7;
		int h = wr.height() / 7;

		dc->TextAlign = Align::Center | Align::Middle;
		dc->TextHeight = h * 2 / 5;
		dc->TextFace = FontType::Bold;

		Vector4i r = wr;

		r.bottom = wr.bottom - h / 2 - h * 6;

		std::vector<std::wstring> days;

		days.push_back(Control::GetString("DAY_MON"));
		days.push_back(Control::GetString("DAY_TUE"));
		days.push_back(Control::GetString("DAY_WEN"));
		days.push_back(Control::GetString("DAY_THU"));
		days.push_back(Control::GetString("DAY_FRI"));
		days.push_back(Control::GetString("DAY_SAT"));
		days.push_back(Control::GetString("DAY_SUN"));

		Vector4i r2 = r.deflate(5);

		dc->Draw9x9(hdr, r2, Vector2i(8, 8));

		for(int i = 0; i < 7; i++)
		{
			dc->Draw(days[i].c_str(), Vector4i(r.left + i * w, r.top, i < 6 ? r.left + (i + 1) * w : r.right, r.bottom));
		}

		Texture* bkg = dc->GetTexture(L"ImageButtonBackground.png");
		Texture* bkgsel = dc->GetTexture(L"ImageButtonFocused.png");

		dc->TextHeight = h * 3 / 5;
		dc->TextFace = FontType::Normal;

		r.top = r.bottom;
		r.bottom = wr.bottom - h / 2;

		CTime today = CTime::GetCurrentTime();

		int year = today.GetYear();
		int month = today.GetMonth();
		int day = today.GetDay();

		for(int j = 0; j < 6; j++)
		{
			for(int i = 0; i < 7; i++)
			{
				const CTime& t = m_grid[j][i].t;

				if(t.GetTime() == 0) continue;

				r2 = Vector4i(r.left + i * w, r.top, i < 6 ? r.left + (i + 1) * w : r.right, r.top + h).deflate(5);

				m_grid[j][i].r = r2;

				dc->TextStyle = t.GetYear() == year && t.GetMonth() == month && t.GetDay() == day ? TextStyles::Underline : 0;

				dc->Draw9x9(i == m_pos.x && j == m_pos.y ? bkgsel : bkg, r2, Vector2i(14, 0));
				dc->Draw(t.Format(L"%#d"), r2);
			}

			r.top += h;
		}

		r.top = r.bottom;
		r.bottom = wr.bottom;

		dc->TextHeight = h * 2 / 5;
		dc->TextFace = FontType::Bold;

		r2 = r.deflate(5);

		dc->Draw9x9(hdr, r2, Vector2i(8, 8));
		dc->Draw(CurrentDate->Format(L"%Y / %m"), r2);

		return true;
	}
	
	CalendarHour::CalendarHour()
	{
		for(int i = 6; i < 17; i++)
		{
			m_items.push_back(i);
		}

		ItemCount.Get([&] (int& value) 
		{
			value = m_items.size();
		});

		ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool 
		{
			p->text = Util::Format(L"%d:00", p->index + 6);

			return true;
		});

		ClickedEvent.Add([&] (Control* c, int index) -> bool
		{
			return HourClickedEvent(c, index + 6);
		});
	}
}