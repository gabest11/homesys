#pragma once

#include "MenuTVOSD.dxui.h"

namespace DXUI
{
	class MenuTVOSD : public MenuTVOSDDXUI
	{
		bool OnThreadTimer(Control* c, UINT64 n);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnRed(Control* c);
		bool OnYellow(Control* c);
		bool OnCurrentProgramChanged(Control* c, Homesys::Program* p);

		Thread m_thread;
		clock_t m_start;
		std::wstring m_input;
		bool m_update;
		bool m_hidden;

		void ShowMoreEPG(int more);
		void CheckInput();

	public:
		MenuTVOSD();
		virtual ~MenuTVOSD();

		bool Create(const Vector4i& r, Control* parent);

		Event<> UpdateEPGEvent;
	};
}
