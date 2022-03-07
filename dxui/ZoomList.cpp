#include "stdafx.h"
#include "ZoomList.h"

namespace DXUI
{
	ZoomList::ZoomList() 
		: m_pos(0)
		, m_scrollpos(0)
	{
		Selected.Set([&] (int& value) {int i = std::max<int>(value, 0); value = std::min<int>(i, ItemCount - 1); m_scrollpos = (float)i;});
		ItemHeight.Set([&] (int& value) {if(TextHeight == 0) TextHeight = value; Selected = 0;});
		ActiveItemHeight.Get([&] (int& value) {if(value == 0) {value = ItemHeight;}});
		ActiveItemHeight.Set([&] (int& value) {Selected = 0;});
		ItemRows.Get([&] (float& value) {int h = Height, aih = ActiveItemHeight; value = h <= aih ? (float)h / aih : (float)(h - aih) / ItemHeight + 1;});

		ActivatedEvent.Add(&ZoomList::OnActivated, this);
		PaintForegroundEvent.Add(&ZoomList::OnPaintForeground, this, true);
		PaintItemBackgroundEvent.Add(&ZoomList::OnPaintItemBackground, this, true);
		PaintItemEvent.Add(&ZoomList::OnPaintItem, this, true);
		KeyEvent.Add(&ZoomList::OnKey, this);
		MouseEvent.Add(&ZoomList::OnMouse, this);
		FocusSetEvent.Add0(&ZoomList::OnFocusSet, this);
		ClickedEvent.Add([] (Control* c, int i) -> bool {return UserActionEvent(c);});
		Horizontal.ChangedEvent.Add(&ZoomList::OnHorizontalChanged, this);

		ItemHeight = 30;
		Selected = -1;
		ItemBackgroundOverflow = Vector4i::zero();
	}

