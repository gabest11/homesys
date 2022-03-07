#include "stdafx.h"
#include "MenuWizard.h"

namespace DXUI
{
	MenuWizard::MenuWizard()
	{
		ActivatedEvent.Add(&MenuWizard::OnActivated, this);
		m_prev.ClickedEvent.Add0(&MenuWizard::OnPrevClicked, this);
	}

	bool MenuWizard::OnActivated(Control* c, int dir)
	{
		m_prev.ToFront();
		m_next.ToFront();

		return true;
	}

	bool MenuWizard::OnPrevClicked(Control* c)
	{
		Close();

		return true;
	}
}