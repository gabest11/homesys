#pragma once

namespace DXUI
{
	class EPGChannelList : public List
	{
		void SetCurrentChannelId(int& value);
		void SetCurrentDate(CTime& value);

		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnListPaintItem(Control* c, PaintItemParam* p);
		bool OnListScrollOverflow(Control* c, float pos);
		bool OnListSelectedChanging(Control* c, int index);

	protected:
		Homesys::Channel m_channel;
		std::vector<Homesys::Program> m_items;

		void Update(const CTime& date);

	public:
		EPGChannelList();

		Property<int> CurrentChannelId;
		Property<Homesys::Channel*> CurrentChannel;
		Property<Homesys::Program*> CurrentProgram;
		Property<CTime> CurrentDate;
	};
}
