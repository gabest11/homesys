#pragma once

#include "numpad.dxui.h"

namespace DXUI
{
	class Numpad : public NumpadDXUI
	{
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnButtonPressed(Control* c);

	protected:
		clock_t m_start;
		int m_key;
		int m_pos;
		bool m_uppercase;

	public:
		Numpad();

		void Press(int i);

		Property<Control*> HintControl;
	};
}

