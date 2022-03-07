#pragma once

#include <string>
#include "Common.h"
#include "Property.h"

namespace DXUI
{
	class Collection
	{
	public:
		std::wstring GetItemText(int i);

		Property<int> ItemCount;
		Event<ItemTextParam*> ItemTextEvent;
		Property<int> Selected;
	};
}