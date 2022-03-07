#pragma once

#include "Dialog.h"
#include "ImageButton.h"

namespace DXUI
{
	class OKDialog : public Dialog
	{
	public:
		OKDialog();

		bool Create(const Vector4i& rect, Control* parent);

		Control m_message;
		ImageButton m_ok;
	};
}
