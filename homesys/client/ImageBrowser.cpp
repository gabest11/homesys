#include "stdafx.h"
#include "ImageBrowser.h"

namespace DXUI
{
	ImageBrowser::ImageBrowser() 
	{
		m_table.ClickedEvent.Add(&ImageBrowser::OnTableClicked, this);
		m_table.PaintItemEvent.Add(&ImageBrowser::OnPaintTableItem, this, true);
		m_table.PaintBeginEvent.Add0([&] (Control* c) -> bool {Lock(); return true;});
		m_table.PaintEndEvent.Add0([&] (Control* c) -> bool {Unlock(); return true;});
		m_table.ItemCount.Get([&] (int& count) {count = m_items.size();});
		m_navigator.Extensions = L".jpeg|.jpg|.gif|.png|.bmp";
		m_navigator.Location.ChangedEvent.Add0([&] (Control* c) -> bool {m_table.Selected = 0; return true;});

		wchar_t path[MAX_PATH];
		
		if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, 0, path))) 
		{
			m_navigator.Location = path;
		}
	}

	bool ImageBrowser::OnTableClicked(Control* c, int index)
	{
		m_navigator.Browse(m_items[index].ni);

		return true;
	}

	bool ImageBrowser::OnPaintTableItem(Control* c, PaintItemParam* p)
	{
		m_visible.insert(p->index);

		const Item& item = m_items[p->index];

		Vector4i dst = p->rect.deflate(2);

		PaintItem(p->index, dst, p->dc);

		std::wstring str;

		if(item.ni.isdir)
		{
			str = item.ni.label.c_str();
		}
		else
		{
			str = item.ni.name.c_str();
		}

		p->dc->Draw(str.c_str(), dst);

		return true;
	}
}