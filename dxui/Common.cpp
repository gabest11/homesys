#pragma once

#include "stdafx.h"
#include "Common.h"
#include "../util/String.h"

namespace DXUI
{
	DWORD Color::None = 0;
	DWORD Color::Black = 0xff000000;
	DWORD Color::White = 0xffffffff;
	DWORD Color::Gray = 0xff808080;
	DWORD Color::Green = 0xff00ff00;
	DWORD Color::Red = 0xff0000ff;
	DWORD Color::Blue = 0xffff0000;
	DWORD Color::LightGreen = 0xff80ff80;
	DWORD Color::LightRed = 0xff8080ff;
	DWORD Color::LightBlue = 0xffff8080;
	DWORD Color::Text1 = 0xffffffff;
	DWORD Color::Text2 = 0xff4080ff;
	DWORD Color::Text3 = 0xff000000;
	DWORD Color::Background = 0x40000000;
	DWORD Color::DarkBackground = 0x80000000;
	DWORD Color::SeparatorLine = 0xff0063ab;
	DWORD Color::MenuBackground1 = 0xff000c37;
	DWORD Color::MenuBackground2 = 0xff18234a;
	DWORD Color::MenuBackground3 = 0xff001a5e;

	std::wstring FontType::Normal = L"Calibri";
	std::wstring FontType::Bold = L"Calibri Bold";

	DWORD RemoteKey::GetKey(DWORD vid, DWORD pid, const BYTE* buff, DWORD size, DWORD code, DWORD mods)
	{
		DWORD res = 0;
		
		if(vid == 0x045e && pid == 0x006d)
		{
 			for(DWORD i = 1, j = std::min<DWORD>(size, 4); i < j; i++)
			{
				res |= buff[i] << ((i - 1) << 3);
			}

			if(res > RemoteKey::Details) 
			{
				res = 0;
			}
		}
		/*
		else if(vid == 0x22b8 && pid == 0x003b) // motorola remote
		{
			if(buff[0] == 1)
			{
				switch(code)
				{
				case VK_F3:
					if((mods & KeyEventParam::Shift) != 0) res = RemoteKey::Red;
					else res = RemoteKey::Guide;
					break;
				case VK_F4:
					if((mods & KeyEventParam::Shift) != 0) res = RemoteKey::Green;
					else res = RemoteKey::User;
					break;
				case VK_F5:
					if((mods & KeyEventParam::Shift) != 0) res = RemoteKey::Yellow;
					break;
				case VK_F6:
					if((mods & KeyEventParam::Shift) != 0) res = RemoteKey::Blue;
					break;
				case 'D':
					if((mods & KeyEventParam::Ctrl) != 0) res = RemoteKey::Details;
					break;
				}
			}
			
			if(res != 0) printf("* %02x %d => %d\n", code, mods, res);
		}
		*/
		//else
		{
			std::wstring s;

 			for(DWORD i = 0; i < size; i++)
			{
				s += Util::Format(L"%02x ", buff[i]);
			}

			wprintf(L"RemoteKey::GetCode (%d) %s\n", size, s.c_str());
		}

		return res;
	}
}

