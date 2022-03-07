#include "stdafx.h"
#include "FileBrowser.h"

namespace DXUI
{
	FileBrowser::FileBrowser() 
	{
		ActivatedEvent.Add(&FileBrowser::OnActivated, this);
		m_list.Selected.ChangedEvent.Add(&FileBrowser::OnListSelected, this);
		m_list.ClickedEvent.Add(&FileBrowser::OnListClicked, this);
		m_list.PaintItemEvent.Add(&FileBrowser::OnPaintListItem, this, true);
		m_list.m_content.PaintBeginEvent.Add0([&] (Control* c) -> bool {Lock(); return true;});
		m_list.m_content.PaintEndEvent.Add0([&] (Control* c) -> bool {Unlock(); return true;});
		m_list.ItemCount.Get([&] (int& count) {count = m_items.size();});
		m_navigator.Location.ChangedEvent.Add0([&] (Control* c) -> bool {m_list.Selected = 0; return true;});

		wchar_t path[MAX_PATH];
		
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, path))) 
		{
			m_navigator.Location = path;
		}
	}

	bool FileBrowser::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
			m_history.clear();
		}

		return true;
	}

	bool FileBrowser::OnListSelected(Control* c, int index)
	{
		if(m_items[index].ni.name != L"..")
		{
			m_history[m_navigator.Location] = m_items[index].ni.name;
		}

		return true;
	}

	bool FileBrowser::OnListClicked(Control* c, int index)
	{
		bool updir = m_items[index].ni.isdir && m_items[index].ni.name == L"..";

		m_navigator.Browse(m_items[index].ni);

		if(updir)
		{
			auto i = m_history.find(m_navigator.Location);

			if(i != m_history.end())
			{
				for(int index = 0; index < m_items.size(); index++)
				{
					if(i->second == m_items[index].ni.name)
					{
						m_list.Selected = index;

						break;
					}
				}
			}
		}

		return true;
	}

	bool FileBrowser::OnPaintListItem(Control* c, PaintItemParam* p)
	{
		m_visible.insert(p->index);

		const Item& item = m_items[p->index];

		int icon_width = p->rect.height() * 4 / 3;

		Vector4i dst = p->rect;

		dst.right = dst.left + icon_width;
		dst = dst.deflate(2);

		//dc->Draw(dst, 0xffffffff);

		PaintItem(p->index, dst, p->dc);

		Vector4i pr = dst;

		c->TextHeight = p->rect.height() / 3;
		c->TextBold = item.ni.isdir;

		p->dc->TextHeight = c->TextHeight;
		p->dc->TextStyle = c->TextStyle;

		dst.left = dst.right + 5;
		dst.right = p->rect.right;

		std::wstring str;

		if(item.ni.isdir)
		{
			str = DeviceContext::Escape(item.ni.label.c_str());
		}
		else
		{
			if(!item.ni.label.empty() && item.ni.label != item.ni.name)
			{
				str = L"[{font.face: 'Arial Black'}] {" + DeviceContext::Escape(item.ni.name.c_str()) + L"}\\n";
				str += Util::Format(L"[{font.size: %d;}] {", c->TextHeight - 5) + DeviceContext::Escape(item.ni.label.c_str()) + L"}";
			}
			else
			{
				str = DeviceContext::Escape(item.ni.name.c_str());
			}
		}

		p->dc->Draw(str.c_str(), dst);

		if(item.dur > 0)
		{
			DVD_HMSF_TIMECODE tc = DirectShow::RT2HMSF(item.dur);

			if(tc.bHours)
			{
				str = Util::Format(L"%d:%02d:%02d", tc.bHours, tc.bMinutes, tc.bSeconds);
			}
			else if(tc.bMinutes)
			{
				str = Util::Format(L"%d:%02d", tc.bMinutes, tc.bSeconds);
			}
			else
			{
				str = Util::Format(L"%d s", tc.bSeconds);
			}

			str = DeviceContext::Escape(str.c_str());

			p->dc->TextAlign = Align::Center | Align::Bottom;
			p->dc->TextColor = Color::Text2;
			p->dc->TextHeight = 20;
			p->dc->TextStyle = TextStyles::Bold;

			dst = pr;
			dst.bottom -= 5;

			p->dc->Draw(str.c_str(), dst);

			p->dc->TextAlign = c->TextAlign;
			p->dc->TextColor = c->TextColor;
			p->dc->TextHeight = c->TextHeight;
			p->dc->TextStyle = c->TextStyle;
		}

		return true;
	}

	bool FileBrowser::OnLocationChanged(Control* c, std::wstring path)
	{
		return true;
	}
}