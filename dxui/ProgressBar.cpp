#include "stdafx.h"
#include "progressbar.h"

namespace DXUI
{
	ProgressBar::ProgressBar()
		: m_drag(false)
	{
		Start.Set([&] (float& value) {value = max(0, min(Stop, value));});
		Stop.Set([&] (float& value) {value = max(Start, min(1.0f, value));});
		Position.Set([&] (float& value) {value = max(0, min(1.0f, value));});

		PaintBackgroundEvent.Add(&ProgressBar::OnPaintBackground, this, true);
		PaintForegroundEvent.Add(&ProgressBar::OnPaintForeground, this);
		MouseEvent.Add(&ProgressBar::OnMouse, this);
	}

	bool ProgressBar::OnPaintBackground(Control* c, DeviceContext* dc)
	{
		Texture* t = dc->GetTexture(BackgroundImage->c_str());

		if(t != NULL)
		{
			Vector2i size = t->GetSize();

			DWORD c1 = 0xffffffff;
			DWORD c2 = FillColor & 0xffffff;

			Vector4i r = WindowRect;

			Vector4 a(r);
			Vector4 b = a;
			Vector4 c = a;

			a.right = a.left + (a.right - a.left) * Start;
			b.left = b.left + (b.right - b.left) * Stop;
			c.left = a.right;
			c.right = b.left;

			Vector4 q(0.5f, 1.0f);

			Vector4 at = Vector4(0.0f, 1.0f).xxyy();
			Vector4 bt = at;
			Vector4 ct = at;

			at.right = Start;
			bt.left = Stop;
			ct.left = at.right;
			ct.right = bt.left;

			Vertex v[18] = 
			{
				{a.xyxy(q), Vector2(at.x, at.y), Vector2i(c1, 0)},
				{a.zyxy(q), Vector2(at.z, at.y), Vector2i(c1, 0)},
				{a.xwxy(q), Vector2(at.x, at.w), Vector2i(c1, 0)},
				{a.zwxy(q), Vector2(at.z, at.w), Vector2i(c1, 0)},
				{a.zyxy(q), Vector2(at.z, at.y), Vector2i(c1, 0)},
				{a.xwxy(q), Vector2(at.x, at.w), Vector2i(c1, 0)},
				{b.xyxy(q), Vector2(bt.x, bt.y), Vector2i(c1, 0)},
				{b.zyxy(q), Vector2(bt.z, bt.y), Vector2i(c1, 0)},
				{b.xwxy(q), Vector2(bt.x, bt.w), Vector2i(c1, 0)},
				{b.zwxy(q), Vector2(bt.z, bt.w), Vector2i(c1, 0)},
				{b.zyxy(q), Vector2(bt.z, bt.y), Vector2i(c1, 0)},
				{b.xwxy(q), Vector2(bt.x, bt.w), Vector2i(c1, 0)},
				{c.xyxy(q), Vector2(ct.x, ct.y), Vector2i(c1, c2)},
				{c.zyxy(q), Vector2(ct.z, ct.y), Vector2i(c1, c2)},
				{c.xwxy(q), Vector2(ct.x, ct.w), Vector2i(c1, c2)},
				{c.zwxy(q), Vector2(ct.z, ct.w), Vector2i(c1, c2)},
				{c.zyxy(q), Vector2(ct.z, ct.y), Vector2i(c1, c2)},
				{c.xwxy(q), Vector2(ct.x, ct.w), Vector2i(c1, c2)},
			};

			dc->DrawTriangleList(v, 6, t);
		}

		return true;
	}

	bool ProgressBar::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		if(Indicator)
		{
			Vector4i margin = BackgroundMargin;

			Texture* t = dc->GetTexture(IndicatorImage->c_str());

			if(t != NULL)
			{
				int w = Width - margin.left - margin.right;
				int x = (int)(margin.left + Position * w + 0.5f);

				Vector2i s = t->GetSize();
				Vector4i r(x - s.x / 2, 0, x + s.x / 2, Height);

				dc->Draw(t, C2W(r), Alpha);
			}
		}

		return true;
	}

	bool ProgressBar::OnMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::Move && Captured && !(p.flags & MK_LBUTTON))
		{
			Captured = false;

			return false;
		}

		if(p.cmd == MouseEventParam::Up && Captured)
		{
			Captured = false;

			DragReleasedEvent(this, Position);

			return true;
		}

		if(p.cmd == MouseEventParam::Down && Hovering || p.cmd == MouseEventParam::Move && Captured)
		{
			Vector4i margin = BackgroundMargin;
			
			if(p.point.x < margin.left) 
			{
				Position = 0;
			}
			else if(p.point.x > Width - margin.right) 
			{
				Position = 1.0f;
			}
			else 
			{
				Position = (float)(p.point.x - margin.left) / (Width - margin.left - margin.right);
			}

			Captured = true;
			
			return true;
		}

		return false;
	}
}