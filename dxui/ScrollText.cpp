#include "stdafx.h"
#include "Scrolltext.h"

namespace DXUI
{
	ScrollText::ScrollText() 
	{
		ActivatedEvent.Add(&ScrollText::OnActivated, this);
		PaintForegroundEvent.Add(&ScrollText::OnPaintForeground, this, true);
		KeyEvent.Add(&ScrollText::OnKey, this);

		CanFocus = 1;
		TabTop = false;
		TabBottom = false;
	}

	bool ScrollText::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_slider.Position = 0;
		}

		return true;
	}

	bool ScrollText::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		std::wstring s = Text;

		bool slider = false;

		Vector4i r = WindowRect->deflate(TextMargin);

		Vector2i offset(0, 0);

		TextEntry* te = dc->Draw(s.c_str(), r, offset, true);

		if(te != NULL)
		{
			int h1 = r.height();
			int h2 = te->bbox.bottom;

			if(h1 < h2)
			{
				slider = true;
				
				r.right = std::min<int>(r.right, m_slider.WindowRect->left - 2);
				
				if(te = dc->Draw(s.c_str(), r, offset, true))
				{
					h2 = te->bbox.bottom;

					m_slider.Step = (float)TextHeight / h2;

					offset.y = -(int)(m_slider.Position * (h2 - h1));
				}
			}

			dc->Draw(s.c_str(), r, offset);
		}
		
		if(m_slider.Visible != slider)
		{
			m_slider.Visible = slider;
		}

		return true;
	}

	bool ScrollText::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.key == VK_UP)
			{
				if(m_slider.Position > 0)
				{
					m_slider.m_upleft.Click();

					return true;
				}
			}

			if(p.cmd.key == VK_DOWN)
			{
				if(m_slider.Position < 1)
				{
					m_slider.m_downright.Click();

					return true;
				}
			}
		}

		return false;
	}
}
