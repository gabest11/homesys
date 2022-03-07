#include "stdafx.h"
#include "MenuTVOSD.h"
#include "client.h"

namespace DXUI
{
	MenuTVOSD::MenuTVOSD() 
		: m_update(false)
		, m_hidden(false)
	{
		PaintForegroundEvent.Add(&MenuTVOSD::OnPaintForeground, this);
		ActivatedEvent.Add(&MenuTVOSD::OnActivated, this);
		DeactivatedEvent.Add(&MenuTVOSD::OnDeactivated, this);
		KeyEvent.Add(&MenuTVOSD::OnKey, this);
		RedEvent.Add0(&MenuTVOSD::OnRed, this);
		//YellowEvent.Add0(&MenuTVOSD::OnYellow, this);
		ShowEvent.Add0([&] (Control* c) -> bool {m_hidden = false; return true;}); // .Chain0(UpdateEPGEvent);
		UpdateEPGEvent.Add0([&] (Control* c) -> bool {m_update = true; return true;});

		m_controls.m_prev.ClickedEvent.Chain0(UpdateEPGEvent);
		m_controls.m_next.ClickedEvent.Chain0(UpdateEPGEvent);
		m_controls.m_forward.ClickedEvent.Chain0(UpdateEPGEvent);
		m_controls.m_rewind.ClickedEvent.Chain0(UpdateEPGEvent);
		m_controls.m_record.ClickedEvent.Chain0(UpdateEPGEvent);
		m_controls.m_up.ClickedEvent.Chain0(UpdateEPGEvent);
		m_controls.m_down.ClickedEvent.Chain0(UpdateEPGEvent);
		m_controls.m_return.ClickedEvent.Chain0(UpdateEPGEvent);
		g_env->SkipBackwardEvent.Chain0(UpdateEPGEvent);
		g_env->SkipForwardEvent.Chain0(UpdateEPGEvent);

		m_epgview.CurrentProgram.ChangedEvent.Add(&MenuTVOSD::OnCurrentProgramChanged, this);
		m_epgview.m_overlay.PaintForegroundEvent.Add([&] (Control* c, DeviceContext* dc) -> bool
		{
			if(g_env->ps.tuner.stat.scrambled)
			{
				Homesys::Program* program = m_epgview.CurrentProgram;

				if(program != NULL && g_env->ps.tuner.preset.channelId == program->channelId)
				{
					if(Texture* t = dc->GetTexture(L"scrambled.png"))
					{
						Vector4i r = m_epgview.m_rowstack.WindowRect;

						r.right = r.left + 300; // TODO: 300 should not be hard-coded
						r.left = r.right - 35;

						r = DeviceContext::FitRect(t->GetSize(), r);

						dc->Draw(t, r);
					}
				}
			}

			return true;
		});

		m_signal.Text.Get([] (std::wstring& s) -> void {s = Util::Format(L"%d", g_env->ps.tuner.stat.quality);});

		m_thread.TimerEvent.Add(&MenuTVOSD::OnThreadTimer, this);
		m_thread.TimerPeriod = 1000;
	}

	MenuTVOSD::~MenuTVOSD() 
	{
		g_env->SkipBackwardEvent.Unchain0(UpdateEPGEvent);
		g_env->SkipForwardEvent.Unchain0(UpdateEPGEvent);
	}

	bool MenuTVOSD::Create(const Vector4i& r, Control* parent)
	{
		if(!__super::Create(r, parent))
			return false;

		ShowMoreEPG(Util::Config(L"Homesys", L"Settings").GetInt(L"ShowMoreEPG", 1));

		return true;
	}

	void MenuTVOSD::ShowMoreEPG(int more)
	{
		Vector4i r = ClientRect->deflate(20);

		int size = 220;

		if(more > 0 || more < 0 && m_epg.Height < size) 
		{
			r.top = r.bottom - size;

			m_epgdesc.Visible = true;

			Util::Config(L"Homesys", L"Settings").SetInt(L"ShowMoreEPG", 1);
		}
		else if(more == 0 || more < 0 && m_epg.Height >= size)
		{
			r.top = r.bottom - m_epgview.Height;

			m_epgdesc.Visible = false;

			Util::Config(L"Homesys", L"Settings").SetInt(L"ShowMoreEPG", 0);
		}
		else
		{
			return;
		}

		m_epg.Move(r, true);
	}
	
	void MenuTVOSD::CheckInput()
	{
		CAutoLock cAutoLock(&m_lock);

		if(!m_input.empty() && clock() - m_start >= 2000)
		{
			int num = _wtoi(m_input.c_str());

			if(num > 0)
			{
				m_input.clear();

				if(!g_env->players.empty())
				{
					CComPtr<IGenericPlayer> p = g_env->players.front();

					if(CComQIPtr<ITunerPlayer> t = p)
					{
						std::list<Homesys::Preset> presets;

						if(g_env->svc.GetPresets(t->GetTunerId(), true, presets))
						{
							for(auto i = presets.begin(); i != presets.end(); i++)
							{
								if(--num == 0)
								{
									t->SetPreset(i->id);
									
									m_update = true;

									break;
								}
							}
						}
					}
				}
			}
		}
	}

