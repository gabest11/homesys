#pragma once

#include "Control.h"

namespace DXUI
{
	class Dialog : public Control
	{
		Control* m_host;

		bool OnDeactivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);

	protected:
		bool OnClose(Control* c);

	public:
		Dialog();

		void Show(Control* host = NULL);
		void Hide();
		void Close();

		Event<> HideEvent;
	};
}
