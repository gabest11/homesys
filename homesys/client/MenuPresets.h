#pragma once

#include "MenuPresets.dxui.h"
#include "Player.h"

namespace DXUI
{
	class MenuPresets : public MenuPresetsDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnPresetListPaintItem(Control* c, PaintItemParam* p);
		bool OnPresetListClicked(Control* c, int index);
		bool OnPresetListKey(Control* c, const KeyEventParam& p);
		bool OnEnabledChecked(Control* c, bool checked);
		bool OnNameTextInput(Control* c);
		bool OnChannelSelected(Control* c, int index);
		bool OnParentalSelected(Control* c, int index);
		bool OnTunerSelected(Control* c, int index);
		bool OnFreqKey(Control* c, const KeyEventParam& p);
		bool OnNextClicked(Control* c);

		struct Tuner
		{
			CComPtr<ITunerPlayer> tuner;
			std::wstring name;
			std::vector<Homesys::Preset> presets;
			int lastpreset;
		};

		std::vector<Tuner> m_tuners;
		std::vector<Homesys::Channel> m_channels;
		std::vector<Homesys::Preset>* m_presets;
		std::vector<std::wstring> m_parentallevels;

	public:
		MenuPresets();
	};
}
