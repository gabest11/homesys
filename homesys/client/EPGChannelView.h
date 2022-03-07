#pragma once

#include "EPGChannelList.h"
#include "ChannelTextureFactory.h"

namespace DXUI
{
	class EPGChannelView : public Control
	{
		bool OnListCurrentDateChanged(Control* c, CTime date);
		bool OnPaintForeground(Control* c, DeviceContext* dc);

	public:
		EPGChannelView();

		bool Create(const Vector4i& rect, Control* parent);

		EPGChannelList m_list;
		ChannelTextureFactory m_ctf;
	};
}
