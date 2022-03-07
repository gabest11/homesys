#include "stdafx.h"
#include "menuedittext.h"

namespace DXUI
{
	MenuEditText::MenuEditText() 
	{
		ActivatedEvent.Add(&MenuEditText::OnActivated, this);
		KeyEvent.Add(&MenuEditText::OnKey, this);
		Target.ChangedEvent.Add(&MenuEditText::OnTargetChanged, this);
		m_next.ClickedEvent.Add0(&MenuEditText::OnNextClicked, this);
		m_numpad.Text.ChangedEvent.Add(&MenuEditText::OnTextChanged, this);
		m_input.HideCursor = false;
		m_input.ReadOnly = true;

		m_closekey.erase(VK_ESCAPE);
	}

	bool MenuEditText::OnActivated(Control* c, int dir)
	{
		m_numpad.m_1.Focused = true;
		m_hint.Visible = false;

		return true;
	}

	bool MenuEditText::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.cmd.chr)
		{
			switch(p.cmd.chr)
			{
			case 246: 
			case '0': m_numpad.Press(0); return true;
			case '1': m_numpad.Press(1); return true;
			case '2': m_numpad.Press(2); return true;
			case '3': m_numpad.Press(3); return true;
			case '4': m_numpad.Press(4); return true;
			case '5': m_numpad.Press(5); return true;
			case '6': m_numpad.Press(6); return true;
			case '7': m_numpad.Press(7); return true;
			case '8': m_numpad.Press(8); return true;
			case '9': m_numpad.Press(9); return true;
			case '*':
			case 40: m_numpad.Press(10); return true;
			case '#': 
			case 43: m_numpad.Press(11); return true;
			}
		}
		else if(p.mods == 0)
		{
			if(p.cmd.key == VK_ESCAPE)
			{
				std::wstring str = m_numpad.Text;

				if(!str.empty())
				{
					str = str.substr(0, str.size() - 1);
				}

				m_numpad.Text = str;

				return true;
			}

			// TODO: lame

			static std::map<Control*, Control*> right;

			if(right.empty())
			{
				right[&m_numpad.m_3] = &m_numpad.m_1;
				right[&m_numpad.m_6] = &m_numpad.m_4;
				right[&m_numpad.m_9] = &m_numpad.m_7;
				right[&m_numpad.m_y] = &m_numpad.m_x;
			}

			static std::map<Control*, Control*> left;

			if(left.empty())
			{
				left[&m_numpad.m_1] = &m_numpad.m_3;
				left[&m_numpad.m_4] = &m_numpad.m_6;
				left[&m_numpad.m_7] = &m_numpad.m_9;
				left[&m_numpad.m_x] = &m_numpad.m_y;
			}

			static std::map<Control*, Control*> up;

			if(up.empty())
			{
				up[&m_numpad.m_1] = &m_next;
				up[&m_numpad.m_2] = &m_next;
				up[&m_numpad.m_3] = &m_next;
				up[&m_numpad.m_4] = &m_numpad.m_1;
				up[&m_numpad.m_5] = &m_numpad.m_2;
				up[&m_numpad.m_6] = &m_numpad.m_3;
				up[&m_numpad.m_7] = &m_numpad.m_4;
				up[&m_numpad.m_8] = &m_numpad.m_5;
				up[&m_numpad.m_9] = &m_numpad.m_6;
				up[&m_numpad.m_x] = &m_numpad.m_7;
				up[&m_numpad.m_0] = &m_numpad.m_8;
				up[&m_numpad.m_y] = &m_numpad.m_9;
			}

			static std::map<Control*, Control*> down;

			if(down.empty())
			{
				down[&m_numpad.m_1] = &m_numpad.m_4;
				down[&m_numpad.m_2] = &m_numpad.m_5;
				down[&m_numpad.m_3] = &m_numpad.m_6;
				down[&m_numpad.m_4] = &m_numpad.m_7;
				down[&m_numpad.m_5] = &m_numpad.m_8;
				down[&m_numpad.m_6] = &m_numpad.m_9;
				down[&m_numpad.m_7] = &m_numpad.m_x;
				down[&m_numpad.m_8] = &m_numpad.m_0;
				down[&m_numpad.m_9] = &m_numpad.m_y;
				down[&m_numpad.m_x] = &m_next;
				down[&m_numpad.m_0] = &m_next;
				down[&m_numpad.m_y] = &m_next;
			}

			std::map<Control*, Control*>* m = NULL;

			switch(p.cmd.key)
			{
			case VK_RIGHT: m = &right; break;
			case VK_LEFT: m = &left; break;
			case VK_UP: m = &up; break;
			case VK_DOWN: m = &down; break;
			}

			if(m != NULL)
			{
				Control* f = GetFocus();

				auto i = m->find(f);

				if(i != m->end())
				{
					i->second->Focused = true;

					return true;
				}
			}
		}

		return false;
	}

	bool MenuEditText::OnTargetChanged(Control* c, Control* t)
	{
		m_numpad.Text = t->Text;

		return true;
	}

	bool MenuEditText::OnNextClicked(Control* c)
	{
		Target->Text = m_input.Text;

		if(Edit* e = dynamic_cast<Edit*>((Control*)Target))
		{
			e->CursorPos = e->Text->size();
			e->Focused = true;
			e->TextInputEvent(e);
		}

		Close();

		return true;
	}
	
	bool MenuEditText::OnTextChanged(Control* c, std::wstring text)
	{
		m_input.Text = text;
		m_input.CursorPos = text.size();

		return true;
	}
}