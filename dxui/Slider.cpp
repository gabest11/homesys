#include "stdafx.h"
#include "slider.h"

namespace DXUI
{
	// Slider

	Slider::Slider()
		: m_drag(false)
	{
		Position.Set(&Slider::SetPosition, this);

		PaintBackgroundEvent.Add(&Slider::OnPaintBackground, this, true);
		m_thumb.PaintBackgroundEvent.Add(&Slider::OnPaintBackgroundThumb, this, true);
		KeyEvent.Add(&Slider::OnKey, this);
		MouseEvent.Add(&Slider::OnMouse, this);
		m_upleft.ClickedEvent.Add0(&Slider::OnClickedUpLeft, this);
		m_downright.ClickedEvent.Add0(&Slider::OnClickedDownRight, this);
		Rect.ChangedEvent.Add(&Slider::OnRectChanged, this);
		Margin.ChangedEvent.Add(&Slider::OnMarginChanged, this);
		Vertical.ChangedEvent.Add(&Slider::OnVerticalChanged, this);

		CanFocus = 0;
		Step = 0.1f;
		Position = 0;
	}

	void Slider::SetPosition(float& value)
	{
		value = max(0, min(1.0f, value));

		Vector2i p;

		if(Vertical)
		{
			p.x = 1;
			p.y = Margin + (int)(value * (Height - (m_thumb.Height + Margin * 2)) + 0.5f);
		}
		else
		{
			p.x = Margin + (int)(value * (Width - (m_thumb.Width + Margin * 2)) + 0.5f);
			p.y = 1;
		}

		m_thumb.Move(Vector4i(p.x, p.y, p.x + m_thumb.Width, p.y + m_thumb.Height));
	}

	void Slider::SetVertex(Vertex* v, const Vector4i& p, const Vector4i& t)
	{
		Vector4 a(p);
		Vector4 b(0.5f, 1.0f);
		
		v[0].p = a.xyxy(b);
		v[1].p = a.zyxy(b);
		v[2].p = a.xwxy(b);
		v[3].p = a.zwxy(b);

		a = Vector4(t);

		v[0].t = Vector2(a.x, a.y);
		v[1].t = Vector2(a.z, a.y);
		v[2].t = Vector2(a.x, a.w);
		v[3].t = Vector2(a.z, a.w);
	}

	bool Slider::OnPaintBackground(Control* c, DeviceContext* dc)
	{
		Texture* t = dc->GetTexture(BackgroundImage->c_str());

		if(t != NULL)
		{
			Vector2i size = t->GetSize();

			int margin = Margin;
			bool vertical = Vertical;

			Vector4i r;

			Vector4i tl = Vector4i(0, 0, size.x, margin);
			Vector4i br = Vector4i(0, size.y - margin, size.x, size.y);
			Vector4i mc = Vector4i(tl.left, tl.bottom, br.right, br.top);

			Vertex v[18];

			r = WindowRect;

			if(vertical) r.bottom = r.top + margin;
			else r.right = r.left + margin;

			SetVertex(&v[0], r, tl);

			r = WindowRect;

			if(vertical) {r.top = r.top + margin; r.bottom = r.bottom - margin;}
			else {r.left = r.left + margin; r.right = r.right - margin;}

			SetVertex(&v[6], r, mc);

			r = WindowRect;

			if(vertical) r.top = r.bottom - margin;
			else r.left = r.right - margin;

			SetVertex(&v[12], r, br);

			if(!vertical)
			{
				for(int i = 0; i < 18; i += 6)
				{
					Vector4 p = v[i + 0].p;
					v[i + 0].p = v[i + 2].p;
					v[i + 2].p = v[i + 3].p;
					v[i + 3].p = v[i + 1].p;
					v[i + 1].p = p;
				}
			}

			v[4] = v[1];
			v[5] = v[2];
			v[10] = v[7];
			v[11] = v[8];
			v[16] = v[13];
			v[17] = v[14];

			for(int i = 0; i < 18; i++)
			{
				v[i].p.z = 0.5f;
				v[i].p.w = 1.0f;
				v[i].t.x /= size.x;
				v[i].t.y /= size.y;
				v[i].c.x = 0xffffffff;
				v[i].c.y = 0;
			}

			dc->DrawTriangleList(v, 6, t);
		}

		return true;
	}

