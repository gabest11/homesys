#pragma once

#include "MenuRecordings.dxui.h"
#include "RecordingsDialog.h"

namespace DXUI
{
	class MenuRecordings : public MenuRecordingsDXUI
	{
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnTabChanged(Control* c, int index);
		bool OnListPaintItem(Control* c, PaintItemParam* p);
		bool OnListSelected(Control* c, int index);
		bool OnListClicked(Control* c, int index);
		bool OnListRedEvent(Control* c);
		bool OnListGreenEvent(Control* c);
		bool OnRecordingPlay(Control* c);
		bool OnRecordingEdit(Control* c);
		bool OnRecordingDelete(Control* c);
		bool OnRecordingModify(Control* c);

		void SortRecordings(int col);

		std::vector<Homesys::Recording> m_items[3];
		std::map<int, Homesys::Channel> m_channels;
		Homesys::Program m_program;
		int m_sort[3];

	public:
		MenuRecordings();

		bool Create(const Vector4i& rect, Control* parent);

		RecordingsDialog m_dialog;
	};
}
