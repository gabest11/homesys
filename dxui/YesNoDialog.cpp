#include "stdafx.h"
#include "YesNoDialog.h"

namespace DXUI
{
	YesNoDialog::YesNoDialog()
	{
		m_no.ClickedEvent.Add0([&] (Control* c) -> bool {return OnClose(c);});
		m_yes.ClickedEvent.Add0([&] (Control* c) -> bool {return OnClose(c);});
	}
}
