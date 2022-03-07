#pragma once

#include "imagebutton.h"

namespace DXUI
{
	class NumpadButton : public ImageButton
	{
	protected:
		Texture* GetBackground(DeviceContext* dc, Vector4i& src, Vector4i& dst);

	public:
		NumpadButton();
	};
}
