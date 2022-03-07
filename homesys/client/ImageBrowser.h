#pragma once

#include "ImageBrowser.dxui.h"

namespace DXUI
{
	class ImageBrowser : public ImageBrowserDXUI
	{
		bool OnTableClicked(Control* c, int index);
		bool OnPaintTableItem(Control* c, PaintItemParam* p);

	public:
		ImageBrowser();
	};
}
