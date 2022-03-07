#pragma once

#include "MenuRecord.dxui.h"

namespace DXUI
{
	class MenuRecord : public MenuRecordDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnMenuItemClicked(Control* c, int index);
		bool OnProgramSelected(Control* c, int index);
		
		typedef MenuList::Item Item;

		std::vector<Item> m_menu_items;
		std::vector<Homesys::Program> m_program_items;
		std::map<int, Homesys::Channel> m_channels;

	public:
		MenuRecord();

		bool Create(const Vector4i& rect, Control* parent);

		Property<int> CurrentProgramId;
	};
}