	bool MenuTVOSD::OnThreadTimer(Control* c, UINT64 n)
	{
		CheckInput();

		if(!m_hidden)
		{
			CAutoLock cAutoLock(&m_lock);

			if(Alpha < 0.001f) // TODO
			{
				m_hidden = true;

				m_update = true;
			}
		}

		if(m_update)
		{
			CComQIPtr<ITunerPlayer> t;

			{
				CAutoLock cAutoLock(&m_lock);

				if(!g_env->players.empty())
				{
					CComPtr<IGenericPlayer> p = g_env->players.front();

					t = p;
				}
			}

			if(t != NULL)
			{
				int presetId = 0;

				g_env->svc.GetCurrentPresetId(t->GetTunerId(), presetId);

				CAutoLock cAutoLock(&m_lock);

				CTime now = CTime::GetCurrentTime();
				CTime current = now - CTimeSpan((g_env->ps.stop - g_env->ps.GetCurrent()) / 10000000);

				m_epgview.MakeVisible(presetId, current);
			}


			m_update = false;
		}

		return true;
	}

	bool MenuTVOSD::OnPaintForeground(Control* c, DeviceContext* dc)
	{
		if(!m_input.empty())
		{
			Vector4i r = WindowRect;

			r += Vector4i(5, 50).xyxy();

			dc->Draw(m_input.c_str(), r); 
		}

		return true;
	}

	bool MenuTVOSD::OnActivated(Control* c, int dir)
	{
		UpdateEPGEvent(NULL);

		m_thread.Create();

		return true;
	}

	bool MenuTVOSD::OnDeactivated(Control* c, int dir)
	{
		m_thread.Join(false);

		m_input.clear();

		if(dir <= 0)
		{
		}

		return true;
	}

	bool MenuTVOSD::OnKey(Control* c, const KeyEventParam& p)
	{
		if(p.cmd.key == VK_LEFT && p.mods == KeyEventParam::Ctrl)
		{
			m_controls.m_rewind.Click();

			return true;
		}

		if(p.cmd.key == VK_RIGHT && p.mods == KeyEventParam::Ctrl)
		{
			m_controls.m_forward.Click();

			return true;
		}

		if(p.mods == KeyEventParam::Ctrl)
		{
			if(p.cmd.key == VK_DOWN)
			{
				m_controls.m_prev.Click();

				return true;
			}

			if(p.cmd.key == VK_UP)
			{
				m_controls.m_next.Click();

				return true;
			}
		}

		if(p.mods == 0)
		{
			if(p.cmd.key == VK_NEXT || p.cmd.app == APPCOMMAND_MEDIA_CHANNEL_UP)
			{
				m_controls.m_up.Click();

				return true;
			}

			if(p.cmd.key == VK_PRIOR || p.cmd.app == APPCOMMAND_MEDIA_CHANNEL_DOWN)
			{
				m_controls.m_down.Click();

				return true;
			}

			if(p.cmd.app == APPCOMMAND_MEDIA_PREVIOUSTRACK)
			{
				m_controls.m_prev.Click();

				return true;
			}

			if(p.cmd.app == APPCOMMAND_MEDIA_NEXTTRACK)
			{
				m_controls.m_next.Click();

				return true;
			}

			if(p.cmd.key == 'V' || p.cmd.remote == RemoteKey::Angle)
			{
				g_env->VideoSwitchEvent(this);

				return true;
			}

			if(p.cmd.key == 'A' || p.cmd.remote == RemoteKey::Audio)
			{
				g_env->AudioSwitchEvent(this);

				return true;
			}

			if(p.cmd.key == 'S' || p.cmd.remote == RemoteKey::Subtitle)
			{
				g_env->SubtitleSwitchEvent(this);

				return true;
			}

			if(p.cmd.key == 'R' || p.cmd.app == APPCOMMAND_MEDIA_RECORD)
			{
				m_controls.m_record.Click();

				return true;
			}

			if(p.cmd.chr >= '0' && p.cmd.chr <= '9' || p.cmd.chr == 246)
			{
				wchar_t c = p.cmd.chr == 246 ? '0' : (wchar_t)p.cmd.chr;

				m_start = clock();

				if(c != '0' && m_input.empty())
				{
					m_start += 1000;
				}

				m_input += c;

				if(m_input == L"00")
				{
					m_input.clear();
				}

				return true;
			}
		}

		if(p.mods == KeyEventParam::Shift)
		{
			if(p.cmd.key == '3') // #
			{
				g_env->SubtitleSwitchEvent(this);

				return true;
			}

			if(p.cmd.key == '8') // *
			{
				g_env->AudioSwitchEvent(this);

				return true;
			}
		}

		return false;
	}

	bool MenuTVOSD::OnRed(Control* c)
	{
		if(m_epg.Visible)
		{
			ShowMoreEPG(-1);
		}
		else
		{
			m_epg.Visible = true;
			if(m_epg.Visible) m_epg.Focused = true;
			AutoHide = true;
		}

		return true;
	}

	bool MenuTVOSD::OnYellow(Control* c)
	{
		m_controls.m_return.Click();

		return true;
	}

	bool MenuTVOSD::OnCurrentProgramChanged(Control* c, Homesys::Program* p)
	{
		m_desc.CurrentProgram = p;

		return true;
	}
}
