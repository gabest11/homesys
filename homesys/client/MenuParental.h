#pragma once

#include "MenuParental.dxui.h"

namespace DXUI
{
	class MenuParental : public MenuParentalDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnPasswordTextInput(Control* c);
		bool OnUpdateNext(Control* c);
		bool OnNext(Control* c);

		Homesys::ParentalSettings m_ps;
		bool m_unlocked;
		std::vector<std::wstring> m_onoffitems;
		std::vector<std::wstring> m_levelitems;
		std::vector<int> m_inactivityitems;

	public:
		MenuParental();
	};
}

