/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Split.h"
#include "Exception.h"
#include "../util/String.h"

namespace SSF
{
	Split::Split()
	{
	}

	Split::Split(WCHAR sep, const std::wstring& str, int limit, Type type)
	{
		DoSplit(std::wstring(1, sep), str, limit, type);
	}

	Split::Split(LPCWSTR sep, const std::wstring& str, int limit, Type type)
	{
		DoSplit(std::wstring(sep), str, limit, type);
	}

	Split::Split(const std::wstring& sep, const std::wstring& str, int limit, Type type)
	{
		DoSplit(sep, str, limit, type);
	}

	void Split::DoSplit(const std::wstring& sep, const std::wstring& str, int limit, Type type)
	{
		clear();

		int seplen = sep.size();

		if(seplen > 0)
		{
			for(int i = 0, j = 0, len = str.size(); 
				i <= len && (limit == 0 || size() < limit); 
				i = j + seplen)
			{
				j = str.find(sep, i);

				if(j < 0) j = len;

				std::wstring s;
				
				if(i < j) s = str.substr(i, j - i);

				switch(type)
				{
				case Min: s = Util::Trim(s); // fall through
				case Def: if(s.empty()) break; // else fall through
				case Max: push_back(s); break;
				}
			}
		}
	}

	int Split::GetAsInt(int i)
	{
		if(i < 0 && i >= size()) 
		{
			throw Exception(L"Index out of bounds");
		}

		return _wtoi(at(i).c_str());
	}

	float Split::GetAsFloat(int i)
	{
		if(i < 0 && i >= size()) 
		{
			throw Exception(L"Index out of bounds");
		}

		return (float)_wtof(at(i).c_str());
	}
}