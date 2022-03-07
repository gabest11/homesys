#pragma once

#include "MenuPlaylist.dxui.h"

namespace DXUI
{
	class MenuPlaylist : public MenuPlaylistDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnEndFile(Control* c);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnListClicked(Control* c, int index);

		DXUI::Event<> EndFileEvent;

	protected:
		struct PlaylistItem
		{
			std::wstring path;
			std::wstring title;
			std::wstring desc;
		};

		std::vector<PlaylistItem> m_items;
		std::vector<std::wstring> m_order_items;
		int m_current;

		void Step(int n);

	public:
		MenuPlaylist();
		virtual ~MenuPlaylist();

		void RemoveAll();
		void Add(LPCWSTR path, LPCWSTR title, LPCWSTR desc);
	};
}
