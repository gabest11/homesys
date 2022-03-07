#pragma once

#include "Banner.dxui.h"

namespace DXUI
{
	class Banner : public BannerDXUI
	{
		bool OnPaintBackground(Control* c, DeviceContext* dc);

		clock_t m_time;

	public:
		Banner();

		Property<int> Timeout;
	};
}
