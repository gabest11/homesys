#include "stdafx.h"
#include "ImageButton.h"

namespace DXUI
{
	ImageButton::ImageButton()
	{
		PaintBackgroundEvent.Add(&ImageButton::OnPaintBackground, this);
	}

	bool ImageButton::OnPaintBackground(Control* c, DeviceContext* dc)
	{
		if(Captured)
		{
			// TODO
		}

		return true;
	}
}