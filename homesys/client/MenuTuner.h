#pragma once

#include "MenuTuner.dxui.h"

namespace DXUI
{
	class MenuTuner : public MenuTunerDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnNextClicked(Control* c);

		std::list<Homesys::TunerDevice> m_devs;

		void Clear();

	public:
		MenuTuner();
		virtual ~MenuTuner();

		std::list<CheckButton*> m_tuners;
	};
}
