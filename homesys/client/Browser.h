#pragma once

namespace DXUI
{
	class Browser : public Control
	{
		bool OnDeviceContextChanged(Control* c, DeviceContext* dc);
		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnRed(Control* c);
		bool OnGreen(Control* c);
		bool OnYellow(Control* c);
		bool OnThreadMessage(Control* c, const MSG& msg);
		bool OnLocationChanged(Control* c, std::wstring path);

		DeviceContext* m_dc;
		Thread m_thread;

	public:

		struct Item
		{
			NavigatorItem ni;
			Texture* tex; 
			bool loading, bogus;
			REFERENCE_TIME dur;

			struct Item() {tex = NULL; loading = bogus = false; dur = 0;}
		};

		const Item& GetItem(int index) const {return m_items[index];}

	protected:

		int m_width;
		int m_height;
		bool m_stretch;

		std::vector<Item> m_items;
		std::set<int> m_visible;

		void Lock();
		void Unlock();

		void PaintItem(int index, const Vector4i& r, DeviceContext* dc);

	public:
		Browser();

		void Refresh();

		Navigator m_navigator;
	};
}
