#include "stdafx.h"
#include "Table.h"

namespace DXUI
{
	Table::Table() 
	{
		m_anim.Set(0, 1, 300);
		m_anim_index = false;

		ThumbWidth.Get(&Table::GetThumbWidth, this);
		ThumbHeight.Get(&Table::GetThumbHeight, this);

		PaintForegroundEvent.Add(&Table::OnPaintForeground, this, true);
		PaintItemBackgroundEvent.Add(&Table::OnPaintItemBackground, this, true);
		PaintItemEvent.Add(&Table::OnPaintItem, this, true);
		KeyEvent.Add(&Table::OnKey, this);
		MouseEvent.Add(&Table::OnMouse, this);
		Selected.ChangedEvent.Add(&Table::OnSelectedChanged, this);

		ThumbGapX = 20;
		ThumbGapY = 20;
		ThumbCountH = 7;
		ThumbCountV = 3;
		SelectedSize = 30;
		CanFocus = 1;
		TextHeight = 30;
		TextAlign = Align::Center | Align::Bottom;
	}

	void Table::GetThumbWidth(int& value)
	{
		int count = ThumbCountH;

		value = count > 0 ? Width / count : value;
	}

	void Table::GetThumbHeight(int& value)
	{
		int count = ThumbCountV;

		value = count > 0 ? Height / count : value;
	}

	int Table::HitTest(const Vector2i& p)
	{
		int i = TopLeft + (p.y / ThumbHeight) * ThumbCountH + (p.x / ThumbWidth);

		if(i < ItemCount)
		{
			return i;
		}

		return -1;
	}

	bool Table::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		int w = ThumbCountH;
		int h = ThumbCountV;
		int tw = ThumbWidth;
		int th = ThumbHeight;
		int tgx = ThumbGapX;
		int tgy = ThumbGapY;
		int count = ItemCount;

		for(int z = 0; z < 2; z++)
		{
			int i = std::min<int>(std::max<int>(0, TopLeft), count);
		
			for(int y = 0; y < h; y++)
			{
				for(int x = 0; x < w && i < count; x++, i++)
				{
					Vector4i r;

					r.left = x * tw;
					r.top = y * th;
					r.right = r.left + tw;
					r.bottom = r.top + th;

					r = C2W(r.deflate(Vector4i(tgx, tgy).xyxy()));

					bool selected = i == Selected;

					if(z ^ (selected ? 1 : 0))
					{
						continue;
					}

					if(selected)
					{
						float fx = m_anim;

						fx = sqrt(fx) * SelectedSize;

						float fy = fx * (r.bottom - r.top) / (r.right - r.left);

						r = r.inflate(Vector4i(Vector4(fx, fy, fx, fy)));
					}

					{
						Vector4i margin = Vector4i(14, 14, 16, 14);
		
						PaintItemBackgroundParam p;

						p.dc = dc;
						p.t = dc->GetTexture(selected && FocusedPath ? L"ListSelectedBackground.png" : L"ListSelectedInactiveBackground.png");
						p.index = i;
						p.rect = r.inflate(margin);
						p.margin = margin;
						p.alpha = 1.0f;
						p.selected = selected;
						p.zoomed = false;

						PaintItemBackgroundEvent(this, &p);
					}

					PaintItemParam p;
					
					p.dc = dc;
					p.index = i;
					p.rect = r;
					p.selected = selected;
					p.zoomed = false;
					p.text = GetItemText(i);
					
					PaintItemEvent(this, &p);
				}
			}
		}

		return true;
	}

	bool Table::OnPaintItemBackground(Control* c, PaintItemBackgroundParam* p)
	{
		p->dc->Draw9x9(p->t, p->rect, p->margin, p->alpha);

		return true;
	}

	bool Table::OnPaintItem(Control* c, PaintItemParam* p)
	{
		p->dc->Draw(p->text.c_str(), p->rect);

		return true;
	}

	bool Table::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			int w = ThumbCountH;
			int h = ThumbCountV;
			int page = w * h;
			int i = Selected;
			int count = ItemCount;

			if(p.cmd.key == VK_LEFT)
			{
				if(TabLeft && (i % w) == 0)
				{
					return false;
				}

				if(i > 0)
				{
					Selected = i - 1;

					return true;
				}
			}
			else if(p.cmd.key == VK_RIGHT)
			{
				if(TabRight && (i % w) == w - 1)
				{
					return false;
				}

				if(i < count - 1)
				{
					Selected = i + 1;

					return true;
				}
			}
			else if(p.cmd.key == VK_UP)
			{
				if(i >= w)
				{
					Selected = i - w;

					return true;
				}
			}
			else if(p.cmd.key == VK_DOWN)
			{
				if(i < count - count % w)
				{
					Selected = std::min<int>(i + w, count - 1);

					return true;
				}
			}
			else if(p.cmd.key == VK_PRIOR)
			{
				if(i > 0)
				{
					Selected = std::max<int>(i - page, 0);

					return true;
				}
			}
			else if(p.cmd.key == VK_NEXT)
			{
				if(i < count - 1)
				{
					Selected = std::min<int>(i + page, count - 1);

					return true;
				}
			}
			else if(p.cmd.key == VK_HOME)
			{
				if(i > 0)
				{
					Selected = 0;
				}

				return true;
			}
			else if(p.cmd.key == VK_END)
			{
				if(i < count - 1)
				{
					Selected = count - 1;
				}

				return true;
			}
			else if(p.cmd.key == VK_RETURN)
			{
				if(i >= 0 && i < count)
				{
					ClickedEvent(this, i);
				}

				return true;
			}
		}

		return false;
	}

	bool Table::OnMouse(Control* c, const MouseEventParam& p)
	{
		int count = ItemCount;

		if(p.cmd == MouseEventParam::Move)
		{
			int index = HitTest(p.point);

			if(index >= 0 && index < count)
			{
				Selected = index;
			}

			return true;
		}
		else if(p.cmd == MouseEventParam::Dbl)
		{
			int index = HitTest(p.point);

			if(index >= 0 && index < count)
			{
				ClickedEvent(this, index);
			}

			return true;
		}
		else if(p.cmd == MouseEventParam::Wheel)
		{
			int w = ThumbCountH;

			float delta = p.delta;

			if(delta > 0)
			{
				int selected = Selected;

				while((delta -= 1) >= 0) 
				{
					selected = selected - std::min<int>(1, selected);
				}

				Selected = selected;
			}
			else if(delta < 0)
			{
				int selected = Selected;

				while((delta += 1) <= 0) 
				{
					selected = selected + std::min<int>(1, (count - 1) - selected);
				}

				Selected = selected;
			}

			return true;
		}


		return false;
	}

	bool Table::OnSelectedChanged(Control* c, int index)
	{
		int w = ThumbCountH;
		int h = ThumbCountV;

		int count = ItemCount;

		if(index >= 0 && index < count && w > 0)
		{
			if(index < TopLeft)
			{
				TopLeft = index / w * w;
			}
			else 
			{
				while(index >= TopLeft + w * h)
				{
					TopLeft = TopLeft + w;
				}
			}
		}

		if(index != m_anim_index)
		{
			m_anim.SetSegment(0);
			m_anim_index = index;
		}

		return true;
	}
}