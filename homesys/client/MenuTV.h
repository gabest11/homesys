#pragma once

#include "MenuTV.dxui.h"

namespace DXUI
{
	class MenuTV : public MenuTVDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnPresetSelected(Control* c, int index);
		bool OnThreadTimer(Control* c, UINT64 n);

		Homesys::Program m_program;
		std::vector<Homesys::Preset> m_presets;
		std::vector<MenuList::Item> m_root;
		std::vector<MenuList::Item> m_recordings;
		std::vector<MenuList::Item> m_settings;
		Thread m_thread;

	public:
		MenuTV();

		enum
		{
			None,
			TV,
			WebTV,
			EPG,
			Recordings,
			Schedule,
			Search,
			Wishlist,
			Tuners,
			SmartCards,
			Tuning,
			Presets,
			PresetsOrder,
			Recommend,
			Settings,
		};
	};
}
