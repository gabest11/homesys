#include "stdafx.h"
#include "Combo.h"

namespace DXUI
{
	Combo::Combo()
	{
		Selected.ChangedEvent.Add(&Combo::OnSelectedChanged, this);
		KeyEvent.Add(&Combo::OnKey, this);
		MouseEvent.Add(&Combo::OnMouse, this);
		m_dialog.MouseEvent.Add(&Combo::OnListMouse, this);
		m_list.ClickedEvent.Add(&Combo::OnListClicked, this);

		ListRows = 5;
	}

	void Combo::ShowList()
	{
		if(ListRows > 0)
		{
			Vector4i r = ClientRect;

			int w = MinListWidth;

			if(r.width() < w)
			{
				int x = (r.left + r.right) / 2;

				r.left = x - w / 2;
				r.right = x + w;
			}

			r.top = Height + 2;
			r.bottom = r.top + std::max<int>(m_dialog.Height, Height * ListRows);

			m_dialog.Move(r);
			m_dialog.Show();

			m_list.MakeVisible(m_list.Selected, true);
		}
	}

	bool Combo::OnSelectedChanged(Control* c, int index)
	{
		m_list.Selected = index;

		Text = m_list.GetItemText(index);
		
		return true;
	}

	bool Combo::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.key == VK_RETURN)
			{
				ShowList(); 

				return true;
			}
		}

		return false;
	}
	
	bool Combo::OnMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::Down)
		{
			ShowList();

			return true;
		}

		return false;
	}

	bool Combo::OnListMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::Down && !c->Hovering)
		{
			((Dialog*)c)->Hide();

			return true;
		}

		return false;
	}

	bool Combo::OnListClicked(Control* c, int index)
	{
		m_dialog.Hide();

		Selected = index;

		return true;
	}
}