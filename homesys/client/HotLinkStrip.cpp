#include "stdafx.h"
#include "HotLinkStrip.h"
#include "client.h"

#define ITEM_DELAY 5000

namespace DXUI
{
	HotLinkStrip::HotLinkStrip()
		: m_anim(true)
		, m_current(INT_MIN)
		, m_target(INT_MIN)
	{
		ActivatedEvent.Add(&HotLinkStrip::OnActivated, this);
		KeyEvent.Add(&HotLinkStrip::OnKey, this);
		MouseEvent.Add(&HotLinkStrip::OnMouse, this);
		PaintForegroundEvent.Add(&HotLinkStrip::OnPaintForeground, this);

		CanFocus.Get([&] (int& value) 
		{
			int i = m_anim.GetSegment(); 
			
			value = i > 0 && i < 3 && !g_env->hotlinks.empty() ? 1 : 0;
		});

		Interactive.Get([&] (float& value) {value = !g_env->hotlinks.empty() ? m_anim : 0;});
		Interactive.Set([&] (float& value) {if(!Focused && m_anim.GetSegment() == 0) m_anim.SetSegment(0);});

		Focused.ChangedEvent.Add([&] (Control* c, bool value) -> bool 
		{
			if(value) 
			{
				if(m_anim.GetSegment() == 0) 
				{
					m_anim.SetSegment(1);
				}

				Step(0);
			}
			else 
			{
				m_anim.SetSegment(3);
			} 

			return true;
		});
	}

	void HotLinkStrip::Step(int dir)
	{
		if(m_target == INT_MIN)
		{
			m_target = (m_current / ITEM_DELAY + dir) * ITEM_DELAY;
		}
		else
		{
			m_target += dir * ITEM_DELAY;
		}
	}

	void HotLinkStrip::Open()
	{
		if(m_target == INT_MIN) return;

		int index = ((m_current + ITEM_DELAY * 1000) / ITEM_DELAY) % g_env->hotlinks.size();
		
		for(auto i = g_env->hotlinks.begin(); i != g_env->hotlinks.end(); i++)
		{
			HotLink* hl = *i;

			if(index-- == 0)
			{
				if(HotLinkProgram* p = dynamic_cast<HotLinkProgram*>(hl))
				{
					UserData = (UINT_PTR)p->program.id;

					g_env->RecordClickedEvent(this);
				}

				break;
			}
		}
	}

