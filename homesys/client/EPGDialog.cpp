#include "stdafx.h"
#include "EPGDialog.h"
#include "client.h"

namespace DXUI
{
	EPGDialog::EPGDialog()
	{
		auto f = [&] (Control* c) -> bool {return Dialog::OnClose(c);};

		m_switch.ClickedEvent.Add0(f);
		m_details.ClickedEvent.Add0(f);
		m_record.ClickedEvent.Add0(f);

		m_switch.ClickedEvent.Chain(g_env->PresetSwitchEvent);
		m_details.ClickedEvent.Chain(g_env->ChannelDetailsEvent);
		m_record.ClickedEvent.Chain(g_env->RecordClickedEvent);
	}

	bool EPGDialog::Create(const Vector4i& rect, Control* parent)
	{
		if(!__super::Create(rect, parent))
			return false;

		int buttons = 3;
		int width = 300;
		int height = 40;
		int gap = 10;
		int totalheight = height * buttons + gap * (buttons - 1);
		int x = (Width - width) / 2;
		int y = (Height - totalheight) / 2;

		Vector4i o = Vector4i(0, height + gap).xyxy();

		m_container.Create(Vector4i(x, y, x + width, y + totalheight), this);
		m_container.AnchorRect = Vector4i(Anchor::Center, Anchor::Center, Anchor::Center, Anchor::Center);

		Vector4i r = Vector4i(0, 0, width, height);

		m_switch.Create(r, &m_container);
		m_switch.Text = Control::GetString("CHANNEL_SWITCH_TO");
		m_switch.TextAlign = Align::Center | Align::Middle;

		r += o;

		m_details.Create(r, &m_container);
		m_details.Text = Control::GetString("CHANNEL_EPG");
		m_details.TextAlign = Align::Center | Align::Middle;

		r += o;

		m_record.Create(r, &m_container);
		m_record.Text = Control::GetString("CHANNEL_SCHEDULE");
		m_record.TextAlign = Align::Center | Align::Middle;

		r += o;

		return true;
	}
}
