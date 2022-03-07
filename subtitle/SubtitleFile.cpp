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
#include "SubtitleFile.h"
#include "Split.h"
#include "../util/String.h"

namespace SSF
{
	SubtitleFile::SubtitleFile()
	{
	}

	SubtitleFile::~SubtitleFile()
	{
	}

	void SubtitleFile::Reset()
	{
		__super::Reset();

		m_segments.RemoveAll();

		SetPredefined(true);

		Reference* root = GetRootRef();

		if(SSF::Definition* def = CreateDef(root, L"color", L"white"))
		{
			def->SetChild(L"r", 255.0f);
			def->SetChild(L"g", 255.0f);
			def->SetChild(L"b", 255.0f);
			def->SetChild(L"a", 255.0f);
		}
		
		if(SSF::Definition* def = CreateDef(root, L"color", L"black"))
		{
			def->SetChild(L"r", 0.0f);
			def->SetChild(L"g", 0.0f);
			def->SetChild(L"b", 0.0f);
			def->SetChild(L"a", 255.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"color", L"gray"))
		{
			def->SetChild(L"r", 128.0f);
			def->SetChild(L"g", 128.0f);
			def->SetChild(L"b", 128.0f);
			def->SetChild(L"a", 255.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"color", L"red"))
		{
			def->SetChild(L"r", 255.0f);
			def->SetChild(L"g", 0.0f);
			def->SetChild(L"b", 0.0f);
			def->SetChild(L"a", 255.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"color", L"green"))
		{
			def->SetChild(L"r", 0.0f);
			def->SetChild(L"g", 255.0f);
			def->SetChild(L"b", 0.0f);
			def->SetChild(L"a", 255.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"color", L"blue"))
		{
			def->SetChild(L"r", 0.0f);
			def->SetChild(L"g", 0.0f);
			def->SetChild(L"b", 255.0f);
			def->SetChild(L"a", 255.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"color", L"cyan"))
		{
			def->SetChild(L"r", 0.0f);
			def->SetChild(L"g", 255.0f);
			def->SetChild(L"b", 255.0f);
			def->SetChild(L"a", 255.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"color", L"yellow"))
		{
			def->SetChild(L"r", 255.0f);
			def->SetChild(L"g", 255.0f);
			def->SetChild(L"b", 0.0f);
			def->SetChild(L"a", 255.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"color", L"magenta"))
		{
			def->SetChild(L"r", 255.0f);
			def->SetChild(L"g", 0.0f);
			def->SetChild(L"b", 255.0f);
			def->SetChild(L"a", 255.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"align", L"topleft"))
		{
			def->SetChild(L"v", 0.0f);
			def->SetChild(L"h", 0.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"align", L"topcenter"))
		{
			def->SetChild(L"v", 0.0f);
			def->SetChild(L"h", 0.5f);
		}

		if(SSF::Definition* def = CreateDef(root, L"align", L"topright"))
		{
			def->SetChild(L"v", 0.0f);
			def->SetChild(L"h", 1.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"align", L"middleleft"))
		{
			def->SetChild(L"v", 0.5f);
			def->SetChild(L"h", 0.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"align", L"middlecenter"))
		{
			def->SetChild(L"v", 0.5f);
			def->SetChild(L"h", 0.5f);
		}

		if(SSF::Definition* def = CreateDef(root, L"align", L"middleright"))
		{
			def->SetChild(L"v", 0.5f);
			def->SetChild(L"h", 1.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"align", L"bottomleft"))
		{
			def->SetChild(L"v", 1.0f);
			def->SetChild(L"h", 0.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"align", L"bottomcenter"))
		{
			def->SetChild(L"v", 1.0f);
			def->SetChild(L"h", 0.5f);
		}

		if(SSF::Definition* def = CreateDef(root, L"align", L"bottomright"))
		{
			def->SetChild(L"v", 1.0f);
			def->SetChild(L"h", 1.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"time", L"time"))
		{
			def->SetChild(L"scale", 1.0f);
		}

		if(SSF::Definition* def = CreateDef(root, L"time", L"startstop"))
		{
			def->SetChild(L"start", L"start");
			def->SetChild(L"stop", L"stop");
		}

		if(SSF::Definition* def = CreateDef(root, L"", L"b"))
		{
			def->SetChild(L"font.weight", L"bold");
		}

		if(SSF::Definition* def = CreateDef(root, L"", L"i"))
		{
			def->SetChild(L"font.italic", L"true");
		}

		if(SSF::Definition* def = CreateDef(root, L"", L"u"))
		{
			def->SetChild(L"font.underline", L"true");
		}

		if(SSF::Definition* def = CreateDef(root, L"", L"s"))
		{
			def->SetChild(L"font.strikethrough", L"true");
		}

		if(SSF::Definition* def = CreateDef(root, L"", L"nobr"))
		{
			def->SetChild(L"linebreak", L"none");
		}

		if(SSF::Definition* subtitle = CreateDef(root, L"subtitle", L"subtitle"))
		{
			subtitle->SetChild(L"clip", L"true");
			subtitle->SetChild(L"wrap", L"normal");
			subtitle->SetChild(L"layer", 0.0f);
			subtitle->SetChild(L"index", 0.0f);

			if(SSF::Definition* frame = CreateDef(CreateRef(subtitle), L"frame"))
			{
				frame->SetChild(L"reference", L"video");
				frame->SetChild(L"resolution.cx", 640.0f);
				frame->SetChild(L"resolution.cy", 480.0f);
			}

			if(SSF::Definition* direction = CreateDef(CreateRef(subtitle), L"direction"))
			{
				direction->SetChild(L"primary", L"right");
				direction->SetChild(L"secondary", L"down");
			}

			if(SSF::Definition* style = CreateDef(CreateRef(subtitle), L"style"))
			{
				style->SetChild(L"linebreak", L"word");
				style->SetChild(L"content", L"text");

				style->SetChild(L"color.r", 255.0f);
				style->SetChild(L"color.g", 255.0f);
				style->SetChild(L"color.b", 255.0f);
				style->SetChild(L"color.a", 255.0f);

				if(SSF::Definition* placement = CreateDef(CreateRef(style), L"placement"))
				{
					placement->SetChild(L"clip", L"none");
					placement->SetChild(L"margin.l", 0.0f);
					placement->SetChild(L"margin.t", 0.0f);
					placement->SetChild(L"margin.r", 0.0f);
					placement->SetChild(L"margin.b", 0.0f);
					placement->SetChild(L"align.v", 1.0f);
					placement->SetChild(L"align.h", 0.5f);
					placement->SetChild(L"pos", L"auto");
					placement->SetChild(L"org", L"auto");
					placement->SetChild(L"offset.x", 0.0f);
					placement->SetChild(L"offset.y", 0.0f);
					placement->SetChild(L"angle.x", 0.0f);
					placement->SetChild(L"angle.y", 0.0f);
					placement->SetChild(L"angle.z", 0.0f);
					placement->SetChild(L"path", L"");
				}

				if(SSF::Definition* font = CreateDef(CreateRef(style), L"font"))
				{
					font->SetChild(L"color.r", 255.0f);
					font->SetChild(L"color.g", 255.0f);
					font->SetChild(L"color.b", 255.0f);
					font->SetChild(L"color.a", 255.0f);
					font->SetChild(L"face", L"Arial");
					font->SetChild(L"size", 20.0f);
					font->SetChild(L"weight", L"bold");
					font->SetChild(L"italic", L"false");
					font->SetChild(L"underline", L"false");
					font->SetChild(L"strikethrough", L"false");
					font->SetChild(L"spacing", 0.0f);
					font->SetChild(L"scale.cx", 1.0f);
					font->SetChild(L"scale.cy", 1.0f);
					font->SetChild(L"kerning", L"true");
				}

				if(SSF::Definition* background = CreateDef(CreateRef(style), L"background"))
				{
					background->SetChild(L"color.r", 0.0f);
					background->SetChild(L"color.g", 0.0f);
					background->SetChild(L"color.b", 0.0f);
					background->SetChild(L"color.a", 255.0f);
					background->SetChild(L"size", 2.0f);
					background->SetChild(L"type", L"outline");
					background->SetChild(L"blur", 0.0f);
					background->SetChild(L"scaled", L"true");
				}

				if(SSF::Definition* shadow = CreateDef(CreateRef(style), L"shadow"))
				{
					shadow->SetChild(L"color.r", 0.0f);
					shadow->SetChild(L"color.g", 0.0f);
					shadow->SetChild(L"color.b", 0.0f);
					shadow->SetChild(L"color.a", 128.0f);
					shadow->SetChild(L"depth", 2.0f);
					shadow->SetChild(L"angle", -45.0f);
					shadow->SetChild(L"blur", 0.0f);
					shadow->SetChild(L"scaled", L"true");
				}

				if(SSF::Definition* fill = CreateDef(CreateRef(style), L"fill"))
				{
					fill->SetChild(L"color.r", 255.0f);
					fill->SetChild(L"color.g", 255.0f);
					fill->SetChild(L"color.b", 0.0f);
					fill->SetChild(L"color.a", 255.0f);
					fill->SetChild(L"width", 0.0f);
				}
			}
		}

		SetPredefined(false);
	}

