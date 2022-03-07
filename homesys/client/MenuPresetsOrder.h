#pragma once

#include "MenuPresetsOrder.dxui.h"
#include "Player.h"

namespace DXUI
{
	class MenuPresetsOrder : public MenuPresetsOrderDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnSortByRank(Control* c);
		bool OnSortByName(Control* c);
		bool OnSortByCurrent(Control* c);
		bool OnMoveUp(Control* c);
		bool OnMoveDown(Control* c);
		bool OnPresetListPaintItem(Control* c, PaintItemParam* p);
		bool OnPresetListClicked(Control* c, int index);
		bool OnPresetListKey(Control* c, const KeyEventParam& p);
		bool OnTunerSelected(Control* c, int index);
		bool OnNextClicked(Control* c);

		struct Tuner
		{
			GUID id;
			std::wstring name;
			std::vector<Homesys::Preset> src;
			std::vector<Homesys::Preset> dst;
		};

		std::vector<Tuner> m_tuners;
		std::map<int, int> m_rank;
		Tuner* m_tuner;

	public:
		MenuPresetsOrder();
	};
}
