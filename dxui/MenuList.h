#pragma once

#include "MenuList.dxui.h"
#include "Collection.h"

namespace DXUI
{
	class MenuList : public MenuListDXUI, public Collection
	{
	public:
		class Item
		{
		public:
			int m_id;
			std::wstring m_text;
			std::wstring m_image;
			std::vector<Item>* m_target;
			Item() {m_id = 0; m_target = NULL;}
			Item(const std::wstring& text, int id, LPCWSTR image, std::vector<Item>* target = NULL) : m_id(id), m_text(text), m_image(image), m_target(target) {}
		};

	private:
		bool OnActivated(Control* c, int dir);
		bool OnPaintForeground(Control* c, DeviceContext* dc);
		bool OnKey(Control* c, const KeyEventParam& p);
		bool OnMouse(Control* c, const MouseEventParam& p);
		bool OnClicked(Control* c, int index);
		bool OnItemsChanged(Control* c);
		bool OnScroll(Control* c, float pos);

	protected:
		float m_scrollpos;
		float m_targetscrollpos;
		clock_t m_slidestart; 

		Vector4i GetItemRect(DeviceContext* dc, int i);
		int HitTest(const Vector2i& p);

	public:
		MenuList();

		void MakeVisible(int i);
		void SlideStart(clock_t delay = 0);

		Property<std::vector<Item>*> Items;
		Property<int> ItemHeight;
		Property<bool> AutoSelect;

		Event<int> ClickedEvent;
		Event<int> MenuClickedEvent;
	};
}
