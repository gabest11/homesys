#pragma once

#include "EPGHeader.dxui.h"

namespace DXUI
{
	class EPGHeader : public EPGHeaderDXUI
	{
		void SetStart(CTime& value);

		bool OnLeftClicked(Control* c);
		bool OnRightClicked(Control* c);

	public:
		EPGHeader();
	};
}
