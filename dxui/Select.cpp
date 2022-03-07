#include "stdafx.h"
#include "select.h"

namespace DXUI
{
	Select::Select()
	{
		PaintBeginEvent.Add0(&Select::OnPaintBegin, this);
		PaintForegroundEvent.Add(&Select::OnPaintForeground, this, true);
		KeyEvent.Add(&Select::OnKey, this);
		m_left.ClickedEvent.Add0(&Select::OnLeftClicked, this);
		m_right.ClickedEvent.Add0(&Select::OnRightClicked, this);
	}

	bool Select::OnPaintBegin(Control* c)
	{
		TextColor = FocusedPath ? Color::Text2 : Color::Text1;

		return true;
	}

	bool Select::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Vector4i r = WindowRect->deflate(TextMargin);

		float f = m_anim;

		if(f != 0)
		{
			int selected = Selected;
			int count = m_list.ItemCount;
			int dir = f < 0 ? +1 : -1;

			std::wstring s1 = m_list.GetItemText(selected);
			std::wstring s2 = m_list.GetItemText((selected + count + dir) % count);

			int o1 = (int)(f * r.width() + 0.5f);
			int o2 = o1 + dir * r.width();

			dc->Draw(s1.c_str(), r, Vector2i(o1, 0));
			dc->Draw(s2.c_str(), r, Vector2i(o2, 0));
		}
		else
		{
			dc->Draw(Text->c_str(), r);
		}

		return true;
	}

	bool Select::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.key == VK_LEFT)
			{
				m_left.Click();

				return true;
			}

			if(p.cmd.key == VK_RIGHT)
			{
				m_right.Click();

				return true;
			}
		}

		return false;
	}

	bool Select::OnLeftClicked(Control* c)
	{
		if(m_list.ItemCount > 0)
		{
			Selected = (Selected - 1 + m_list.ItemCount) % m_list.ItemCount;

			if(m_list.ItemCount > 1)
			{
				m_anim.RemoveAll();
				m_anim.Set(-1, 0, 200);
			}

			return true;
		}

		return false;
	}

	bool Select::OnRightClicked(Control* c)
	{
		if(m_list.ItemCount > 0)
		{
			Selected = (Selected + 1) % m_list.ItemCount;

			if(m_list.ItemCount > 1)
			{
				m_anim.RemoveAll();
				m_anim.Set(+1, 0, 200);
			}

			return true;
		}

		return false;
	}
}