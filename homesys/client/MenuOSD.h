#pragma once

#include "MenuOSD.dxui.h"

namespace DXUI
{
	class MenuOSD : public MenuOSDDXUI
	{
		Animation m_anim;
		Animation m_volume_anim;

		bool OnActivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnPlayFile(Control* c);
		bool OnVolumeChanged(Control* c, float volume);
		bool OnMutedChanged(Control* c, bool muted);

		float m_volume;
		bool m_muted;
		int m_debug;

	protected:
		bool Message(const KeyEventParam& p);
		bool Message(const MouseEventParam& p);

	public:
		MenuOSD();

		void Show();
		void Hide();

		Property<bool> AutoHide;
		
		Event<> ShowEvent;
	};
}
