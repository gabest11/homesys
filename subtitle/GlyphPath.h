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

#pragma once

#include "../util/Vector.h"

namespace SSF
{
	#define PT_MOVETONC 0xfe
	#define PT_BSPLINETO 0xfc
	#define PT_BSPLINEPATCHTO 0xfa
		
	class GlyphPath
	{
	public:
		int m_count;
		int m_reserved;
		BYTE* m_types;
		Vector2i* m_points;

	public:
		GlyphPath();
		virtual ~GlyphPath();

		GlyphPath(const GlyphPath& path);
		void operator = (const GlyphPath& path);
		void operator += (const GlyphPath& path);

		bool IsEmpty();
		void RemoveAll();
		void SetCount(int count);

		void MovePoints(const Vector2i& o); 
		void Enlarge(const GlyphPath& path, float size);
	};
}