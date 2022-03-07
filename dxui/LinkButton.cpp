#include "stdafx.h"
#include "LinkButton.h"
#include <shellapi.h>

namespace DXUI
{
	LinkButton::LinkButton()
	{
		Url.Set([&] (std::wstring& value) {if(Text->empty()) Text = value;});
		CanFocus.Get([&] (int& value) {value = !Url->empty() ? 2 : 0;});

		PaintBeginEvent.Add0(&LinkButton::OnPaintBegin, this);
		ClickedEvent.Add0(&LinkButton::OnClicked, this);
	}

	bool LinkButton::OnPaintBegin(Control* c)
	{
		TextUnderline = Hovering || FocusedPath;

		return true;
	}

	bool LinkButton::OnClicked(Control* c)
	{
		std::wstring url = Url;

		if(!url.empty())
		{
			ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOW);
		}

		return true;
	}
}