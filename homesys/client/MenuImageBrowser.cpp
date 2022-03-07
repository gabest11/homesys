#include "stdafx.h"
#include "MenuImageBrowser.h"
#include "client.h"

namespace DXUI
{
	MenuImageBrowser::MenuImageBrowser()
	{
		m_location.ReturnKeyEvent.Add0(&MenuImageBrowser::OnReturnKey, this);
		m_imagebrowser.m_navigator.Location.ChangedEvent.Add(&MenuImageBrowser::OnDirectory, this);
		m_imagebrowser.m_navigator.ClickedEvent.Add(&MenuImageBrowser::OnFile, this);
	}

	bool MenuImageBrowser::OnReturnKey(Control* c)
	{
		std::wstring str = Util::TrimRight(c->Text, L"\\/");

		if(PathIsDirectory(str.c_str()) || str.size() == 2 && str[1] == ':')
		{
			m_imagebrowser.m_navigator.Location = c->Text;
		}

		return true;
	}

	bool MenuImageBrowser::OnDirectory(Control* c, std::wstring path)
	{
		m_location.Text = path;

		return true;
	}

	bool MenuImageBrowser::OnFile(Control* c, const NavigatorItem* item)
	{
		g_env->PlayFileEvent(c, m_imagebrowser.m_navigator.GetPath(item));

		return true;
	}
}