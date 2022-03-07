#pragma once

#include "Combo.dxui.h"

namespace DXUI
{
	class Combo : public ComboDXUI
	{
		bool OnSelectedChanged(Control* c, int index);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnListMouse(Control* c, const MouseEventParam& p);
		bool OnListClicked(Control* c, int index);

	public:
		Combo();

		void ShowList();

  		Property<int> ListRows;
		Property<int> Selected;
	};
}
