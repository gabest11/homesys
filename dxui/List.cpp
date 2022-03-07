#include "stdafx.h"
#include "List.h"

namespace DXUI
{
	List::List() 
		: m_pos(0)
	{
		Selected.Set([&] (int& value) {value = std::min<int>(std::max<int>(value, 0), ItemCount - 1); MakeVisible(value);});
		ItemHeight.Set([&] (int& value) {TextHeight = value; Selected = 0;});
		ItemRows.Get([&] (float& value) {value = (float)m_content.Height / ItemHeight;});

		PaintItemEvent.Add(&List::OnPaintItem, this, true);
		KeyEvent.Add(&List::OnKey, this);
		MouseEvent.Add(&List::OnMouse, this);
		FocusSetEvent.Add0(&List::OnFocusSet, this);
		ClickedEvent.Add([] (Control* c, int i) -> bool {return UserActionEvent(c);});
		m_content.PaintForegroundEvent.Add(&List::OnPaintForeground, this, true);
		m_slider.Position.ChangedEvent.Add(&List::OnSliderPositionChanged, this);
		m_slider.m_upleft.ClickedEvent.Unchain(UserActionEvent);
		m_slider.m_downright.ClickedEvent.Unchain(UserActionEvent);

		ItemHeight = 30;
		Selected = -1;

		m_content.TextAlign.Chain(&TextAlign);
		m_content.TextFace.Chain(&TextFace);
		m_content.TextHeight.Chain(&TextHeight);
		m_content.TextColor.Chain(&TextColor);
		m_content.TextStyle.Chain(&TextStyle);
	}

	void List::MakeVisible(int i, bool first)
	{
		if(ItemCount > ItemRows)
		{
			if(first)
			{
				m_pos = (float)i;
			}
			else
			{
				Vector4i r = GetItemRect(i);

				if(r.bottom > m_content.Height)
				{
					m_pos = (float)i - ItemRows + 1;
				}

				if(r.top < 0)
				{
					m_pos = (float)i;
				}
			}
		}
	}

	bool List::OnSliderPositionChanged(Control* c, float pos)
	{
		if(ItemCount > ItemRows)
		{
			m_pos = pos * (ItemCount - ItemRows);
		}
		else
		{
			m_pos = 0;
		}

		return true;
	}

	bool List::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Vector4i wr = c->WindowRect;

		Vector4i r = wr;

		//r.right -= m_slider.Width - 5;

		Vector4i rsel = r;

		dc->SetScissor(&rsel);

		int index = (int)m_pos;

		r = c->C2W(GetItemRect(index));

		for(int i = index, j = (int)std::min<int>(ItemRows + index + 1 + 1, ItemCount); i < j; i++)
		{
			if(Selected != i)
			{
				PaintItemParam p;
				
				p.dc = dc;
				p.index = i;
				p.rect = r.deflate(TextMargin);
				p.selected = false;
				p.zoomed = false;
				p.text = GetItemText(i);
				
				dc->TextColor = TextColor;
				// TextUnderline = p.selected && FocusedPath;
				// dc->TextStyle = TextStyle;

				PaintItemEvent(this, &p);
			}

			r += Vector4i(0, ItemHeight).xyxy();
		}

		index = (int)m_pos;

		r = c->C2W(GetItemRect(index));

