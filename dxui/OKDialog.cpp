#include "stdafx.h"
#include "OKDialog.h"

namespace DXUI
{
	OKDialog::OKDialog()
	{
		m_ok.ClickedEvent.Add0([&] (Control* c) -> bool {return OnClose(c);});
	}

	bool OKDialog::Create(const Vector4i& rect, Control* parent)
	{
		if(!__super::Create(rect, parent))
			return false;

		m_message.Create(Vector4i(40, 40, Width - 40, Height - 100), this);
		m_message.TextHeight = 30;
		m_message.TextAlign = Align::Left | Align::Top;
		m_message.TextWrap = true;
		m_message.AnchorRect = Vector4i(Anchor::Left, Anchor::Top, Anchor::Right, Anchor::Bottom);

		Vector4i r = ClientRect;
		Vector2i c((r.x + r.z) / 2, (r.y + r.w) / 2);

		m_ok.Create(Vector4i(c.x - 80, Height - 80, c.x + 80, Height - 40), this);
		m_ok.Text = L"OK";
		m_ok.TextAlign = Align::Center | Align::Middle;
		m_ok.AnchorRect = Vector4i(Anchor::Center, Anchor::Bottom, Anchor::Center, Anchor::Bottom);

		return true;
	}
}
