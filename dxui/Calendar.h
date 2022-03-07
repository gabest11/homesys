#pragma once

#include "Control.h"
#include "MenuList.h"
#include <atltime.h>

namespace DXUI
{
	class Calendar : public Control
	{
		void SetCurrentDate(CTime& t);

		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnPaintForeground(Control* c, DeviceContext* dc);

		struct Day : public AlignedClass<16>
		{
			Vector4i r;
			CTime t;
		};

		Day* m_grid[6];
		Vector2i m_pos;

	public:
		Calendar();
		virtual ~Calendar();

		Event<CTime> ClickedEvent;
		Event<CTime> SelectedEvent;

		Property<CTime> CurrentDate;
	};

	class CalendarHour : public MenuList
	{
		std::vector<int> m_items;

	public:
		CalendarHour();

		Event<int> HourClickedEvent;
	};
}
