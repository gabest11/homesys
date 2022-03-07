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
#include "FontWrapper.h"

namespace SSF
{
	FontWrapper::FontWrapper(HDC dc, HFONT font, LPCWSTR key)
		: m_font(font)
		, m_key(key)
	{
		HGDIOBJ old = SelectObject(dc, font);

		GetTextMetrics(dc, &m_tm);

		DWORD pairs = GetKerningPairs(dc, 0, NULL);

		if(pairs > 0)
		{
			KERNINGPAIR* kp = new KERNINGPAIR[pairs];

			GetKerningPairs(dc, pairs, kp);

			for(DWORD i = 0; i < pairs; i++) 
			{
				m_kerning[(kp[i].wFirst << 16) | kp[i].wSecond] = kp[i].iKernAmount;
			}

			delete [] kp;
		}

		SelectObject(dc, old);
	}

	FontWrapper::~FontWrapper()
	{
		DeleteObject(m_font);
	}

	int FontWrapper::GetKernAmount(WCHAR c1, WCHAR c2)
	{
		auto i = m_kerning.find((c1 << 16) | c2);

		if(i != m_kerning.end())
		{
			return i->second;
		}

		return 0;
	}

	void FontWrapper::GetTextExtentPoint(HDC dc, WCHAR c, Vector2i& extent)
	{
		auto i = m_extent.find(c);

		if(i != m_extent.end())
		{
			extent = i->second;
		}
		else
		{
			GetTextExtentPoint32W(dc, &c, 1, (SIZE*)&extent);

			m_extent[c] = extent;
		}
	}
}