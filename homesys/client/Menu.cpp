#include "stdafx.h"
#include "Menu.h"
#include "client.h"

namespace DXUI
{
	Menu::Menu()
		: m_anim(true)
		, m_anim2(true)
	{
		m_anim.Set(0, 1, 10000);
		m_anim.Add(0, 0);
		m_anim.Add(0, 5000);
		m_anim2.Set(0, 1, 200000);

		KeyEvent.Add(&Menu::OnKey, this);
		PaintBackgroundEvent.Add(&Menu::OnPaintBackground, this, true);
		// m_logo.PaintBackgroundEvent.Add(&Menu::OnPaintLogoBackground, this);
		m_logo.Alpha.Get([&] (float& value) {value = 1.0f - m_hotlink.Interactive;});
		m_logo.Visible.Get([&] (bool& value) {value = m_hotlink.Interactive < 1;});
		m_homesys_logo.Alpha.Get([&] (float& value) {value = 1.0f - m_hotlink.Interactive;});
		m_homesys_logo.Visible.Get([&] (bool& value) {value = m_hotlink.Interactive < 1;});
	}

	bool Menu::Message(const KeyEventParam& e)
	{
		m_hotlink.Interactive = 0;

		return __super::Message(e);
	}

	bool Menu::Message(const MouseEventParam& p)
	{
		m_hotlink.Interactive = 0;

		return __super::Message(p);
	}

	bool Menu::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.remote == RemoteKey::Details || p.cmd.key == VK_F12)
			{
				if(m_hotlink.CanFocus) 
				{
					m_hotlink.Focused = true;

					return true;
				}
			}
		}

		return false;
	}

	bool Menu::OnPaintBackground(Control* c, DeviceContext* dc)
	{
		Vector4i r = WindowRect;

		bool hasbkg = false;

		if(Texture* t = dc->GetTexture(L"MenuBackground.jpg"))
		{
			dc->Draw(t, r);

			return true;
		}

		r.bottom = 92;

		dc->Draw(r, Color::Black);

		r = WindowRect;

		r.top = 92;

		DWORD color1 = Color::MenuBackground1; // (0 << 16) | (12 << 8) | (55) | (255 << 24);
		DWORD color2 = Color::MenuBackground2; // (24 << 16) | (35 << 8) | (74) | (255 << 24);
		DWORD color3 = Color::MenuBackground3; // (0 << 16) | (26 << 8) | (94) | (255 << 24);

		dc->Draw(r, color1);

		Texture* t = dc->GetTexture(L"Highlight.png");

		PixelShader* ps = dc->GetPixelShader(L"MenuBackground.psh");

		if(t != NULL && ps != NULL)  // t?
		{
			int cx = (r.x + r.z) / 2;
			int h = r.height();
			int left = cx - h / 2;
			int right = cx + h / 2;

			Vector2i color((int)color1, (int)color3);

			Vertex v[6] = 
			{
				{Vector4(left, r.top), Vector2(0, 0), color},
				{Vector4(right, r.top), Vector2(1, 0), color},
				{Vector4(left, r.bottom), Vector2(0, 1), color},
				{Vector4(right, r.bottom), Vector2(1, 1), color},
			};

			v[4] = v[1];
			v[5] = v[2];

			for(int i = 0; i < 6; i++)
			{
				v[i].p.z = 0.5f;
				v[i].p.w = 1.0f;
			}

			dc->DrawTriangleList(v, 2, t, ps);
		}

		{
			Vertex v[6] = 
			{
				{Vector4(r.left, r.top), Vector2(0, 0), Vector2i((int)color2, 0)},
				{Vector4(r.right, r.top), Vector2(1, 0), Vector2i((int)color2, 0)},
				{Vector4(r.left, r.top + 30), Vector2(0, 1), Vector2i((int)color1, 0)},
				{Vector4(r.right, r.top + 30), Vector2(1, 1), Vector2i((int)color1, 0)},
			};

			v[4] = v[1];
			v[5] = v[2];

			for(int i = 0; i < 6; i++)
			{
				v[i].p.z = 0.5f;
				v[i].p.w = 1.0f;
			}

			dc->DrawTriangleList(v, 2, NULL);
		}

		if(t != NULL)
		{
			float f = m_anim;

			if(f > 0)
			{
				int p = (int)(350 + f * (Width - 700) + 0.5f);

				Vector4i r(p - 30, 90, p + 30, 93);

				if(f >= 0.5f) f = 1.0f - f;
				
				f *= 3;

				dc->Draw(t, r, f);
			}
		}

		return true;
	}

	bool Menu::OnPaintLogoBackground(Control* c, DeviceContext* dc)
	{
		if(Texture* t = dc->GetTexture(c->BackgroundImage->c_str()))
		{
			Vector4i r = c->WindowRect;
			
			r += Vector4i(0, 90 - r.top).xyxy();

			Vertex v[6] = 
			{
				{Vector4(r.left - 100, r.bottom), Vector2(0, 0), Vector2i(0x00ffffff, 0)},
				{Vector4(r.right + 100, r.bottom), Vector2(1, 0), Vector2i(0x00ffffff, 0)},
				{Vector4(r.left, r.top), Vector2(0, 1), Vector2i(0x40ffffff, 0)},
				{Vector4(r.right, r.top), Vector2(1, 1), Vector2i(0x40ffffff, 0)},
			};

			v[4] = v[1];
			v[5] = v[2];

			for(int i = 0; i < 6; i++)
			{
				v[i].p.z = 0.5f;
				v[i].p.w = 1.0f;
			}

			dc->DrawTriangleList(v, 2, t);
		}

		return true;
	}
}