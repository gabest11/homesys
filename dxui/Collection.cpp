#include "stdafx.h"
#include "Collection.h"

namespace DXUI
{
	std::wstring Collection::GetItemText(int i)
	{
		std::wstring s;

		if(i >= 0 && i < ItemCount)
		{
			ItemTextParam p(i);

			if(ItemTextEvent(NULL, &p))
			{
				s = p.text;
			}
		}

		return s;
	}
}
