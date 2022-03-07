#pragma once

#include "MenuSettings.dxui.h"

namespace DXUI
{
	class MenuSettings : public MenuSettingsDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnVerifyPath(Control* c);
		bool OnNext(Control* c);
		bool OnUpdate(Control* c);
		bool OnDownloaderDone(Control* c, int status);

		Homesys::AppVersion m_appversion;
		Downloader m_downloader;

		std::vector<int> m_durations;
		std::vector<std::wstring> m_exts;
		std::vector<int> m_languages;
		std::vector<std::wstring> m_skins;

	public:
		MenuSettings();
	};
}
