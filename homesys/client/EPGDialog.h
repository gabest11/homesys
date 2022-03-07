#pragma once

namespace DXUI
{
	class EPGDialog : public Dialog
	{
	public:
		EPGDialog();

		bool Create(const Vector4i& rect, Control* parent);

		Control m_container;
		ImageButton m_switch;
		ImageButton m_details;
		ImageButton m_record;
	};
}