	void SubtitleFile::Parse(InputStream& s)
	{
		__super::Parse(s);

		// TODO: check file.format == "ssf" and file.version == 1

		std::list<Definition*> defs;

		GetRootRef()->GetChildDefs(defs, L"subtitle");
		
		std::map<std::wstring, float> offset;

		for(auto i = defs.begin(); i != defs.end(); i++)
		{
			Definition* def = *i;

			try
			{
				Definition::Time time;

				if(def->GetAsTime(time, offset) && def->HasValue(L"@"))
				{
					m_segments.Insert(time.start.value, time.stop.value, def);
				}
			}
			catch(Exception&)
			{
			}
		}
	}

	void SubtitleFile::Append(InputStream& s, float start, float stop, bool settime)
	{
		Reference* root = GetRootRef();

		ParseDefs(s);

		std::list<Definition*> defs;

		GetNewDefs(defs);

		for(auto i = defs.begin(); i != defs.end(); i++)
		{
			Definition* def = *i;

			if(def->m_parent == root && def->m_type == L"subtitle" && def->HasValue(L"@"))
			{
				m_segments.Insert(start, stop, def);

				if(settime) 
				{
					try
					{
						Definition::Time time;

						std::map<std::wstring, float> offset;

						def->GetAsTime(time, offset);

						if(time.start.value == start && time.stop.value == stop)
						{
							continue;
						}
					}
					catch(Exception&)
					{
					}

					def->SetChildAsNumber(L"time.start", Util::Format(L"%.3f", start).c_str(), L"s");
					def->SetChildAsNumber(L"time.stop", Util::Format(L"%.3f", stop).c_str(), L"s");
				}
			}
		}

		Commit();
	}

