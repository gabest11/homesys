#include "stdafx.h"
#include "MenuEPGChannel.h"
#include "client.h"

namespace DXUI
{
	MenuEPGChannel::MenuEPGChannel()
	{
		ChannelId.Set(&MenuEPGChannel::SetChannelId, this);

		ActivatedEvent.Add(&MenuEPGChannel::OnActivated, this);
		DeactivatedEvent.Add(&MenuEPGChannel::OnDeactivated, this);
		m_channelview.m_list.Selected.ChangedEvent.Add(&MenuEPGChannel::OnProgramSelected, this);
		m_channelview.m_list.ClickedEvent.Add(&MenuEPGChannel::OnProgramClicked, this);
	}

	void MenuEPGChannel::SetChannelId(int& value)
	{
		m_channelview.m_list.CurrentChannelId = value;
		m_channelview.m_list.CurrentDate = CTime::GetCurrentTime();
	}

	bool MenuEPGChannel::OnActivated(Control* c, int dir)
	{
		m_channelview.Focused = true;

		return true;
	}

	bool MenuEPGChannel::OnDeactivated(Control* c, int dir)
	{
		return true;
	}

	bool MenuEPGChannel::OnProgramSelected(Control* c, int index)
	{
		m_program.CurrentProgram = m_channelview.m_list.CurrentProgram;

		return true;
	}

	bool MenuEPGChannel::OnProgramClicked(Control* c, int index)
	{
		Homesys::Program* p = m_channelview.m_list.CurrentProgram;

		if(p != NULL)
		{
			m_dialog.m_switch.UserData = (UINT_PTR)p->channelId;
			m_dialog.m_details.UserData = (UINT_PTR)p->channelId;
			m_dialog.m_record.UserData = (UINT_PTR)p->id;
			m_dialog.Show(&m_channelview);
		}

		return true;
	}
}
