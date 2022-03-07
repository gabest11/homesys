#include "stdafx.h"
#include "EPGChannelView.h"

namespace DXUI
{
	EPGChannelView::EPGChannelView()
	{
		PaintForegroundEvent.Add(&EPGChannelView::OnPaintForeground, this, true);
		m_list.CurrentDate.ChangedEvent.Add(&EPGChannelView::OnListCurrentDateChanged, this);

		TextHeight = 30;
		TextAlign = Align::Right | Align::Middle;
	}

	bool EPGChannelView::Create(const Vector4i& rect, Control* parent)
	{
		if(!__super::Create(rect, parent))
			return false;

		m_list.Create(Vector4i(0, TextHeight + 5, Width, Height), this);
		m_list.AnchorRect = Vector4i(Anchor::Left, Anchor::Top, Anchor::Right, Anchor::Bottom);
		m_list.ItemHeight = m_list.Height / 10;
		m_list.TextHeight = m_list.ItemHeight - 20;

		m_ctf.Create(Vector4i::zero(), this);

		return true;
	}

	bool EPGChannelView::OnListCurrentDateChanged(Control* c, CTime date)
	{
		setlocale(LC_TIME, ".ACP");
		Text = std::wstring(date.Format(L"%Y. %B %#d."));
		setlocale(LC_TIME, "English");

		return true;
	}

	bool EPGChannelView::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Homesys::Channel* channel = m_list.CurrentChannel;

		if(channel != NULL)
		{
			Vector4i r(0, 0, TextHeight * 2, TextHeight);

			dc->Draw(C2W(r), 0xffffffff);

			r = r.deflate(1);

			Texture* t = m_ctf.GetTexture(channel->id, dc);

			if(t != NULL)
			{
				dc->Draw(t, C2W(r));
			}

			r.left = r.right + 5;
			r.right = Width;

			dc->Draw(Text->c_str(), C2W(r));
		}

		return true;
	}
}