	bool SubtitleFile::Lookup(float at, std::list<Subtitle*>& subs)
	{
		if(!subs.empty()) 
		{
			return false;
		}

		std::list<SegmentItem> sis;

		m_segments.Lookup(at, sis);

		if(!sis.empty())
		{
			std::vector<Subtitle*> tmp;

			tmp.reserve(sis.size());

			for(auto i = sis.begin(); i != sis.end(); i++)
			{
				Subtitle* s = new Subtitle(this);

				if(s->Parse(i->def, i->start, i->stop, at))
				{
					tmp.push_back(s);
				}
				else
				{
					delete s;
				}
			}

			std::sort(tmp.begin(), tmp.end(), [] (Subtitle* a, Subtitle* b) -> bool
			{
				if(a->m_layer < b->m_layer) return true;
				if(a->m_layer > b->m_layer) return false;

				// same layer => index decides
				
				return a->m_index < b->m_index; 
			});

			subs.insert(subs.end(), tmp.begin(), tmp.end());
		}

		return !subs.empty();
	}

	std::wstring SubtitleFile::Escape(LPCWSTR str)
	{
		std::wstring s = str;

		Util::Replace(s, L"\\", L"\\\\");
		Util::Replace(s, L"{", L"\\{");
		Util::Replace(s, L"}", L"\\}");
		Util::Replace(s, L"[", L"\\[");
		Util::Replace(s, L"]", L"\\]");
		Util::Replace(s, L"/", L"\\/");
		Util::Replace(s, L"\n", L"\\n");

		return s;
	}

