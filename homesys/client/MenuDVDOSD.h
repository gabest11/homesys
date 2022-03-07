#pragma once

#include "MenuDVDOSD.dxui.h"

namespace DXUI
{
	class MenuDVDOSD : public MenuDVDOSDDXUI
	{
		bool GetSeekbarVisible(bool value);
		bool GetControlsVisible(bool value);

		bool OnActivated(Control* c, int dir);
		bool OnEndFile(Control* c);
		bool OnInfoEvent(Control* c);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnRed(Control* c);

		DXUI::Event<> EndFileEvent;
		DXUI::Event<> InfoEvent;

	public:
		MenuDVDOSD();
		virtual ~MenuDVDOSD();

		Property<bool> HandleEndOfFile;
	};
}
