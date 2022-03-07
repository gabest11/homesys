#pragma once

#include "Control.h"
#include "DeviceContext.h"
#include "Animation.h"
#include "../util/AlignedClass.h"
#include <list>

namespace DXUI
{
	class WindowManager : public AlignedClass<16>
	{
	protected:
		Vector4i m_control_rect;
		Control* m_control;
		Texture* m_rt;

		// TODO
		Texture* m_rt_old;
		Vector4i m_rt_rect;
		float m_rt_alpha;
		bool m_rt_painted;
		bool m_rt_changed;
		Animation m_anim;
		Animation m_anim_dim;

		std::list<Control*> m_stack;

		void ResetDim();

	public:
		WindowManager();

		Control* GetControl();
		void SetControl(Control* control);

		void Open(Control* c, bool reset = false);
		void Close();
		bool IsOpen(Control* c);

		bool Save(LPCWSTR fn);

		bool OnInput(const KeyEventParam& p);
		bool OnInput(const MouseEventParam& p);
		void OnPaint(const Vector4i& r, DeviceContext* dc, bool locked);
		bool OnDeviceContextChanged(Control* c, DeviceContext* dc);
	};
}
