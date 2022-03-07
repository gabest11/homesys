#pragma once

#include "MenuRecordingModify.dxui.h"

namespace DXUI
{
	class MenuRecordingModify : public MenuRecordingModifyDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnNext(Control* c);
		bool OnRecordingIdChanged(Control* c, GUID recordingId);
		bool OnThreadTimer(Control* c, UINT64 n);
		bool OnCurrentDateTimeChanged(Control* c, CTime t);
		bool OnPresetChanged(Control* c, int index);

		std::vector<int> m_delay;
		std::vector<Homesys::Preset> m_presets;
		Homesys::Program m_program;
		Thread m_thread;

		void UpdateProgram();

	public:
		MenuRecordingModify();

		Property<GUID> RecordingId;
	};
}
