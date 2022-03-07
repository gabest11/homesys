#include "stdafx.h"
#include "MenuPlaylist.h"
#include "client.h"

namespace DXUI
{
	MenuPlaylist::MenuPlaylist()
	{
		ActivatedEvent.Add(&MenuPlaylist::OnActivated, this);
		KeyEvent.Add(&MenuPlaylist::OnKey, this);
		EndFileEvent.Add0(&MenuPlaylist::OnEndFile, this);
		m_controls.m_prev.ClickedEvent.Add0([&] (Control* c) -> bool {Step(-1); return true;});
		m_controls.m_next.ClickedEvent.Add0([&] (Control* c) -> bool {Step(+1); return true;});
		m_list.ClickedEvent.Add(&MenuPlaylist::OnListClicked, this);
		m_list.ItemCount.Get([&] (int& count) {count = m_items.size();});
		m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = Util::Format(L"%d. ", p->index + 1) + m_items[p->index].title; return true;});
		m_order.m_list.ItemCount.Get([&] (int& count) {count = m_order_items.size();});
		m_order.m_list.ItemTextEvent.Add([&] (Control* c, ItemTextParam* p) -> bool {p->text = m_order_items[p->index]; return true;});

		g_env->EndFileEvent.Chain(EndFileEvent);
	}

	MenuPlaylist::~MenuPlaylist()
	{
		g_env->EndFileEvent.Unchain(EndFileEvent);
	}

	void MenuPlaylist::RemoveAll()
	{
		m_items.clear();
	}

	void MenuPlaylist::Add(LPCWSTR path, LPCWSTR title, LPCWSTR desc)
	{
		PlaylistItem item;

		item.path = path;
		item.title = title;
		item.desc = desc;

		if(item.title.empty())
		{
			WCHAR buff[MAX_PATH];

			wcscpy(buff, path);

			PathStripPath(buff);
			PathRemoveExtension(buff);

			item.title = buff;
		}

		item.desc = Util::Trim(item.title + L"\n" + item.desc);

		if(item.desc.empty())
		{
			item.desc = item.title;
		}

		m_items.push_back(item);
	}

	void MenuPlaylist::Step(int n)
	{
		int count = m_items.size();

		if(count >= 1)
		{
			int next = (m_current + n) % count;
				
			m_list.Selected = next;
			m_list.ClickedEvent(&m_list, next);
		}
	}

	bool MenuPlaylist::OnActivated(Control* c, int dir)
	{
		if(m_order_items.empty())
		{
			m_order_items.push_back(Control::GetString("REPEAT_MODE_ONE"));
			m_order_items.push_back(Control::GetString("REPEAT_MODE_ALL"));
			m_order_items.push_back(Control::GetString("REPEAT_MODE_RANDOM"));
			m_order.Selected = 1;
		}

		m_current = -1;

		m_desc.Text = L"";

		return true;
	}

	bool MenuPlaylist::OnEndFile(Control* c)
	{
		if(Activated)
		{
			int i = m_order.Selected;

			if(i == 1)
			{
				Step(1);
			}
			else if(i == 2)
			{
				Step(rand());
			}
		}

		return true;
	}

	bool MenuPlaylist::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == KeyEventParam::Ctrl)
		{
			if(p.cmd.key == VK_LEFT)
			{
				m_controls.m_rewind.Click();

				return true;
			}

			if(p.cmd.key == VK_RIGHT)
			{
				m_controls.m_forward.Click();

				return true;
			}
		}

		if(p.mods == 0)
		{
			if(p.cmd.key == VK_PRIOR || p.cmd.app == APPCOMMAND_MEDIA_PREVIOUSTRACK)
			{
				m_controls.m_prev.Click();

				return true;
			}

			if(p.cmd.key == VK_NEXT || p.cmd.app == APPCOMMAND_MEDIA_NEXTTRACK)
			{
				m_controls.m_next.Click();

				return true;
			}
		}

		return false;
	}

	bool MenuPlaylist::OnListClicked(Control* c, int index)
	{
		if(index >= 0 && index < m_items.size())
		{
			m_current = index;

			m_desc.Text = m_items[index].desc;

			g_env->OpenFileEvent(this, m_items[index].path);
		}

		return true;
	}
}