#include "stdafx.h"
#include "MenuMain.h"
#include "client.h"

namespace DXUI
{
	MenuMain::MenuMain()
	{
		ActivatedEvent.Add(&MenuMain::OnActivated, this);
		YellowEvent.Add0(&MenuMain::OnYellow, this);

		m_icon.PaintBackgroundEvent.Add(&MenuMain::OnPaintIconBackground, this, true);
	}

	bool MenuMain::Create(const Vector4i& r, Control* parent)
	{
		if(!__super::Create(r, parent))
			return false;

		return true;
	}

	bool MenuMain::OnActivated(Control* c, int dir)
	{
		std::wstring s;

		if(g_env->ss.machine.username.empty())
		{
			s = Control::GetString("REGISTRATION");
		}

		YellowInfo = s;

		bool online = g_env->ss.online;

		std::wstring region;

		g_env->svc.GetRegion(region);

		m_root.clear();
		m_media.clear();
		m_settings.clear();
		m_playback.clear();

		m_root.push_back(Item(Control::GetString("TV"), TV, L"tv.png"));
		m_root.push_back(Item(Control::GetString("MY_COMPUTER"), Media, L"mycomputer.png", &m_media));
		m_root.push_back(Item(Control::GetString("SETTINGS"), Settings, L"setting.png", &m_settings));
		m_root.push_back(Item(Control::GetString("EXIT"), Exit, L"exit.png"));

		m_media.push_back(Item(Control::GetString("FILES"), BrowseFiles, L"files.png"));
		m_media.push_back(Item(Control::GetString("PICTURES"), BrowsePictures, L"kepek.png"));
		m_media.push_back(Item(Control::GetString("CD_DVD"), DVD, L"dvd.png"));
		m_media.push_back(Item(Control::GetString("BACK"), None, L"back.png", &m_root));
		
		m_settings.push_back(Item(Control::GetString("GENERAL"), SettingsGeneral, L"setting_common.png"));
		
		if(g_env->ss.machine.username.empty())
		{
			m_settings.push_back(Item(Control::GetString("REGISTRATION"), SettingsRegistration, L""));
		}

		// m_settings.push_back(Item(Control::GetString("SETTINGS_PERSONAL"), SettingsPersonal, L"")); // don't collect personal data
		m_settings.push_back(Item(Control::GetString("PLAYBACK"), SettingsPlayback, L"setting_play.png", &m_playback));
		m_settings.push_back(Item(Control::GetString("HOTLINK_STRIP"), SettingsHotLink, L"setting_recommend.png"));
		m_settings.push_back(Item(Control::GetString("PARENTAL"), SettingsParental, L"setting_lock.png"));
		m_settings.push_back(Item(Control::GetString("BACK"), None, L"back.png", &m_root));

		m_playback.push_back(Item(Control::GetString("VIDEO_SETTINGS"), SettingsVideo, L""));
		m_playback.push_back(Item(Control::GetString("AUDIO_SETTINGS"), SettingsAudio, L""));
		m_playback.push_back(Item(Control::GetString("SUBTITLE_SETTINGS"), SettingsSubtitle, L""));
		m_playback.push_back(Item(Control::GetString("BACK"), None, L"back.png", &m_settings));

		if(dir >= 0)
		{
			m_list.Items = &m_root;
			m_list.Selected = 0;
			m_list.Focused = true;
		}

		m_video.Visible = !g_env->players.empty();

		Vector4i r = m_icon.Rect;

		r.bottom = g_env->players.empty() ? m_container.Height : m_container.Height / 2;
		
		m_icon.Move(r);

		return true;
	}

	bool MenuMain::OnYellow(Control* c)
	{
		std::wstring username;

		if(g_env->svc.GetUsername(username) && username.empty())
		{
			m_list.MenuClickedEvent(this, SettingsRegistration);
		}

		return true;
	}

	bool MenuMain::OnPaintIconBackground(Control* c, DeviceContext* dc)
	{
		int index = m_list.Selected;

		if(index < 0) return true;

		std::vector<MenuList::Item>* items = m_list.Items;

		if(items == NULL) return true;

		std::wstring s = (*items)[index].m_image;

		Texture* t = dc->GetTexture(s.c_str());

		if(t == NULL)
		{
			t = dc->GetTexture(L"jolly.png");
		}

		if(t != NULL)
		{
			dc->Draw(t, dc->FitRect(t->GetSize(), c->WindowRect));
		}

		return true;
	}
}