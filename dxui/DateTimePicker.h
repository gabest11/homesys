#pragma once

#include "DateTimePicker.dxui.h"

namespace DXUI
{
	class DateTimePicker : public DateTimePickerDXUI
	{
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);

		int m_pos;

		void Adjust(int dir);

	public:
		DateTimePicker();
	};
}

