#pragma once

#include "MenuDatePicker.dxui.h"

namespace DXUI
{
	class MenuDatePicker : public MenuDatePickerDXUI
	{
		bool OnDate(Control* c, CTime t);
		bool OnHour(Control* c, int index);

	public:
		MenuDatePicker();

		Event<CTime> DateTimeEvent;
	};
}
