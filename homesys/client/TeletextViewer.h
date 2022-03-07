#pragma once

#include "TeletextViewer.dxui.h"

namespace DXUI
{
	class TeletextViewer : public TeletextViewerDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnKey(Control* c, const KeyEventParam& p);

		struct Teletext {Texture* tex; BYTE* buff; void Free();} m_teletext;

		bool JumpToLink(int index);

	public:
		TeletextViewer();
		virtual ~TeletextViewer();
	};
}
