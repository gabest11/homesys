#pragma once

#include "MenuEPGChannel.dxui.h"

namespace DXUI
{
	class MenuEPGChannel : public MenuEPGChannelDXUI
	{
		void SetChannelId(int& value);

		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnProgramSelected(Control* c, int index);
		bool OnProgramClicked(Control* c, int index);

		// RecordingStateTextureFactory m_rstf;

	public:
		MenuEPGChannel();
		
		/*
		bool Create(CRect rect, Control* parent);
		
		EPGChannelView m_channelview;
		EPGDialog m_dialog;
		EPGProgram m_program;
		*/

		Property<int> ChannelId;
	};
}
