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

#include "File.h"
#include "Subtitle.h"
#include <list>

namespace SSF
{
	class SubtitleFile : public File
	{
		void CreatePredefined();

	public:
		struct SegmentItem
		{
			float start;
			float stop;

			Definition* def;
		};

		class Segment : public std::list<SegmentItem>
		{
		public:
			float m_start;
			float m_stop; 

			Segment() {}
			Segment(float start, float stop, const SegmentItem* si = NULL);
			Segment(const Segment& s);

			void operator = (const Segment& s);
		};

		class SegmentList : public std::list<Segment> 
		{
			std::vector<Segment*> m_index;

			int Index(bool force = false);

		public:
			void RemoveAll();
			void Insert(float start, float stop, Definition* def);
			void Lookup(float at, std::list<SegmentItem>& sis);
			bool Lookup(float at, int& index);
			const Segment* GetSegment(int index);
		};

		SegmentList m_segments;

	public:
		SubtitleFile();
		virtual ~SubtitleFile();

		void Reset();

		void Parse(InputStream& s);
		void Append(InputStream& s, float start, float stop, bool settime = false);
		bool Lookup(float at, std::list<Subtitle*>& subs);

		static std::wstring Escape(LPCWSTR str);
	};
}