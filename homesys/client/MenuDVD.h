#pragma once

#include "MenuDVD.dxui.h"

namespace DXUI
{
	class MenuDVD : public MenuDVDDXUI
	{
		bool OnActivated(Control* c, int dir);

		std::vector<MenuList::Item> m_drives;

	public:
		MenuDVD();

		void Reload();

		Event<TCHAR> ClickedEvent;
	};
}