	bool Slider::OnPaintBackgroundThumb(Control* c, DeviceContext* dc)
	{
		Texture* t = dc->GetTexture(c->BackgroundImage->c_str());

		if(t != NULL)
		{
			Vector2i size = t->GetSize();

			Vertex v[6];

			SetVertex(&v[0], c->WindowRect, Vector4i(0, 0, size.x, size.y));

			if(!Vertical)
			{
				Vector4 p = v[0].p;
				v[0].p = v[2].p;
				v[2].p = v[3].p;
				v[3].p = v[1].p;
				v[1].p = p;
			}

			v[4] = v[1];
			v[5] = v[2];

			for(int i = 0; i < 6; i++)
			{
				v[i].p.z = 0.5f;
				v[i].p.w = 1.0f;
				v[i].t.x /= size.x;
				v[i].t.y /= size.y;
				v[i].c.x = 0xffffffff;
				v[i].c.y = 0;
			}

			dc->DrawTriangleList(v, 2, t);
		}

		return true;
	}

	bool Slider::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			bool vertical = Vertical;

			if(vertical && p.cmd.key == VK_UP || !vertical && p.cmd.key == VK_LEFT)
			{
				m_upleft.Click();

				return true;
			}

			if(vertical && p.cmd.key == VK_DOWN || !vertical && p.cmd.key == VK_RIGHT)
			{
				m_downright.Click();

				return true;
			}
		}

		return false;
	}
	
	bool Slider::OnMouse(Control* c, const MouseEventParam& p)
	{
		Vector4i m(Margin, 0);

		Vector4i r = ClientRect->deflate(Vertical ? m.yxyx() : m.xyxy());

		bool dragging = Captured;

		if(p.cmd == MouseEventParam::Move && dragging && !(p.flags & MK_LBUTTON))
		{
			Captured = false;
			
			return true;
		}

		if(p.cmd == MouseEventParam::Down && r.contains(p.point) || p.cmd == MouseEventParam::Move && dragging)
		{
			if(Vertical)
			{
				Position = (float)(p.point.y - Margin - m_thumb.Height / 2) / (r.height() - m_thumb.Height);
			}
			else
			{
				Position = (float)(p.point.x - Margin - m_thumb.Width / 2) / (r.width() - m_thumb.Width);
			}

			if(MapClientRect(&m_thumb).contains(p.point))
			{
				if(!dragging) Captured = true;
			}

			ScrollEvent(this, Position);

			return true;
		}

		return false;
	}

	bool Slider::OnClickedUpLeft(Control* c)
	{
		float f = Position - Step;
		if(Position == 0) OverflowEvent(this, f);
		else Position = f;
		ScrollEvent(this, Position);
		return true;
	}

	bool Slider::OnClickedDownRight(Control* c)
	{
		float f = Position + Step;
		if(Position == 1.0f) OverflowEvent(this, f);
		else Position = f;
		ScrollEvent(this, Position);
		return true;
	}

	bool Slider::OnRectChanged(Control* c, const Vector4i& value)
	{
		UpdateLayout();

		return true;
	}

	bool Slider::OnMarginChanged(Control* c, int margin)
	{
		UpdateLayout();
		
		return true;
	}

	bool Slider::OnVerticalChanged(Control* c, bool vertical)
	{
		UpdateLayout();

		return true;
	}

	void Slider::UpdateLayout()
	{
		if(Vertical)
		{
			m_upleft.Rect = Vector4i(0, 0, Width, Margin);
			m_upleft.AnchorRect = Vector4i(Anchor::Left, Anchor::Top, Anchor::Right, Anchor::Top);
			m_downright.Rect = Vector4i(0, Height - Margin, Width, Height);
			m_downright.AnchorRect = Vector4i(Anchor::Left, Anchor::Bottom, Anchor::Right, Anchor::Bottom);
			m_thumb.Rect = Vector4i(1, 1, Width - 1, Width - 1);
			m_thumb.AnchorRect = Vector4i(Anchor::Left, Anchor::Center, Anchor::Right, Anchor::Center);
		}
		else
		{
			m_upleft.Rect = Vector4i(0, 0, Margin, Height);
			m_upleft.AnchorRect = Vector4i(Anchor::Left, Anchor::Top, Anchor::Left, Anchor::Bottom);
			m_downright.Rect = Vector4i(Width - Margin, 0, Width, Height);
			m_downright.AnchorRect = Vector4i(Anchor::Right, Anchor::Top, Anchor::Right, Anchor::Bottom);
			m_thumb.Rect = Vector4i(1, 1, Height - 1, Height - 1);
			m_thumb.AnchorRect = Vector4i(Anchor::Center, Anchor::Top, Anchor::Center, Anchor::Bottom);
		}

		Position = Position;
	}
}