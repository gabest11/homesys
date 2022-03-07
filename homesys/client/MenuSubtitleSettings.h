#pragma once

#include "MenuSubtitleSettings.dxui.h"

namespace DXUI
{
	class MenuSubtitleSettings : public MenuSubtitleSettingsDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnNext(Control* c);
		bool OnFaceSelected(Control* c, int index);
		bool OnPaintFace(Control* c, PaintItemParam* p);

		std::vector<std::wstring> m_face;
		std::vector<int> m_size;
		std::vector<int> m_outline;
		std::vector<int> m_shadow;
		std::vector<int> m_position;

	public:
		MenuSubtitleSettings();
	};
}
