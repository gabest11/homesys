#pragma once

#include "MenuMain.dxui.h"

namespace DXUI
{
	class MenuMain : public MenuMainDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnYellow(Control* c);
		bool OnPaintIconBackground(Control* c, DeviceContext* dc);

		typedef MenuList::Item Item;

		std::vector<Item> m_root;
		std::vector<Item> m_media;
		std::vector<Item> m_settings;
		std::vector<Item> m_playback;

	public:
		MenuMain();

		bool Create(const Vector4i& r, Control* parent);

		enum
		{
			None,
			TV,
			DVD,
			Media,
			BrowseFiles,
			BrowsePictures,
			Settings,
			SettingsGeneral,
			SettingsPersonal,
			SettingsPlayback,
			SettingsVideo,
			SettingsAudio,
			SettingsSubtitle,
			SettingsHotLink,
			SettingsRegistration,
			SettingsParental, 
			Exit,
		};
	};
}
