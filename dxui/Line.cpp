#include "stdafx.h"
#include "Line.h"

namespace DXUI
{
	Line::Line()
	{
		PaintForegroundEvent.Add(&Line::OnPaintForeground, this);
	}

	bool Line::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Vector4i r = WindowRect;

		dc->Draw(r.tl, r.br, Color1, Color2);

		return true;
	}
}