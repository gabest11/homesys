#include "stdafx.h"
#include "Busy.h"

namespace DXUI
{
	Busy::Busy() : m_anim(true)
	{
		m_foreground.Alpha.Get([&] (float& alpha) {alpha = m_anim;});

		m_anim.Set(0.3f, 1, 700);
		m_anim.Add(0.3f, 700);
	}
}