	void HotLinkStrip::PaintHotLink(HotLink* hl, const Vector4& r2, bool selected, DeviceContext* dc)
	{
		Texture* image = NULL;
		Texture* icon = NULL;
		Texture* icon2 = NULL;
		std::wstring text;
		std::wstring label[2];

		label[1] = hl->username;

		if(HotLinkProgram* p = dynamic_cast<HotLinkProgram*>(hl))
		{
			Homesys::Channel channel;

			image = dc->GetTexture(g_env->FormatMoviePictureUrl(p->program.episode.movie.id).c_str());
			icon = m_ctf.GetTexture(p->program.channelId, dc, &channel);
			text = p->program.start.Format(L"%H:%M");
			label[0] = p->program.episode.movie.title; 
			setlocale(LC_TIME, ".ACP");
			label[1] = p->program.start.Format(L"%Y. %b %#d. %#H:%M");
			setlocale(LC_TIME, "English");
			if(!channel.name.empty()) label[0] += L" (" + channel.name + L")";
		}

		if(image == NULL)
		{
			image = icon;
			icon = NULL;
		}

		if(hl->message)
		{
			icon2 = dc->GetTexture(L"user_48.png");
		}

		if(selected)
		{
			int w = Width;
			int h = Height;
			/*
			if(Texture* t = dc->GetTexture(L"csik2.png"))
			{
				dc->Draw9x9(t, Vector4i(0, h - 39, w, h - 2), Vector4i(2, 15, 22, 10));
			}
			
			if(Texture* t = dc->GetTexture(L"csik1.png"))
			{
				dc->Draw9x9(t, Vector4i(r2).inflate(5), Vector4i(14));
			}
			*/

			std::wstring s = dc->TextFace;

			dc->TextFace = FontType::Bold;
			dc->TextHeight = 25;
			dc->TextStyle = TextStyles::Bold;

			dc->TextAlign = Align::Left | Align::Middle;
			dc->TextColor = (Color::Text1 & 0xffffff) | ((DWORD)(m_anim * 0xff) << 24);

			Vector4i r(h * 4 / 3 + 10, h - 39, w - 10, h);

			dc->Draw(label[0].c_str(), C2W(r)); 

			dc->TextAlign = Align::Right | Align::Middle;
			dc->TextColor = (Color::Text2 & 0xffffff) | ((DWORD)(m_anim * 0xff) << 24);

			dc->Draw(label[1].c_str(), C2W(r));

			dc->TextFace = s;
		}

		DWORD c = 0xffffffff;

		if(image != NULL)
		{
			Vector4 r = r2;

			r = r.fit(image->GetWidth(), image->GetHeight());

			r += Vector4(3, 6, -3, -6);

			Vertex v[6] = 
			{
				{Vector4(r.x, r.y), Vector2(0, 0), Vector2i(c, 0)},
				{Vector4(r.z, r.y), Vector2(1, 0), Vector2i(c, 0)},
				{Vector4(r.x, r.w), Vector2(0, 1), Vector2i(c, 0)},
				{Vector4(r.z, r.w), Vector2(1, 1), Vector2i(c, 0)},
			};

			v[4] = v[1];
			v[5] = v[2];

			for(int i = 0; i < 6; i++)
			{
				v[i].p.z = 0.5f;
				v[i].p.w = 1.0f;
			}

			dc->DrawTriangleList(v, 2, image);

			// dc->Draw(image, Vector4i(x, 0, x + item_width, 92).deflate(5));
		}

		if(icon != NULL)
		{
			Vector4 r = r2;

			r.left = r.right - 32;
			r.bottom = r.top + 32;

			r += Vector4(-4.0f, 4.0f).xyxy();

			r = Vector4(DeviceContext::FitRect(icon->GetSize(), Vector4i(r)));

			Vertex v[6] = 
			{
				{Vector4(r.x, r.y), Vector2(0, 0), Vector2i(c, 0)},
				{Vector4(r.z, r.y), Vector2(1, 0), Vector2i(c, 0)},
				{Vector4(r.x, r.w), Vector2(0, 1), Vector2i(c, 0)},
				{Vector4(r.z, r.w), Vector2(1, 1), Vector2i(c, 0)},
			};

			v[4] = v[1];
			v[5] = v[2];

			for(int i = 0; i < 6; i++)
			{
				v[i].p.z = 0.5f;
				v[i].p.w = 1.0f;
			}

			dc->DrawTriangleList(v, 2, icon);
		}

		if(icon2 != NULL)
		{
			Vector4 r = r2;

			r.right = r.left + 32;
			r.bottom = r.top + 32;

			r += Vector4(4.0f, 4.0f).xyxy();

			r = Vector4(DeviceContext::FitRect(icon2->GetSize(), Vector4i(r)));

			Vertex v[6] = 
			{
				{Vector4(r.x, r.y), Vector2(0, 0), Vector2i(c, 0)},
				{Vector4(r.z, r.y), Vector2(1, 0), Vector2i(c, 0)},
				{Vector4(r.x, r.w), Vector2(0, 1), Vector2i(c, 0)},
				{Vector4(r.z, r.w), Vector2(1, 1), Vector2i(c, 0)},
			};

			v[4] = v[1];
			v[5] = v[2];

			for(int i = 0; i < 6; i++)
			{
				v[i].p.z = 0.5f;
				v[i].p.w = 1.0f;
			}

			dc->DrawTriangleList(v, 2, icon2);
		}

		if(!text.empty())
		{
			Vector4i r = Vector4i(r2);
			
			r -= Vector4i(0, 4).xyxy();

			dc->TextAlign = Align::Center | Align::Bottom;
			dc->TextColor = Color::Text1;
			dc->TextHeight = r.height() / 4;
			dc->TextStyle = TextStyles::Bold | TextStyles::Outline;
					
			dc->Draw(text.c_str(), r);
		}
	}

	void HotLinkStrip::GetInfoRect(int i, Vector4i& r)
	{
		static Vector4i margin[] = 
		{
			Vector4i(10, 10, 10, 5),
			Vector4i(10, 5, 10, 10),
		};

		r = WindowRect;

		int h = Height;

		if(i & 1)
		{
			r.top = h / 2;
			r.bottom = h;
		}
		else
		{
			r.bottom = h / 2;
		}

		r = r.deflate(margin[i & 1]);
	}

	bool HotLinkStrip::OnActivated(Control* c, int dir)
	{
		Util::Config cfg(L"Homesys", L"HotLink");

		m_anim.RemoveAll();

		int t = cfg.GetInt(L"Timeout", 0);

		if(t > 0)
		{
			m_anim.Set(0, 0, t * 1000);
			m_anim.Add(1, 1000);
			m_anim.Add(1, 3600000);
			m_anim.Add(0, 1000);
		}

		return true;
	}

	bool HotLinkStrip::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			int dir = 0;

			switch(p.cmd.key)
			{
			case VK_LEFT: dir = -1; break;
			case VK_RIGHT: dir = +1; break;
			case VK_RETURN: Open(); return true;
			}

			if(dir != 0)
			{
				Step(dir);

				return true;
			}

