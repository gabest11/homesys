#include "stdafx.h"
#include "numpadbutton.h"

namespace DXUI
{
	NumpadButton::NumpadButton()
	{
	}

	Texture* NumpadButton::GetBackground(DeviceContext* dc, Vector4i& src, Vector4i& dst)
	{
		Texture* t = __super::GetBackground(dc, src, dst);

		if(t != NULL)
		{
			if(Focused)
			{
				dst = dst.inflate(14);
			}
		}

		return t;
	}
}