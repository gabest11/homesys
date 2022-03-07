#include "stdafx.h"
#include "recordingsdialog.h"
#include "client.h"

namespace DXUI
{
	RecordingsDialog::RecordingsDialog()
	{
		auto f = [&] (Control* c) -> bool {return Dialog::OnClose(c);};

		m_play.ClickedEvent.Add0(f);
		m_edit.ClickedEvent.Add0(f);
		m_delete.ClickedEvent.Add0(f);
		m_modify.ClickedEvent.Add0(f);
	}

	bool RecordingsDialog::Create(const Vector4i& rect, Control* parent)
	{
		if(!__super::Create(rect, parent))
			return false;

		int buttons = 4;
		int width = 300;
		int height = 40;
		int gap = 10;
		int totalheight = height * buttons + gap * (buttons - 1);
		int x = (Width - width) / 2;
		int y = (Height - totalheight) / 2;

		Vector4i o = Vector4i(0, height + gap).xyxy();

		m_container.Create(Vector4i(x, y, x + width, y + totalheight), this);
		m_container.AnchorRect = Vector4i(Anchor::Center, Anchor::Center, Anchor::Center, Anchor::Center);

		Vector4i r(0, 0, width, height);

		m_play.Create(r, &m_container);
		m_play.Text = Control::GetString("PLAY");
		m_play.TextAlign = Align::Center | Align::Middle;

		r += o;

		m_edit.Create(r, &m_container);
		m_edit.Text = Control::GetString("CUT");
		m_edit.TextAlign = Align::Center | Align::Middle;

		r += o;

		m_modify.Create(r, &m_container);
		m_modify.Text = Control::GetString("MODIFY");
		m_modify.TextAlign = Align::Center | Align::Middle;

		r += o;

		m_delete.Create(r, &m_container);
		m_delete.Text = Control::GetString("DELETE");
		m_delete.TextAlign = Align::Center | Align::Middle;

		r += o;

		return true;
	}
}
