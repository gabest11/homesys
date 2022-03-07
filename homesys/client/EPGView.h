#pragma once

#include "EPGView.dxui.h"
#include "Player.h"

namespace DXUI
{
	class EPGView : public EPGViewDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnColsChanged(Control* c);
		bool OnPaintOverlayForeground(Control* c, DeviceContext* dc);
		bool OnPaintChannelForeground(Control* c, DeviceContext* dc);
		bool OnColFocused(Control* c, bool focused);
		bool OnColClicked(Control* c);
		bool OnNavigateLeft(Control* c);
		bool OnNavigateRight(Control* c);
		bool OnNavigateUp(Control* c);
		bool OnNavigateDown(Control* c);
		bool OnProgramClicked(Control* c, Homesys::Program* p);

		int FindPreset(int id);

		Homesys::Program m_program;

		void InitRows(ITunerPlayer* t);
		void FreeRows();

	public:
		EPGView();

		bool Create(const Vector4i& rect, Control* parent);
		void MakeVisible(int presetId, CTime time);

		Control m_overlay;

		std::vector<Homesys::Preset> m_presets;
		std::vector<EPGRow*> m_rows;
		GUID m_tunerId;

		Property<bool> Compact;
		Property<int> PresetRows;
		Property<CTime> Start;
		Property<CTimeSpan> Duration;
		Property<Homesys::Program*> CurrentProgram;

		Event<Homesys::Program*> ProgramClicked;
	};
}
