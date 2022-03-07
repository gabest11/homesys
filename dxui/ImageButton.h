#pragma once

#include "ImageButton.dxui.h"

namespace DXUI
{
	class ImageButton : public ImageButtonDXUI
	{
		bool OnPaintBackground(Control* c, DeviceContext* dc);

	public:
		ImageButton();
	};
}
