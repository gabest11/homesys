#pragma once

#include "Control.h"
#include <string>

namespace DXUI
{
	class Edit : public Control
	{
		void GetDisplayedText(std::wstring& value);
		void Paste();

		bool OnCursorPosChanged(Control* c, int pos);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnKeyRecord(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnFocusSet(Control* c);

		Animation m_anim;
		int m_start;

	public:
		Edit();

		Property<int> CursorPos;
		Property<bool> Password;
		Property<bool> WantReturn;
		Property<std::wstring> DisplayedText;
		Property<std::wstring> BalloonText;
		Property<bool> HideCursor;
		Property<bool> ReadOnly;
		Property<int> TextAsInt;

		Event<> TextInputEvent;
		Event<> ReturnKeyEvent;

		static Event<> EditTextEvent;
	};
}
