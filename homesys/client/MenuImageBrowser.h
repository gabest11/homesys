#pragma once

#include "MenuImageBrowser.dxui.h"

namespace DXUI
{
	class MenuImageBrowser : public MenuImageBrowserDXUI
	{
		bool OnReturnKey(Control* c);
		bool OnDirectory(Control* c, std::wstring path);
		bool OnFile(Control* c, const NavigatorItem* item);

	public:
		MenuImageBrowser();
	};
}
