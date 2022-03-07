#pragma once

#include "MenuFileBrowser.dxui.h"

namespace DXUI
{
	class MenuFileBrowser : public MenuFileBrowserDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnPlaylistKey(Control* c, const KeyEventParam& p);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnReturnKey(Control* c);
		bool OnDirectory(Control* c, std::wstring path);
		bool OnFile(Control* c, const NavigatorItem* item);
		bool OnEditFile(Control* c);
		bool OnPlayFile(Control* c);
		bool OnSavePlaylist(Control* c);
		bool OnThreadMessage(Control* c, const MSG& msg);

		struct PlaylistItem
		{
			std::wstring path;
			std::wstring name;
			REFERENCE_TIME dur;
		};

		std::vector<PlaylistItem> m_playlist_items;
		REFERENCE_TIME m_total;
		std::wstring m_lastpath;
		Animation m_anim;
		bool m_animplaylist;
		Thread m_thread;

		void TogglePlaylist(int i, bool prompt);

		static std::wstring FormatTime(REFERENCE_TIME rt)
		{
			std::wstring str;

			DVD_HMSF_TIMECODE tc = DirectShow::RT2HMSF(rt);

			if(tc.bHours)
			{
				str = Util::Format(L"%d:%02d:%02d", tc.bHours, tc.bMinutes, tc.bSeconds);
			}
			else if(tc.bMinutes)
			{
				str = Util::Format(L"%d:%02d", tc.bMinutes, tc.bSeconds);
			}
			else
			{
				str = Util::Format(L"%d s", tc.bSeconds);
			}

			return str;
		}

	public:
		MenuFileBrowser();
	};
}