			switch(p.cmd.app)
			{
			case APPCOMMAND_MEDIA_PREVIOUSTRACK:
				Step(-1);
				return true;
			case APPCOMMAND_MEDIA_NEXTTRACK:
				Step(+1);
				return true;
			}
		}

		return false;
	}

	bool HotLinkStrip::OnMouse(Control* c, const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::Wheel)
		{
			Step(p.delta < 0 ? -1 : +1);

			return true;
		}

		return false;
	}

	bool HotLinkStrip::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		Vector4i r = WindowRect;

		if(!g_env->hotlinks.empty())
		{
			float f = m_anim;

			if(f == 0)
			{
				m_target = INT_MIN;
			}

			if(f > 0)
			{
				r = WindowRect;

				int h = r.height();
				int w = h * 4 / 3;

				int style = dc->TextStyle;

				clock_t now = clock();

				if(m_target != INT_MIN)
				{
					if(m_current != m_target)
					{
						m_current = (m_current + m_target) / 2;
					}

					if(std::abs(m_current - m_target) == 1)
					{
						m_current = m_target;
					}

					now = m_current;
				}
				else
				{
					m_current = now;
				}

				now += ITEM_DELAY * 1000;

				int i = (now / ITEM_DELAY) % g_env->hotlinks.size();

				float offset = (float)(now % ITEM_DELAY) * w / ITEM_DELAY;

				float yoffset = (f * 2 - 1) * h;

				std::vector<HotLink*> hotlinks;

				hotlinks.insert(hotlinks.end(), g_env->hotlinks.begin(), g_env->hotlinks.end());

				Vector4 r2;
				/*
				r2.right = r.right + w - offset;

				for(bool first = true; r2.right > r.left; first = false, r2.right = r2.left)
				{
					float h2 = r.height();

					if(m_target != INT_MIN) 
					{
						h2 -= r2.right < r.right - w ? 30 : r2.right < r.right ? ((float)r.right - r2.right) * 30 / w : 0;
					}

					float w2 = h2 * 4 / 3;

					r2.left = r2.right - w2;

					float yoffset2 = std::min<float>(yoffset - (r2.left - r.left) / r.width() * h2, 0);

					r2.top = yoffset2;
					r2.bottom = r2.top + h2;

					PaintHotLink(hotlinks[i], r2, m_target != INT_MIN && first, dc);

					if(--i < 0) 
					{
						i = hotlinks.size() - 1;
					}
				}
				*/

				r2.left = r.left - offset;

				for(bool first = true; r2.left < r.right; first = false, r2.left = r2.right)
				{
					float h2 = r.height();

					if(m_target != INT_MIN) 
					{
						h2 -= r2.left > r.left + w ? 30 : r2.left > r.left ? (r2.left - (float)r.left) * 30 / w : 0;
					}

					float w2 = h2 * 4 / 3;

					r2.right = r2.left + w2;

					float yoffset2 = std::min<float>(yoffset - (r2.left - r.left) / r.width() * h2, 0);

					r2.top = yoffset2;
					r2.bottom = r2.top + h2;

					bool selected = false; // m_target != INT_MIN && first;

					if(m_target != INT_MIN)
					{
						if(m_target >= m_current)
						{
							selected = r2.left >= r.left && r2.left < r.left + w;
						}
						else
						{
							selected = r2.left > r.left - w && r2.left <= r.left;
						}
					}

					PaintHotLink(hotlinks[i], r2, selected, dc);

					if(++i == hotlinks.size()) 
					{
						i = 0;
					}
				}

				dc->TextStyle = style;
			}
		}
		else
		{
			m_anim.SetSegment(0);
		}

		if(Control* c = GetRoot()->GetFocus())
		{
			std::wstring buttons[4];

			do
			{
				if(buttons[0].empty() && !c->RedInfo->empty()) buttons[0] = c->RedInfo;
				if(buttons[1].empty() && !c->GreenInfo->empty()) buttons[1] = c->GreenInfo;
				if(buttons[2].empty() && !c->YellowInfo->empty()) buttons[2] = c->YellowInfo;
				if(buttons[3].empty() && !c->BlueInfo->empty()) buttons[3] = c->BlueInfo;

				c = c->Parent;
			}
			while(c);

			bool draw = false;

			for(int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++)
			{
				if(!buttons[i].empty())
				{
					draw = true;
				}
			}

			if(draw)
			{
				float f = 1.0f - m_anim;

				DWORD alpha = (DWORD)((int)(f * 255) << 24);

				int h = Height;

				dc->TextFace = FontType::Bold;

				for(int i = 0; i < 4; i++)
				{
					if(buttons[i].empty()) continue;

					Vector4i r;

					GetInfoRect(i, r);

					Texture* t = dc->GetTexture(Util::Format(L"F%d.png", i + 1).c_str());

					if(t != NULL)
					{
						Vector4i dst = r;

						if(i < 2) dst.right = dst.left + dst.height();
						else dst.left = dst.right - dst.height();

						dc->Draw(t, dst, f);

						dc->TextAlign = Align::Center | Align::Middle;
						dc->TextHeight = dst.height() * 2 / 3;
						dc->TextColor = (Color::Black & 0xffffff) | alpha;

						dc->Draw(Util::Format(L"F%d", i + 1).c_str(), dst);
					}

					{
						Vector4i dst = r;

						if(i < 2) dst.left = dst.left + (dst.height() + 5);
						else dst.right = dst.right - (dst.height() + 5);

						dc->TextAlign = (i < 2 ? Align::Left : Align::Right) | Align::Middle;
						dc->TextHeight = dst.height();
						dc->TextColor = (Color::White & 0xffffff) | alpha;

						dc->Draw(buttons[i].c_str(), dst);
					}
				}
			}
		}

		return true;
	}
}