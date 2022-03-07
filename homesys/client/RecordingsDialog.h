#pragma once

namespace DXUI
{
	class RecordingsDialog : public Dialog
	{
	public:
		RecordingsDialog();

		bool Create(const Vector4i& rect, Control* parent);

		Control m_container;
		ImageButton m_play;
		ImageButton m_edit;
		ImageButton m_delete;
		ImageButton m_modify;
	};
}
