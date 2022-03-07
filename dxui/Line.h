#pragma once

#include "control.h"

namespace DXUI
{
	class Line : public Control
	{
		bool OnPaintForeground(Control* c, DeviceContext* dc);

	public:
		Line();

		Property<DWORD> Color1;
		Property<DWORD> Color2;
	};
}
