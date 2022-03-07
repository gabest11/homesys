#include "stdafx.h"
#include "Video.h"
#include "Player.h"
#include "client.h"
#include "../../DirectShow/Filters.h"

namespace DXUI
{
	Video::Video()
	{
		Focused.ChangedEvent.Add([&] (Control* c, bool focused) -> bool {m_anim.Set(m_anim, focused ? 1 : 0, 300); return true;});
		PaintForegroundEvent.Add(&Video::OnPaintForeground, this);
		ClickedEvent.Chain(g_env->VideoClickedEvent);

		MirrorEffect = false;
		ZoomEffect = true;
	}

	bool Video::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Vector4i margin(14, 14, 16, 14);

		Vector4i src;
		Vector4i dst = C2W(dc->FitRect(Vector2i(4, 3), ClientRect));
		Vector2i ar;

		CComPtr<IDirect3DTexture9> video;

		Texture* t = NULL;

		bool del = false;
		bool recycle = false;

		int index = 0;

		for(auto i = g_env->players.begin(); i != g_env->players.end(); i++)
		{
			if(index++ == Selected)
			{
				CComPtr<IGenericPlayer> player = *i;

				if(SUCCEEDED(player->GetVideoTexture(&video, ar, src)))
				{
					dst = C2W(dc->FitRect(ar, ClientRect));

					t = new Texture9(video);

					del = true;
				}

				if(t == NULL && player->HasVideo() == S_FALSE)
				{
					t = g_env->GetAudioTexture(player, dc);

					if(t != NULL)
					{
						src = Vector4i(0, 0, t->GetWidth(), t->GetHeight());
					}

					recycle = true;
				}

				break;
			}
		}

		Texture* f1 = dc->GetTexture(L"ListSelectedBackground.png");
		Texture* f2 = dc->GetTexture(L"ListSelectedInactiveBackground.png");

		if(f1 != NULL && f2 != NULL)
		{
			if(ZoomEffect)
			{
				float f = m_anim;

				dst = dst.inflate(Vector4i(Vector4(margin) * f));

				dc->Draw9x9(f2, dst, margin, 1.0f - f);
				dc->Draw9x9(f1, dst, margin, f);
			}
			else
			{
				dc->Draw9x9(f1, dst, margin);
			}
		}

		if(ZoomEffect)
		{
			dst = dst.deflate(margin);
		}

		if(t != NULL)
		{
			dc->Draw(t, dst, src);

			if(MirrorEffect)
			{
				Vector4i r = dst;
				Vector2i c((r.left + r.right) / 2, r.bottom);
			
				r -= Vector4i(c).xyxy();

				Vector4 p(r);
				Vector4 q(0.0f, 1.0f);

				Vertex v[6] = 
				{
					{p.xyxy(q), Vector2(0, 0), Vector2i(0x00ffffff, 0)},
					{p.zyxy(q), Vector2(1, 0), Vector2i(0x00ffffff, 0)},
					{p.xwxy(q), Vector2(0, 1), Vector2i(0x80ffffff, 0)},
					{p.zwxy(q), Vector2(1, 1), Vector2i(0x80ffffff, 0)},
				};

				float ca = cos(110.0f * M_PI / 180);
				float sa = sin(110.0f * M_PI / 180);

				for(int i = 0; i < 4; i++)
				{
					float x = v[i].p.x;
					float y = ca * v[i].p.y - sa * v[i].p.z;
					float z = sa * v[i].p.y + ca * v[i].p.z;

					v[i].p.z = z / 1000 + 0.5f;
					v[i].p.w = 1.0f / v[i].p.z;
					v[i].p.x = x / (v[i].p.z * 2) + c.x;
					v[i].p.y = y / (v[i].p.z * 2) + c.y + 10;
				}

				v[4] = v[1];
				v[5] = v[2];

				dc->DrawTriangleList(v, 2, t, dc->GetPixelShader(L"VideoMirror.psh"));
			}
		}

		if(t != NULL)
		{
			if(del)
			{
				delete t;
			}
			else if(recycle) 
			{
				dc->GetDevice()->Recycle(t);
			}
		}

		return true;
	}
}