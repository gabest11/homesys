#include "stdafx.h"
#include "MenuList.h"

namespace DXUI
{
	MenuList::MenuList()
		: m_scrollpos(0)
		, m_targetscrollpos(0)
		, m_slidestart(0)
	{
		Selected.Set([&] (int& value)
		{
			value = std::min<int>(std::max<int>(value, 0), ItemCount - 1);
			m_slider.Position = ItemCount > 1 ? (float)value / (ItemCount - 1) : 0;
			MakeVisible(value);
		});

		ActivatedEvent.Add(&MenuList::OnActivated, this);
		PaintForegroundEvent.Add(&MenuList::OnPaintForeground, this);
		KeyEvent.Add(&MenuList::OnKey, this);
		MouseEvent.Add(&MenuList::OnMouse, this);
		ClickedEvent.Add(&MenuList::OnClicked, this);
		ItemCount.Get([&] (int& count) {count = Items ? Items->size() : NULL;});
		ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {if(Items != NULL) p->text = (*Items)[p->index].m_text; return true;});
		Items.ChangedEvent.Add0(&MenuList::OnItemsChanged, this);
		m_slider.ScrollEvent.Add(&MenuList::OnScroll, this);

		ItemHeight = 57;
	}

	void MenuList::MakeVisible(int i)
	{
		float range = (float)Height / ItemHeight;

		int items = std::max<int>(1, (int)(range / 2));

		if(m_scrollpos < (float)i + 1 - (range - items))
		{
			m_targetscrollpos = (float)i + 1 - (range - items);
		}

		if(m_scrollpos > (float)i - items)
		{
			m_targetscrollpos = (float)i - items;
		}

		m_targetscrollpos = std::max<float>(std::min<float>(m_targetscrollpos, ItemCount - range), 0);
	}

	void MenuList::SlideStart(clock_t delay)
	{
		m_slidestart = clock() + delay;
	}

	bool MenuList::OnActivated(Control* c, int dir)
	{
		SlideStart(500);

		return true;
	}

	bool MenuList::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		float diff = m_targetscrollpos - m_scrollpos;

		if(abs(diff) > 0.01f) m_scrollpos += diff * 0.2f;
		else m_scrollpos = m_targetscrollpos;

		int o = (int)(fmod(m_scrollpos, 1) * ItemHeight + 0.5);
		int h = Height;

		Vector4i r = WindowRect;

		dc->SetScissor(&r);

		for(int i = (int)m_scrollpos, j = ItemCount; i < j; i++)
		{
			Vector4i dst = GetItemRect(NULL, i);

			if(dst.top >= h) break;
			if(dst.bottom < 0) break;
			// if(dst.top < 0) continue;
			// if(dst.bottom > h) continue;

			std::wstring id;
			Vector4i margin;
			bool selected;
			
			if(Focused && i == Selected)
			{
				id = SelectedBackgroundImage->c_str();
				margin = SelectedBackgroundMargin;
				selected = true;
			}
			else
			{
				id = ItemBackgroundImage->c_str();
				margin = ItemBackgroundMargin;
				selected = false;
			}

			Texture* t = dc->GetTexture(id.c_str());

			if(t != NULL)
			{
				dc->Draw9x9(t, C2W(dst), margin);
			}

			dst = C2W(dst.deflate(TextMargin));

			std::wstring str = GetItemText(i);

			if(selected) DrawScrollText(dc, str.c_str(), dst);
			else dc->Draw(str.c_str(), dst);
		}

		dc->SetScissor(NULL);

		m_slider.Step = ItemCount > 0 ? 1.0f / ItemCount : 0;;

		return true;
	}

	bool MenuList::OnKey(Control* c, const KeyEventParam& p)
	{
		if(ItemCount <= 0)
		{
			return false;
		}

		if(p.mods == 0)
		{
			int i = Selected;

			bool update = false;

			if(p.cmd.key == VK_DOWN)
			{
				if(i < ItemCount - 1 || !TabBottom)
				{
					if(++i >= ItemCount) i = 0;
					
					update = true;
				}
			}
			else if(p.cmd.key == VK_UP)
			{
				if(i > 0 || !TabTop)
				{
					if(--i < 0) i = ItemCount - 1;
					
					update = true;
				}
			}
			else if(p.cmd.key == VK_RETURN)
			{
				ClickedEvent(this, i);

				return true;
			}
			else if(p.cmd.key == VK_ESCAPE || p.cmd.key == VK_BACK || p.cmd.app == APPCOMMAND_BROWSER_BACKWARD)
			{
				Item& item = Items->back();

				if(item.m_id == 0 && item.m_target != NULL)
				{
					Items = item.m_target;

					Selected = 0;

					return true;
				}
			}

			if(update)
			{
				Selected = i;

				return true;
			}
		}

		return false;
	}

	bool MenuList::OnMouse(Control* c, const MouseEventParam& p)
	{
		int i = HitTest(p.point);
				
		if(p.cmd == MouseEventParam::Move)
		{
			if(i >= 0 && AutoSelect)
			{
				Selected = i;
			
				return true;
			}
		}
		else if(p.cmd == MouseEventParam::Down)
		{
			if(i >= 0)
			{
				if(!AutoSelect) 
				{
					Selected = i;
				}

				ClickedEvent(this, i);

				return true;
			}
		}
		else if(p.cmd == MouseEventParam::RDown)
		{
			std::vector<Item>* items = Items;

			if(items != NULL && !items->empty())
			{
				Item& item = items->back();

				if(item.m_id == 0 && item.m_target != NULL)
				{
					Items = item.m_target;

					Selected = 0;

					return true;
				}
			}
		}
		else if(p.cmd == MouseEventParam::Wheel)
		{
			Selected = Selected - p.delta;

			return true;
		}

		return false;
	}

	bool MenuList::OnClicked(Control* c, int index)
	{
		if(index >= 0 && Items != NULL)
		{
			const Item& item = (*Items)[index];

			if(item.m_target != NULL)
			{
				Items = item.m_target;
				
				Selected = 0;
			}
			else
			{
				MenuClickedEvent(this, item.m_id);
			}

			UserActionEvent(c);

			return true;
		}

		return false;
	}

	bool MenuList::OnItemsChanged(Control* c)
	{
		m_scrollpos = 0;
		
		SlideStart();

		return true;
	}

	bool MenuList::OnScroll(Control* c, float pos)
	{
		if(ItemCount > 0)
		{
			Selected = (int)(pos * (ItemCount - 1) + 0.5f);

			return true;
		}

		return false;
	}

	Vector4i MenuList::GetItemRect(DeviceContext* dc, int i)
	{
		Vector4i cr = ClientRect;

		Vector4i r(0, 0, Width, ItemHeight);
		
		if(m_slider.Visible)
		{
			r.right = MapClientRect(&m_slider).left;
		}

		if(dc)
		{
			Vector4i bbox;

			TextEntry* te = dc->Draw(GetItemText(i).c_str(), r, Vector2i(0, 0), true);

			if(te != NULL)
			{
				r.left = (r.width() - te->bbox.width()) / 2;
				r.right = r.left + te->bbox.width();
			}
		}

		r += Vector4i(0, cr.top + (i - (int)m_scrollpos) * ItemHeight - (int)(fmod(m_scrollpos, 1) * ItemHeight + 0.5)).xyxy();

		float f = (float)(clock() - m_slidestart) / 150 - i - 1;

		if(f < 1)
		{
			if(f < 0) f = 0;

			f = 1.0f - f;

			if(1) // !(i & 1))
			{
				// int dir = (i & 1) ? 1 : -1;

				Vector4i v1 = WindowRect;
				Vector4i v2 = GetRoot()->WindowRect;

				int dir = ((v1.left + v1.right) - (v2.left + v2.right)) / 2 > 0 ? 1 : -1;

				r += Vector4i((int)(f * r.width() * dir), 0).xyxy();
			}
			else
			{
				r += Vector4i(0, (int)(f * Height)).xyxy();
			}
		}

		return r;
	}

	int MenuList::HitTest(const Vector2i& p)
	{
		if(!ClientRect->contains(p))
		{
			return -1;
		}

		// LAME
		
		for(int i = 0, j = ItemCount; i < j; i++)
		{
			if(GetItemRect(NULL, i).contains(p)) 
			{
				return i;
			}
		}

		return -1;
	}
}