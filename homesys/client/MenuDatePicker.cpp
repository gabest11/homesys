#include "stdafx.h"
#include "MenuDatePicker.h"

namespace DXUI
{
	MenuDatePicker::MenuDatePicker() 
	{
		m_calendar.ClickedEvent.Add(&MenuDatePicker::OnDate, this);
		m_hour.ClickedEvent.Add(&MenuDatePicker::OnHour, this);
	}

	bool MenuDatePicker::OnDate(Control* c, CTime t)
	{
		m_hour.Focused = true;

		return true;
	}

	bool MenuDatePicker::OnHour(Control* c, int index)
	{
		CTime t = m_calendar.CurrentDate;

		DateTimeEvent(this, t + CTimeSpan(0, 6 + index - t.GetHour(), 0, 0));

		return true;
	}
}
