#pragma once

#include "MenuSmartCard.dxui.h"

namespace DXUI
{
	class MenuSmartCard : public MenuSmartCardDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnRed(Control* c);
		
		std::vector<Homesys::SmartCardDevice> m_cards;

	public:
		MenuSmartCard();
		virtual ~MenuSmartCard();
	};
}
