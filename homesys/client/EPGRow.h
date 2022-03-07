#pragma once

#include "EPGRow.dxui.h"
#include "EPGRowButton.h"

namespace DXUI
{
	class EPGRow : public EPGRowDXUI
	{
		void GetPosition(CTime& value);
		void SetPosition(CTime& value);

		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnCurrentPresetChanged(Control* c, Homesys::Preset* preset);
		bool OnStartChanged(Control* c, CTime start);
		bool OnDurationChanged(Control* c, CTimeSpan duration);
		bool OnKey(Control* c, const KeyEventParam& p);

		void Reset();
		void Update();
		
		std::list<Homesys::Program> m_programs;

	public:
		EPGRow();
		virtual ~EPGRow();

		std::vector<EPGRowButton*> m_cols;

		Property<Homesys::Preset*> CurrentPreset;
		Property<CTime> Start;
		Property<CTimeSpan> Duration;
		Property<CTime> Position;

		Event<bool> RowButtonFocused;
		Event<> RowButtonClicked;
		Event<> NavigateLeftEvent;
		Event<> NavigateRightEvent;
		Event<> NavigateUpEvent;
		Event<> NavigateDownEvent;
		Event<> ColsChanged;
	};
}
