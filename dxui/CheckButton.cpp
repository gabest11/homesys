#include "stdafx.h"
#include "CheckButton.h"

namespace DXUI
{
	CheckButton::CheckButton()
	{
		PaintBackgroundEvent.Add(&CheckButton::OnPaintBackground, this, true);
		ClickedEvent.Add0(&CheckButton::OnClicked, this);
		Checked.ChangedEvent.Add(&CheckButton::OnChecked, this);
	}

	bool CheckButton::OnPaintBackground(Control* c, DeviceContext* dc)
	{
		Vector4i r = WindowRect;

		r.right = r.left + TextMarginLeft - 5;

		Texture* on = dc->GetTexture(CheckedImage->c_str());
		Texture* off = dc->GetTexture((Focused ? FocusedImage : BackgroundImage)->c_str());

		if(on != NULL && off != NULL)
		{
			dc->Draw(off, r);
			dc->Draw(on, r, m_anim);
		}

		return true;
	}

	bool CheckButton::OnClicked(Control* c)
	{
		Checked = !Checked;

		return true;
	}

	bool CheckButton::OnChecked(Control* c, bool value)
	{
		m_anim.Set(m_anim, value ? 1.0f : 0, 200);

		return true;
	}
}