#pragma once

#include "LinkButton.dxui.h"

namespace DXUI
{
	class LinkButton : public LinkButtonDXUI
	{
		bool OnPaintBegin(Control* c);
		bool OnClicked(Control* c);

	public:
		LinkButton();

		Property<std::wstring> Url;
	};
}
