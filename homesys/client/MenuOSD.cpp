#include "stdafx.h"
#include "MenuOSD.h"
#include "client.h"
#include "../../DirectShow/Filters.h"
#include "../../DirectShow/MediaTypeEx.h"

#define ALPHA_MIN 0.0001f
#define ALPHA_MAX 0.9f

namespace DXUI
{
	MenuOSD::MenuOSD() 
	{
		m_anim.Set(ALPHA_MIN, 1, 1000);
		m_anim.Add(1, 8000);
		m_anim.Add(ALPHA_MIN, 1000);

		m_volume_anim.Set(1, 1, 3000);
		m_volume_anim.Add(0, 1000);

		Alpha.Get([&] (float& value) {if(AutoHide) value = m_anim; value = std::min<float>(value, ALPHA_MAX);});

		ActivatedEvent.Add(&MenuOSD::OnActivated, this);
		KeyEvent.Add(&MenuOSD::OnKey, this);
		PaintForegroundEvent.Add(&MenuOSD::OnPaintForeground, this);

		AutoHide = true;
		Transparent = true;
		ClickThrough = true;

		g_env->PlayFileEvent.Add0(&MenuOSD::OnPlayFile, this);
		g_env->Volume.ChangedEvent.Add(&MenuOSD::OnVolumeChanged, this);
		g_env->Muted.ChangedEvent.Add(&MenuOSD::OnMutedChanged, this);

		m_volume = 1.0f;
		m_muted = false;
		m_debug = 0;
	}

	bool MenuOSD::Message(const KeyEventParam& e)
	{
		if(e.cmd.key != VK_MENU && e.cmd.key != VK_CONTROL && e.cmd.key != VK_SHIFT)
		{
			Show();
		}
		
		if(0) // e.cmd.chr == 'd')
		{
			m_debug = (m_debug + 1) % 3;
		}

		return __super::Message(e);
	}

	bool MenuOSD::Message(const MouseEventParam& p)
	{
		if(p.cmd == MouseEventParam::Down || p.cmd == MouseEventParam::Move)
		{
			Show();

			if(p.cmd == MouseEventParam::Down && Alpha > ALPHA_MIN)
			{
				Control* clicked = NULL;

				// TODO: Control::HitTest

				std::list<Control*> controls;

				GetDescendants(controls, true, false);

				for(auto i = controls.begin(); i != controls.end(); i++)
				{
					Control* c = *i;

					if(c != this && c->C2W(c->HoverRect).contains(p.point))
					{
						clicked = c;

						break;
					}
				}

				if(clicked == NULL)
				{
					Hide();
				}
			}
		}

		return __super::Message(p);
	}

	void MenuOSD::Show()
	{
		switch(m_anim.GetSegment())
		{
		case 1: 
		case 2: 
			m_anim.SetSegment(1); 
			break;
		case 3: 
			m_anim.SetSegment(0); 
			ShowEvent(this);
			break;
		}
	}

	void MenuOSD::Hide()
	{
		m_anim.SetSegment(3);
	}

	bool MenuOSD::OnActivated(Control* c, int dir)
	{
		if(dir >= 0)
		{
		}

		return true;
	}

	bool MenuOSD::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.mods == 0)
		{
			if(p.cmd.chr == 't' || p.cmd.remote == RemoteKey::Teletext)
			{
				g_env->TeletextEvent(this);

				return true;
			}
		}

		return false;
	}

	bool MenuOSD::OnPlayFile(Control* c)
	{
		Show();
		
		return true;
	}

	bool MenuOSD::OnVolumeChanged(Control* c, float volume)
	{
		m_volume = volume;

		m_volume_anim.SetSegment(0);

		Show();
		
		return true;
	}

	bool MenuOSD::OnMutedChanged(Control* c, bool muted)
	{
		m_muted = muted;

		m_volume_anim.SetSegment(0);

		Show();

		return true;
	}

	bool MenuOSD::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		float f = m_volume_anim;

		if(f > 0)
		{
			Vector4i r = WindowRect;

			int x = 15;
			int y = 15;
			int w = 20;
			int h = 30;
			int gap = 5;

			DWORD color = (!m_muted ? Color::Green : Color::Red) | ((DWORD)(f * 0xff + 0.5f) << 24);

			for(int i = 0, j = 10, k = (int)(m_volume * j + 0.5f); i < j && i < k; i++, x += w + gap)
			{
				dc->Draw(Vector4i(x, y, x + w, y + h), color);
			}
		}

		if(m_debug > 0)
		{
			if(!g_env->players.empty())
			{
				CComPtr<IGenericPlayer> player = g_env->players.front();

				CComPtr<IFilterGraph> pFG;

				player->GetFilterGraph(&pFG);

				Vector4i r = ClientRect;

				r.left = 30;
				r.top = 50;
				r.bottom = 70;

				dc->TextHeight = r.height();
				dc->TextColor = Color::White;
				dc->TextStyle = dc->TextStyle | TextStyles::Outline;

				Vector4i rowsize = Vector4i(0, r.height() + 5).xyxy();

				if(CComQIPtr<ITunerPlayer> tuner = player)
				{
					Homesys::TunerStat& stat = g_env->ps.tuner.stat;

					std::wstring s;
					s = Util::Format(L"[Tuner] P: %d, L: %d, F: %d KHz, S: %d, Q: %d", stat.present, stat.present, stat.freq, stat.strength, stat.quality);
					if(stat.received >= 0) s += Util::Format(L", R: %lld KB", stat.received >> 10);
					
					dc->Draw(s.c_str(), r);					
					
					r += rowsize;
				}

				Foreach(pFG, [&] (IBaseFilter* pBF) -> HRESULT
				{
					CFilterInfo fi;

					pBF->QueryFilterInfo(&fi);

					std::wstring s = L"[Filter] ";
					s += fi.achName;

					TextEntry* te = dc->Draw(s.c_str(), r);
					
					r += rowsize;
					
					if(m_debug > 1)
					{
						::Foreach(pBF, [&] (IPin* pPin) -> HRESULT
						{
							CMediaType mt;

							if(SUCCEEDED(pPin->ConnectionMediaType(&mt)))
							{
								PIN_DIRECTION dir;
								
								pPin->QueryDirection(&dir);

								s = (dir == PINDIR_INPUT ? L" [In] " : L" [Out] ") + CMediaTypeEx(mt).ToString();

								dc->Draw(s.c_str(), r);
					
								r += rowsize;
							}

							return S_CONTINUE;
						});
					}

					CComQIPtr<IBufferInfo> pBI = pBF;

					if(pBI != NULL)
					{
						std::wstring s;

						for(int i = 0, j = pBI->GetCount(); i < j; i++)
						{
							int samples = 0, size = 0;

							if(SUCCEEDED(pBI->GetStatus(i, samples, size)))
							{
								s += Util::Format(L"[%d] %d/%d ", i, samples, size);
							}
						}

						if(!s.empty())
						{
							te = dc->Draw(s.c_str(), r);

							r += rowsize;
						}
					}

					return S_CONTINUE;
				});
			}
		}

		return true;
	}
}
