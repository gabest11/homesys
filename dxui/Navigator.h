#pragma once

#include "Property.h"

namespace DXUI
{
	class NavigatorItem
	{
	public:
		bool isdir; 
		bool islnk;
		std::wstring name;
		std::wstring label;
		__int64 size;

		NavigatorItem() {isdir = islnk = false; size = 0;}
	};

	class Navigator : public std::list<NavigatorItem>
	{
		void SetLocation(std::wstring& value);
		void Sort();

	public:
		Navigator();

		bool Browse(const NavigatorItem& item);

		std::wstring GetPath(const NavigatorItem* item);

		Property<std::wstring> Extensions;
		Property<std::wstring> Location;

		Event<const NavigatorItem*> ClickedEvent;
	};
}
