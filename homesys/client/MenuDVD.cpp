#include "stdafx.h"
#include "MenuDVD.h"
#include "client.h"

namespace DXUI
{
	MenuDVD::MenuDVD()
	{
		ActivatedEvent.Add(&MenuDVD::OnActivated, this);		
	}

	void MenuDVD::Reload()
	{
		m_drives.clear();
		
		for(wchar_t c = 'C'; c <= 'Z'; c++)
		{
			std::wstring label;

			int type = DirectShow::GetDriveType(c);

			if(type == DirectShow::Drive_DVDVideo)
			{
				label = DirectShow::GetDriveLabel(c);
			}
			else if(type == DirectShow::Drive_Audio)
			{
				label = DirectShow::GetDriveLabel(c);
			}
			else
			{
				continue;
			}

			if(label.size() > 13)
			{
				label = label.substr(0, 13) + L"...";
			}

			m_drives.push_back(MenuList::Item(Util::Format(L"%s (%c:)", label.c_str(), c), c, L""));
		}

		m_list.Items = &m_drives;
		m_list.Selected = 0;
		m_list.Focused = true;
	}

	bool MenuDVD::OnActivated(Control* c, int dir)
	{
		if(!g_env->players.empty())
		{
			CComPtr<IGenericPlayer> p = g_env->players.front();

			if(!CComQIPtr<IDVDPlayer>(p)) 
			{
				g_env->CloseFileEvent(this);
			}
		}

		Reload();

		return true;
	}
}