	bool ZoomList::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_scrollpos = 0;
			m_pos = 0;
		}

		return true;
	}

	bool ZoomList::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		if(abs(m_pos - m_scrollpos) > 0.001f)
		{
			m_pos = (m_pos * 3 + m_scrollpos) / 4;
		}
		else
		{
			m_pos = m_scrollpos;
		}

		//

		Vector4i wr = WindowRect;

		Vector4i rsel = wr;

		dc->SetScissor(&rsel);

		int items = ItemCount;
		
		int pos = std::min<int>(items, (int)m_pos);

		int start = pos;

		bool horizontal = Horizontal;

		int top = horizontal ? 0 : 1;
		int bottom = horizontal ? 2 : 3;

		while(start > 0 && C2W(GetItemRect(start)).v[bottom] > wr.v[top])
		{
			start--;
		}

		int end = pos;

		while(end < items && C2W(GetItemRect(end)).v[top] < wr.v[bottom])
		{
			end++;
		}

		for(int i = start; i < end; i++)
		{
			if(Selected != i)
			{
				PaintItemParam p;
				
				p.dc = dc;
				p.index = i;
				p.rect = C2W(GetItemRect(i));
				p.selected = false;
				p.zoomed = abs(m_pos - i) < 0.1;
				p.text = GetItemText(i);
				
				dc->TextColor = TextColor;
				// TextUnderline = p.selected && FocusedPath;
				// dc->TextStyle = TextStyle;
					
				Texture* t = dc->GetTexture(ItemBackgroundImage->c_str());

				if(t != NULL)
				{
					Vector4i r = p.rect;
					
					if(!horizontal) r.left = rsel.left;

					Vector4i margin = ItemBackgroundMargin;

					Vector4i overflow = ItemBackgroundOverflow;

					r = r.inflate(overflow);
					
					PaintItemBackgroundParam p2;

					p2.dc = p.dc;
					p2.t = t;
					p2.index = p.index;
					p2.rect = r;
					p2.margin = margin;
					p2.alpha = 1.0f;
					p2.selected = p.selected;
					p2.zoomed = p.zoomed;

					PaintItemBackgroundEvent(this, &p2);
				}

				p.rect = p.rect.deflate(TextMargin);

				PaintItemEvent(this, &p);
			}
		}

		for(int i = start; i < end; i++)
		{
			if(Selected == i)
			{
				PaintItemParam p;
				
				p.dc = dc;
				p.index = i;
				p.rect = C2W(GetItemRect(i));
				p.selected = true;
				p.zoomed = std::abs(m_pos - i) < 0.1f;
				p.text = GetItemText(i);
				
				dc->TextColor = SelectedTextColor;
				// TextUnderline = p.selected && FocusedPath;
				// dc->TextStyle = TextStyle;

				if(p.rect.v[top] < wr.v[bottom] && p.rect.v[bottom] > wr.v[top])
				{
					int size = horizontal ? p.rect.width() : p.rect.height();

					float alpha = 1;

					if(p.rect.v[top] < wr.v[top])
					{
						alpha = std::max<float>(0, 1.0f - (float)(wr.v[top] - p.rect.v[top]) / size);
					}

					if(p.rect.v[bottom] > wr.v[bottom])
					{
						alpha = std::max<float>(0, 1.0f - (float)(p.rect.v[bottom] - wr.v[bottom]) / size);
					}

					dc->SetScissor(NULL);

					Texture* t = dc->GetTexture((FocusedPath ? SelectedBackgroundImage : ItemBackgroundImage)->c_str());

					if(t != NULL)
					{
						Vector4i r = p.rect;
						
						if(!horizontal) r.left = rsel.left;

						Vector4i margin = SelectedBackgroundMargin;

						Vector4i overflow = ItemBackgroundOverflow;

						r = r.inflate(overflow);

						PaintItemBackgroundParam p2;

						p2.dc = p.dc;
						p2.t = t;
						p2.index = p.index;
						p2.rect = r;
						p2.margin = margin;
						p2.alpha = alpha;
						p2.selected = p.selected;
						p2.zoomed = p.zoomed;

						PaintItemBackgroundEvent(this, &p2);
					}

					dc->SetScissor(&rsel);
				}
				
				p.rect = p.rect.deflate(TextMargin);

				PaintItemEvent(this, &p);

				break;
			}
		}

		dc->SetScissor(NULL);

		return true;
	}

	bool ZoomList::OnPaintItemBackground(Control* c, PaintItemBackgroundParam* p)
	{
		p->dc->Draw9x9(p->t, p->rect, p->margin, p->alpha);

		return true;
	}

	bool ZoomList::OnPaintItem(Control* c, PaintItemParam* p)
	{
		p->dc->Draw(p->text.c_str(), p->rect);

		return true;
	}

	bool ZoomList::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			int i = Selected;

			if(p.cmd.key == VK_RETURN && i >= 0 && i < ItemCount)
			{
				ClickedEvent(this, i);

				return true;
			}
			else if(p.cmd.key == (Horizontal ? VK_LEFT : VK_UP))
			{
				if(ItemCount > 0 && i > 0)
				{
					Selected = std::max<int>(i - 1, 0);

					return true;
				}
				else
				{
					OverflowEvent(this, -1);

					return false;
				}
			}
			else if(p.cmd.key == (Horizontal ? VK_RIGHT : VK_DOWN))
			{
				if(ItemCount > 0 && i < ItemCount - 1)
				{
					Selected = i + 1;

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
				Selected = (int)std::max<int>(i - ItemRows, 0);
			
				return true;
			}
			else if(p.cmd.key == VK_NEXT)
			{
				Selected = (int)std::min<int>(i + ItemRows, ItemCount - 1);
			
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

	bool ZoomList::OnMouse(Control* c, const MouseEventParam& p)
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
			if(p.delta > 0)
			{
				if(ItemCount > 0 && Selected > 0)
				{
					Selected = std::max<int>(Selected - 1, 0);
				}
				else
				{
					return OverflowEvent(this, -1);
				}
			}
			else if(p.delta < 0)
			{
				if(ItemCount > 0 && Selected < ItemCount - 1)
				{
					Selected = Selected + 1;
				}
				else
				{
					return OverflowEvent(this, +1);
				}
			}

			return true;
		}

		return false;
	}

	bool ZoomList::OnFocusSet(Control* c)
	{
		if((Selected < 0 || Selected >= ItemCount) && ItemCount > 0)
		{
			Selected = 0;
		}

		return true;
	}

	bool ZoomList::OnHorizontalChanged(Control* c, bool hor)
	{
		if(hor)
		{
			TabTop = true;
			TabBottom = true;
			TabLeft = false;
			TabRight = false;
		}
		else
		{
			TabTop = false;
			TabBottom = false;
			TabLeft = true;
			TabRight = true;
		}

		return true;
	}

	Vector4i ZoomList::GetItemRect(int i)
	{
/*
3 1 2
0
 0.0 (1.0) 2.0*
 2.0 (2.5) 3.0
 3.0 (3.5) 4.0
1
-0.5 (0.0) 0.5
 0.5 (1.5) 2.5*
 2.5 (3.0) 3.5
2
-1.0 (-.5) 0.0
 0.0 (0.5) 1.0
 1.0 (2.0) 3.0*

4 1 3
0
 0.0 (1.5) 3.0*
 3.0 (3.5) 4.0
 4.0 (4.5) 5.0
1
 -.5 (0.0) 0.5
 0.5 (2.0) 3.5*
 3.5 (4.0) 4.5
2
-1.0 (-.5) 0.0
 0.0 (0.5) 1.0
 1.0 (2.5) 4.0*
 */
		bool horizontal = Horizontal;

		int top = horizontal ? 0 : 1;
		int bottom = horizontal ? 2 : 3;

		int count = ItemCount;

		int h = horizontal ? Width : Height;
		int ih = ItemHeight;
		int aih = ActiveItemHeight;
		int fh = (count - 1) * ih + aih;

		float y = 0;

		if(fh > h && count > 0)
		{
			y = m_pos / (count - 1) * (h - fh);
		}

		float size = ih;

		float f = m_pos - floor(m_pos);

		int pos = (int)m_pos;

		if(i <= pos)
		{
			y += i * ih;

			if(i == pos)
			{
				size = f * (ih - aih) + aih;
			}
		}
		else
		{
			y += pos * ih;

			if(i == pos + 1)
			{
				y += f * (ih - aih) + aih;
				size = f * (aih - ih) + ih;
			}
			else
			{
				y += aih + ih;
				y += (i - (pos + 2)) * ih;
			}
		}

		Vector4i r = ClientRect;

		if(horizontal)
		{
			r.left = y;
			r.right = r.left + size;
		}
		else
		{
			r.top = y;
			r.bottom = r.top + size;
		}

		return r;
	}

	float ZoomList::GetItemHeight(int i)
	{
		int ih = ItemHeight;
		int aih = ActiveItemHeight;

		float f = abs(m_pos - i);

		return (f < 1 ? (1.0f - f) * (aih - ih) : 0.0f) + ih;
	}

	int ZoomList::HitTest(const Vector2i& p)
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