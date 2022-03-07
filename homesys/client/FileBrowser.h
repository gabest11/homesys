#pragma once

#include "FileBrowser.dxui.h"

namespace DXUI
{
	class FileBrowser : public FileBrowserDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnListSelected(Control* c, int index);
		bool OnListClicked(Control* c, int index);
		bool OnPaintListItem(Control* c, PaintItemParam* p);
		bool OnLocationChanged(Control* c, std::wstring path);

		std::map<std::wstring, std::wstring> m_history;

	public:
		FileBrowser();
	};
}