	//

	SubtitleFile::Segment::Segment(float start, float stop, const SegmentItem* si)
	{
		m_start = start;
		m_stop = stop;

		if(si != NULL)
		{
			push_back(*si);
		}
	}

	SubtitleFile::Segment::Segment(const Segment& s)
	{
		*this = s;
	}

	void SubtitleFile::Segment::operator = (const Segment& s)
	{
		m_start = s.m_start; 
		m_stop = s.m_stop; 

		assign(s.begin(), s.end());
	}

	//

	void SubtitleFile::SegmentList::RemoveAll()
	{
		clear();

		m_index.clear();
	}

	void SubtitleFile::SegmentList::Insert(float start, float stop, Definition* def)
	{
		if(start >= stop) 
		{
			return;
		}

		m_index.clear();

		SegmentItem si = {start, stop, def};

		if(empty())
		{
			push_back(Segment(start, stop, &si));

			return;
		}
		
		const Segment& head = front();
		const Segment& tail = back();
		
		if(start >= tail.m_stop && stop > tail.m_stop)
		{
			if(start > tail.m_stop) 
			{
				push_back(Segment(tail.m_stop, start));
			}

			push_back(Segment(start, stop, &si));
		}
		else if(start < head.m_start && stop <= head.m_start)
		{
			if(stop < head.m_start) 
			{
				push_front(Segment(stop, head.m_start));
			}

			push_front(Segment(start, stop, &si));
		}
		else 
		{
			if(start < head.m_start)
			{
				push_front(Segment(start, head.m_start, &si));

				start = head.m_start;
			}

			if(stop > tail.m_stop)
			{
				push_back(Segment(tail.m_stop, stop, &si));

				stop = tail.m_stop;
			}

			for(auto i = begin(); i != end(); i++)
			{
				Segment& s = *i;

				if(start >= s.m_stop) 
				{
					continue;
				}

				if(stop <= s.m_start) 	
				{
					break;
				}

				if(s.m_start < start && start < s.m_stop)
				{
					Segment s2 = s;

					s2.m_start = start;

					auto j = i;

					insert(++j, s2);

					s.m_stop = start;
				}
				else if(s.m_start == start)
				{
					if(stop > s.m_stop)
					{
						start = s.m_stop;
					}
					else if(stop < s.m_stop)
					{
						Segment s2 = s;

						s2.m_start = stop;

						auto j = i;

						insert(++j, s2);

						s.m_stop = stop;
					}

					s.push_back(si);
				}
			}
		}
	}

	int SubtitleFile::SegmentList::Index(bool force)
	{
		if(m_index.empty() || force)
		{
			m_index.clear();

			for(auto i = begin(); i != end(); i++)
			{
				m_index.push_back(&*i);
			}
		}

		return m_index.size();
	}

	void SubtitleFile::SegmentList::Lookup(float at, std::list<SegmentItem>& sis)
	{
		sis.clear();

		int index;

		if(Lookup(at, index)) 
		{
			const Segment* s = GetSegment(index);

			sis.insert(sis.end(), s->begin(), s->end());
		}
	}

	bool SubtitleFile::SegmentList::Lookup(float at, int& index)
	{
		if(!Index()) return false;

		int i = 0;
		int j = m_index.size() - 1;

		if(m_index[i]->m_start <= at && at < m_index[j]->m_stop)
		{
			do
			{
				index = (i + j) / 2;

				if(m_index[index]->m_start <= at && at < m_index[index]->m_stop) 
				{
					return true;
				}
				else if(at < m_index[index]->m_start) 
				{
					if(j == index)
					{
						index--; 
					}
				
					j = index;
				}
				else if(at >= m_index[index]->m_stop) 
				{
					if(i == index) 
					{
						index++; 
					}

					i = index;
				}
			}
			while(i <= j);
		}

		return false;
	}

	const SubtitleFile::Segment* SubtitleFile::SegmentList::GetSegment(int index)
	{
		return 0 <= index && index < m_index.size() ? m_index[index] : NULL;
	}
}
