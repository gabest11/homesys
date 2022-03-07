#include "stdafx.h"
#include "Banner.h"

namespace DXUI
{
	Banner::Banner()
		: m_time(0)
	{
		PaintBackgroundEvent.Add(&Banner::OnPaintBackground, this);

		Timeout = 5 * 60 * 1000;
	}

	bool Banner::OnPaintBackground(Control* c, DeviceContext* dc)
	{
		clock_t now = clock();

		if(now - m_time > Timeout)
		{
			m_time = now;

			dc->GetTexture(BackgroundImage->c_str(), false, true);

			wprintf(L"* %s\n", BackgroundImage->c_str());
		}

		return true;
	}
}