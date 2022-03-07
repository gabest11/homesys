#include "stdafx.h"
#include "Numpad.h"
#include "../util/String.h"

namespace DXUI
{
	static const wchar_t* s_lcchars[12] = 
	{
		L"0+=<>€$%&",
		L"1.,-?!@:()[]{}/\\",
		L"2aáäbc",
		L"3deéf",
		L"4ghií",
		L"5jkl",
		L"6mnoóöő",
		L"7pqrsß",
		L"8tuúüűv",
		L"9wxyz",
		L"",
		L" ",
	};

	static const wchar_t* s_ucchars[12] = 
	{
		L"0+=<>€$%&",
		L"1.,-?!@:()[]{}/\\",
		L"2AÁÄBC",
		L"3DEÉF",
		L"4GHIÍ",
		L"5JKL",
		L"6MNOÓÖŐ",
		L"7PQRSß",
		L"8TUÚÜŰV",
		L"9WXYZ",
		L"",
		L" ",
	};

	Numpad::Numpad()
		: m_start(0)
		, m_key(-1)
		, m_pos(0)
		, m_uppercase(false)
	{
		PaintForegroundEvent.Add(&Numpad::OnPaintForeground, this, true);

		m_0.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_1.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_2.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_3.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_4.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_5.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_6.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_7.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_8.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_9.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_x.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
		m_y.ClickedEvent.Add0(&Numpad::OnButtonPressed, this);
	}

	void Numpad::Press(int i)
	{
		if(i >= 0 && i < sizeof(s_lcchars) / sizeof(s_lcchars[0]))
		{
			switch(i)
			{
			case 0: m_0.Focused = true; break;
			case 1: m_1.Focused = true; break;
			case 2: m_2.Focused = true; break;
			case 3: m_3.Focused = true; break;
			case 4: m_4.Focused = true; break;
			case 5: m_5.Focused = true; break;
			case 6: m_6.Focused = true; break;
			case 7: m_7.Focused = true; break;
			case 8: m_8.Focused = true; break;
			case 9: m_9.Focused = true; break;
			case 10: m_x.Focused = true; break;
			case 11: m_y.Focused = true; break;
			}

			if(m_key != i)
			{
				m_start = 0;
				m_pos = 0;
			}

			if(i == 10)
			{
				m_uppercase = !m_uppercase;
			}
			else
			{
				std::wstring str = Text;

				clock_t now = clock();

				if(now - m_start >= 1000)
				{
					m_pos = 0;
				}

				const wchar_t* chars = m_uppercase ? s_ucchars[i] : s_lcchars[i];

				wchar_t c = chars[m_pos]; // TODO

				if(str.empty() || now - m_start >= 1000 || i == 11)
				{
					str += c;
				}
				else
				{
					str.back() = c;
				}

				m_pos = (m_pos + 1) % _tcslen(chars);

				Text = str;

				m_start = now;

				if(Control* hint = HintControl)
				{
					hint->TextEscaped = true;

					std::wstring str = L"[{font {face: 'Courier New'; weight: 'bold';}}]";

					for(int i = 0, j = wcslen(chars); i < j; i++)
					{
						std::wstring s;
						
						s += chars[i];
						
						s = DeviceContext::Escape(s.c_str());

						if(chars[i] == c)
						{
							s = Util::Format(L"[{font {color {r: %d; g: %d; b: %d; a: %d;};}}]{%s}", 
								(Color::Text2 >> 0) & 0xff,
								(Color::Text2 >> 8) & 0xff,
								(Color::Text2 >> 16) & 0xff,
								(Color::Text2 >> 24) & 0xff,
								s.c_str());
						}

						str += s;
					}

					hint->Text = str;
				}
			}

			m_key = i;
		}
	}

	bool Numpad::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		if(Control* c = HintControl)
		{
			c->Visible = clock() - m_start < 1000;
		}

		return true;
	}

	bool Numpad::OnButtonPressed(Control* c)
	{
		if(c == &m_0) Press(0);
		else if(c == &m_1) Press(1);
		else if(c == &m_2) Press(2);
		else if(c == &m_3) Press(3);
		else if(c == &m_4) Press(4);
		else if(c == &m_5) Press(5);
		else if(c == &m_6) Press(6);
		else if(c == &m_7) Press(7);
		else if(c == &m_8) Press(8);
		else if(c == &m_9) Press(9);
		else if(c == &m_x) Press(10);
		else if(c == &m_y) Press(11);

		return true;
	}
}