		for(int i = index, j = (int)min(ItemRows + index + 1 + 1, ItemCount); i < j; i++)
		{
			if(Selected == i)
			{
				PaintItemParam p;
				
				p.dc = dc;
				p.index = i;
				p.rect = r.deflate(TextMargin);
				p.selected = true;
				p.zoomed = false;
				p.text = GetItemText(i);
				
				dc->TextColor = SelectedTextColor;
				// TextUnderline = p.selected && FocusedPath;
				// dc->TextStyle = TextStyle;

				if(p.rect.top < wr.bottom && p.rect.bottom > wr.top && FocusedPath)
				{
					float alpha = 1;

					if(p.rect.top < wr.top)
					{
						alpha = std::max<float>(0, 1.0f - (float)(wr.top - p.rect.top) / p.rect.height());
					}

					if(p.rect.bottom > wr.bottom)
					{
						alpha = std::max<float>(0, 1.0f - (float)(p.rect.bottom - wr.bottom) / p.rect.height());
					}

					dc->SetScissor(NULL);

					Vector4i margin = SelectedBackgroundMargin;

					Texture* t = dc->GetTexture(SelectedBackgroundImage->c_str());

					if(t != NULL)
					{
						Vector4i r = p.rect;
						
						r.left = rsel.left;
						r = r.inflate(margin);

						dc->Draw9x9(t, r, margin, alpha);
					}

					dc->SetScissor(&rsel);
				}

				PaintItemEvent(this, &p);

				break;
			}

			r += Vector4i(0, ItemHeight).xyxy();
		}

		dc->SetScissor(NULL);

		m_slider.Position = ItemCount > ItemRows ? m_pos / (ItemCount - ItemRows) : 0;
		m_slider.Step = ItemCount > ItemRows ? 1.0f / (ItemCount - ItemRows) : 0;

		return true;
	}

	bool List::OnPaintItem(Control* c, PaintItemParam* p)
	{
		// if(p->selected) p->dc->Draw(p->rect, 0x40000000);

		p->dc->Draw(p->text.c_str(), p->rect);

		return true;
	}

	bool List::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.key == VK_RETURN && Selected >= 0 && Selected < ItemCount)
			{
				ClickedEvent(this, Selected);

				return true;
			}
			else if(p.cmd.key == VK_UP)
			{
				if(ItemCount > 0 && Selected > 0)
				{
					Selected = std::max<int>(Selected - 1, 0);

					return true;
				}
				else
				{
					OverflowEvent(this, -1);

					return false;
				}
			}
			else if(p.cmd.key == VK_DOWN)
			{
				if(ItemCount > 0 && Selected < ItemCount - 1)
				{
					Selected = Selected + 1;

					return true;
				}
				else
				{
					OverflowEvent(this, +1);

					return false;
				}
			}
			else if(p.cmd.key == VK_PRIOR)
			{
				Selected = (int)std::max<int>(Selected - ItemRows, 0);

				return true;
			}
			else if(p.cmd.key == VK_NEXT)
			{
				Selected = (int)std::min<int>(Selected + ItemRows, ItemCount - 1);

				return true;
			}
			else if(p.cmd.key == VK_HOME)
			{
				Selected = 0;

				return true;
			}
			else if(p.cmd.key == VK_END)
			{
				Selected = ItemCount - 1;

				return true;
			}
		}

		return false;
	}

	bool List::OnMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::Down && Hovering)
		{
			int i = HitTest(p.point);
			if(i >= 0) Selected = i;
			return true;
		}
		else if(p.cmd == MouseEventParam::Dbl && Hovering)
		{
			int i = HitTest(p.point);
			if(i >= 0) {Selected = i; ClickedEvent(this, i);}
			return true;
		}
		else if(p.cmd == MouseEventParam::Wheel && Focused)
		{
			if(p.delta > 0) m_slider.m_upleft.Click();
			else if(p.delta < 0) m_slider.m_downright.Click();
			return true;
		}

		return false;
	}

	bool List::OnFocusSet(Control* c)
	{
		if((Selected < 0 || Selected >= ItemCount) && ItemCount > 0)
		{
			Selected = 0;
		}

		return true;
	}

	Vector4i List::GetItemRect(int i)
	{
		Vector4i r = m_content.ClientRect;

		r.bottom = r.top + ItemHeight;

		int offset = (int)(ItemHeight * ((float)i - m_pos));

		r += Vector4i(0, offset).xyxy();

		return r;
	}

	int List::HitTest(const Vector2i& p)
	{
		for(int i = 0, j = ItemCount; i < j; i++)
		{
			if(GetItemRect(i).contains(p))
			{
				return i;
			}
		}

		return -1;
	}
}