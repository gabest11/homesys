#pragma once

#include "MenuWishlist.dxui.h"

namespace DXUI
{
	class MenuWishlist : public MenuWishlistDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnConfirmAdd(Control* c);
		bool OnAdd(Control* c);
		bool OnDelete(Control* c);

		std::vector<Homesys::WishlistRecording> m_recordings;
		std::vector<Homesys::Channel> m_channels;
		std::map<int, std::wstring> m_channel_map;
		std::vector<Homesys::Program> m_programs;

	public:
		MenuWishlist();
	};
}
