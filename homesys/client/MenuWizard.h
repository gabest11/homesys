#pragma once

#include "MenuWizard.dxui.h"

namespace DXUI
{
	class MenuWizard : public MenuWizardDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnPrevClicked(Control* c);

	public:
		MenuWizard();

		Event<> DoneEvent;
	};
}
