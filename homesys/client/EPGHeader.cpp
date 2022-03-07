#include "stdafx.h"
#include "EPGHeader.h"

namespace DXUI
{
	EPGHeader::EPGHeader()
	{
		Start.Set(&EPGHeader::SetStart, this);
		Duration.Get([] (CTimeSpan& value) {value = CTimeSpan(0, 1, 30, 0);});
		m_left.ClickedEvent.Add0(&EPGHeader::OnLeftClicked, this);
		m_right.ClickedEvent.Add0(&EPGHeader::OnRightClicked, this);
	}

	void EPGHeader::SetStart(CTime& value)
	{
		int minute = value.GetMinute();

		value = CTime(value.GetYear(), value.GetMonth(), value.GetDay(), value.GetHour(), minute / 30 * 30, 0);

		CTime t = value;

		setlocale(LC_TIME, ".ACP");
		m_date.Text = std::wstring(t.Format(L"%B %#d. %A"));
		setlocale(LC_TIME, "English");
		
		m_time1.Text = std::wstring(t.Format(L"%H:%M"));
		t += CTimeSpan(0, 0, 30, 0);
		m_time2.Text = std::wstring(t.Format(L"%H:%M"));
		t += CTimeSpan(0, 0, 30, 0);
		m_time3.Text = std::wstring(t.Format(L"%H:%M"));
	}

	bool EPGHeader::OnLeftClicked(Control* c)
	{
		Start = (CTime)Start - CTimeSpan(1, 0, 0, 0);

		return true;
	}

	bool EPGHeader::OnRightClicked(Control* c)
	{
		Start = (CTime)Start + CTimeSpan(1, 0, 0, 0);

		return true;
	}
}
