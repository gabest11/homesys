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
#include "Subtitle.h"
#include "Split.h"
#include "../util/String.h"

namespace SSF
{
	Subtitle::Subtitle(File* file) 
		: m_file(file)
		, m_animated(false)
		, m_clip(true)
		, m_fill_id(0)
	{
		m_n2n.align[0][L"left"] = 0;
		m_n2n.align[0][L"center"] = 0.5f;
		m_n2n.align[0][L"middle"] = 0.5f;
		m_n2n.align[0][L"right"] = 1;
		m_n2n.align[1][L"top"] = 0;
		m_n2n.align[1][L"middle"] = 0.5f;
		m_n2n.align[1][L"center"] = 0.5f;
		m_n2n.align[1][L"bottom"] = 1;
		m_n2n.weight[L"thin"] = FW_THIN;
		m_n2n.weight[L"normal"] = FW_NORMAL;
		m_n2n.weight[L"bold"] = FW_BOLD;
		m_n2n.transition[L"start"] = 0;
		m_n2n.transition[L"stop"] = 1e10;
		m_n2n.transition[L"linear"] = 1;
	}

	Subtitle::~Subtitle() 
	{
	}

	bool Subtitle::Parse(Definition* def, float start, float stop, float at)
	{
		m_name = def->m_name;

		m_text.clear();

		m_time.start = start;
		m_time.stop = stop;

		at -= start;

		m_fill_id = 0;

		m_file->Commit();

		try
		{
			m_clip = (*def)[L"clip"];

			Definition& frame = (*def)[L"frame"];

			m_frame.reference = frame[L"reference"];
			m_frame.resolution.cx = frame[L"resolution"][L"cx"];
			m_frame.resolution.cy = frame[L"resolution"][L"cy"];

			Definition& direction = (*def)[L"direction"];

			m_direction.primary = direction[L"primary"];
			m_direction.secondary = direction[L"secondary"];

			m_wrap = (*def)[L"wrap"];

			m_layer = (*def)[L"layer"];

			m_index =  (*def)[L"index"];

			Style style;

			GetStyle(&(*def)[L"style"], style);

			std::map<std::wstring, float> offset;

			Definition& block = (*def)[L"@"];

			WCharInputStream s(block);

			Parse(s, style, at, offset, dynamic_cast<Reference*>(block.m_parent));

			// TODO: trimming should be done by the renderer later, after breaking the words into lines

			while(!m_text.empty() && (m_text.front().str == L" " || m_text.front().str == L"\n"))
			{
				m_text.pop_front();
			}

			while(!m_text.empty() && (m_text.back().str == L" " || m_text.back().str == L"\n"))
			{
				m_text.pop_back();
			}

			for(auto i = m_text.begin(); i != m_text.end(); i++)
			{
				if(i->str == L"\n")
				{
					auto j = i;

					j--;

					while(j != m_text.end() && j->str == L" ")
					{
						auto k = j;
						
						j--;
						
						m_text.erase(k);
					}

					j = i;

					j++;

					while(j != m_text.end() && j->str == L" ")
					{
						auto k = j;
						
						j++;
						
						m_text.erase(k);
					}
				}
			}

			for(auto i = m_text.begin(); i != m_text.end(); i++)
			{
				Text& t = *i;

				Color c = t.style.color;

				c.r /= 255;
				c.g /= 255;
				c.b /= 255;
				c.a /= 255;

				t.style.font.color.r = t.style.font.color.r * c.r;
				t.style.font.color.g = t.style.font.color.g * c.g;
				t.style.font.color.b = t.style.font.color.b * c.b;
				t.style.font.color.a = t.style.font.color.a * c.a;

				t.style.background.color.r = t.style.background.color.r * c.r;
				t.style.background.color.g = t.style.background.color.g * c.g;
				t.style.background.color.b = t.style.background.color.b * c.b;
				t.style.background.color.a = t.style.background.color.a * c.a;

				t.style.shadow.color.r = t.style.shadow.color.r * c.r;
				t.style.shadow.color.g = t.style.shadow.color.g * c.g;
				t.style.shadow.color.b = t.style.shadow.color.b * c.b;
				t.style.shadow.color.a = t.style.shadow.color.a * c.a;
			}
		}
		catch(Exception& e)
		{
			wprintf(L"%s", (LPCWSTR)e);

			return false;
		}

		m_file->Rollback();

		return true;
	}

	void Subtitle::GetStyle(Definition* def, Style& style)
	{
		style.placement.pos.x = 0;
		style.placement.pos.y = 0;
		style.placement.pos.auto_x = true;
		style.placement.pos.auto_y = true;

		style.placement.org.x = 0;
		style.placement.org.y = 0;
		style.placement.org.auto_x = true;
		style.placement.org.auto_y = true;

		Rect frame = {0, 0, m_frame.resolution.cx, m_frame.resolution.cy};

		style.placement.clip.t = -1;
		style.placement.clip.r = -1;
		style.placement.clip.b = -1;
		style.placement.clip.l = -1;

		//

		style.linebreak = (*def)[L"linebreak"];
		style.content = (*def)[L"content"];

		Definition& placement = (*def)[L"placement"];
		Definition& clip = placement[L"clip"];
		Definition& margin = placement[L"margin"];
		Definition& align = placement[L"align"];
		Definition& pos = placement[L"pos"];
		Definition& org = placement[L"org"];
		Definition& offset = placement[L"offset"];
		Definition& angle = placement[L"angle"];

		if(clip.IsValue(Definition::DefString))
		{
			std::wstring str = clip;

			if(str == L"frame") style.placement.clip = frame;
			// else ?
		}
		else
		{
			if(clip[L"t"].IsValue()) style.placement.clip.t = clip[L"t"];
			if(clip[L"r"].IsValue()) style.placement.clip.r = clip[L"r"];
			if(clip[L"b"].IsValue()) style.placement.clip.b = clip[L"b"];
			if(clip[L"l"].IsValue()) style.placement.clip.l = clip[L"l"];
		}

		std::map<std::wstring, float> n2n_margin[2];

		n2n_margin[0][L"top"] = 0;
		n2n_margin[0][L"right"] = 0;
		n2n_margin[0][L"bottom"] = frame.b - frame.t;
		n2n_margin[0][L"left"] = frame.r - frame.l;
		n2n_margin[1][L"top"] = frame.b - frame.t;
		n2n_margin[1][L"right"] = frame.r - frame.l;
		n2n_margin[1][L"bottom"] = 0;
		n2n_margin[1][L"left"] = 0;

		margin[L"t"].GetAsNumber(style.placement.margin.t, &n2n_margin[0]);
		margin[L"r"].GetAsNumber(style.placement.margin.r, &n2n_margin[0]);
		margin[L"b"].GetAsNumber(style.placement.margin.b, &n2n_margin[1]);
		margin[L"l"].GetAsNumber(style.placement.margin.l, &n2n_margin[1]);

		align[L"h"].GetAsNumber(style.placement.align.h, &m_n2n.align[0]);
		align[L"v"].GetAsNumber(style.placement.align.v, &m_n2n.align[1]);

		if(pos[L"x"].IsValue()) {style.placement.pos.x = pos[L"x"]; style.placement.pos.auto_x = false;}
		if(pos[L"y"].IsValue()) {style.placement.pos.y = pos[L"y"]; style.placement.pos.auto_y = false;}

		if(org[L"x"].IsValue()) {style.placement.org.x = org[L"x"]; style.placement.org.auto_x = false;}
		if(org[L"y"].IsValue()) {style.placement.org.y = org[L"y"]; style.placement.org.auto_y = false;}

		offset[L"x"].GetAsNumber(style.placement.offset.x);
		offset[L"y"].GetAsNumber(style.placement.offset.y);

		style.placement.angle.x = angle[L"x"];
		style.placement.angle.y = angle[L"y"];
		style.placement.angle.z = angle[L"z"];

		style.placement.path = placement[L"path"];

		Definition& color = (*def)[L"color"];

		style.color.a = color[L"a"];
		style.color.r = color[L"r"];
		style.color.g = color[L"g"];
		style.color.b = color[L"b"];

		Definition& font = (*def)[L"font"];
		Definition& fontcolor = font[L"color"];
		Definition& fontscale = font[L"scale"];

		style.font.face = font[L"face"];
		style.font.size = font[L"size"];
		font[L"weight"].GetAsNumber(style.font.weight, &m_n2n.weight);
		style.font.color.a = fontcolor[L"a"];
		style.font.color.r = fontcolor[L"r"];
		style.font.color.g = fontcolor[L"g"];
		style.font.color.b = fontcolor[L"b"];
		style.font.underline = font[L"underline"];
		style.font.strikethrough = font[L"strikethrough"];
		style.font.italic = font[L"italic"];
		style.font.spacing = font[L"spacing"];
		style.font.scale.cx = fontscale[L"cx"];
		style.font.scale.cy = fontscale[L"cy"];
		style.font.kerning = font[L"kerning"];

		Definition& background = (*def)[L"background"];
		Definition& backgroundcolor = background[L"color"];

		style.background.color.a = backgroundcolor[L"a"];
		style.background.color.r = backgroundcolor[L"r"];
		style.background.color.g = backgroundcolor[L"g"];
		style.background.color.b = backgroundcolor[L"b"];
		style.background.size = background[L"size"];
		style.background.type = background[L"type"];
		style.background.blur = background[L"blur"];
		style.background.scaled = background[L"scaled"];

		Definition& shadow = (*def)[L"shadow"];
		Definition& shadowcolor = shadow[L"color"];

		style.shadow.color.a = shadowcolor[L"a"];
		style.shadow.color.r = shadowcolor[L"r"];
		style.shadow.color.g = shadowcolor[L"g"];
		style.shadow.color.b = shadowcolor[L"b"];
		style.shadow.depth = shadow[L"depth"];
		style.shadow.angle = shadow[L"angle"];
		style.shadow.blur = shadow[L"blur"];
		style.shadow.scaled = shadow[L"scaled"];

		Definition& fill = (*def)[L"fill"];
		Definition& fillcolor = fill[L"color"];

		style.fill.color.a = fillcolor[L"a"];
		style.fill.color.r = fillcolor[L"r"];
		style.fill.color.g = fillcolor[L"g"];
		style.fill.color.b = fillcolor[L"b"];
		style.fill.width = fill[L"width"];
	}

	float Subtitle::GetMixWeight(Definition* def, float at, std::map<std::wstring, float>& offset, int default_id)
	{
		float t = 1;

		try
		{
			std::map<std::wstring, float> n2n;

			n2n[L"start"] = 0;
			n2n[L"stop"] = m_time.stop - m_time.start;

			Definition::Time time;

			if(def->GetAsTime(time, offset, &n2n, default_id))// && time.start.value <= time.stop.value)
			{
				float u = (at - time.start.value) / (time.stop.value - time.start.value);

				if(at <= time.start.value) t = 0;
				else if(at >= time.stop.value) t = 1;
				else t = u;

				if(t < 0) t = 0;
				else if(t >= 1) t = 0.99999f; // doh

				if((*def)[L"loop"].IsValue())
				{
					t *= (float)(*def)[L"loop"];
				}

				std::wstring direction = (*def)[L"direction"].IsValue() ? (*def)[L"direction"] : L"fw";

				if(direction == L"fwbw" || direction == L"bwfw") 
				{
					t *= 2;
				}

				float n;

				t = modf(t, &n);

				if(direction == L"bw" 
				|| direction == L"fwbw" && ((int)n & 1)
				|| direction == L"bwfw" && !((int)n & 1)) 
				{
					t = 1 - t;
				}

				float accel = 1;

				if((*def)[L"transition"].IsValue())
				{
					Definition::Number<float> n;

					(*def)[L"transition"].GetAsNumber(n, &m_n2n.transition);

					if(n.value >= 0) accel = n.value;
				}

				if(t == 0.99999f) 
				{
					t = 1;
				}

				if(u >= 0 && u < 1)
				{
					t = accel == 0 ? 1 : 
						accel == 1 ? t : 
						accel >= 1e10 ? 0 :
						pow(t, accel);
				}
			}
		}
		catch(Exception&)
		{
		}

		return t;
	}

	template<class T> bool Subtitle::MixValue(Definition& def, T& value, float t)
	{
		std::map<std::wstring, T> n2n;

		return MixValue(def, value, t, &n2n);
	}

	template<> bool Subtitle::MixValue(Definition& def, float& value, float t)
	{
		std::map<std::wstring, float> n2n;

		return MixValue(def, value, t, &n2n);
	}

	template<class T> bool Subtitle::MixValue(Definition& def, T& value, float t, std::map<std::wstring, T>* n2n)
	{
		if(!def.IsValue()) 
		{
			return false;
		}

		if(t >= 0.5)
		{
			if(n2n != NULL && def.IsValue(Definition::DefString))
			{
				auto i = n2n->find((LPCWSTR)def);

				if(i != n2n->end())
				{
					value = i->second;

					return true;
				}
			}

			value = (T)def;
		}

		return true;
	}

	template<> bool Subtitle::MixValue(Definition& def, float& value, float t, std::map<std::wstring, float>* n2n)
	{
		if(!def.IsValue()) 
		{
			return false;
		}

		if(t > 0)
		{
			if(n2n != NULL && def.IsValue(Definition::DefString))
			{
				auto i = n2n->find((LPCWSTR)def);

				if(i != n2n->end())
				{
					value += (i->second - value) * t;

					return true;
				}
			}

			value += ((float)def - value) * t;
		}

		return true;
	}

	template<> bool Subtitle::MixValue(Definition& def, Path& src, float t)
	{
		if(!def.IsValue(Definition::DefString))
		{
			return false;
		}

		if(t >= 1)
		{
			src = (LPCWSTR)def;
		}
		else if(t > 0)
		{
			Path dst = (LPCWSTR)def;

			if(src.size() == dst.size())
			{
				for(int i = 0, j = src.size(); i < j; i++)
				{
					Point& s = src[i];

					s.x += (dst[i].x - s.x) * t;
					s.y += (dst[i].y - s.y) * t;
				}
			}
		}

		return true;
	}

	void Subtitle::MixStyle(Definition* def, Style& dst, float t)
	{
		NodePriority fillwidth_priority;

		if(t > 0)
		{
			if(t > 1) t = 1;

			const Style src = dst;

			MixValue((*def)[L"linebreak"], dst.linebreak, t);
			MixValue((*def)[L"content"], dst.content, t);

			Definition& placement = (*def)[L"placement"];
			Definition& clip = placement[L"clip"];
			Definition& margin = placement[L"margin"];
			Definition& align = placement[L"align"];
			Definition& offset = placement[L"offset"];
			Definition& angle = placement[L"angle"];
			Definition& pos = placement[L"pos"];
			Definition& org = placement[L"org"];

			MixValue(clip[L"t"], dst.placement.clip.t, t);
			MixValue(clip[L"r"], dst.placement.clip.r, t);
			MixValue(clip[L"b"], dst.placement.clip.b, t);
			MixValue(clip[L"l"], dst.placement.clip.l, t);
			MixValue(margin[L"t"], dst.placement.margin.t, t);
			MixValue(margin[L"r"], dst.placement.margin.r, t);
			MixValue(margin[L"b"], dst.placement.margin.b, t);
			MixValue(margin[L"l"], dst.placement.margin.l, t);
			MixValue(align[L"h"], dst.placement.align.h, t, &m_n2n.align[0]);
			MixValue(align[L"v"], dst.placement.align.v, t, &m_n2n.align[1]);
			MixValue(offset[L"x"], dst.placement.offset.x, t);
			MixValue(offset[L"y"], dst.placement.offset.y, t);
			MixValue(angle[L"x"], dst.placement.angle.x, t);
			MixValue(angle[L"y"], dst.placement.angle.y, t);
			MixValue(angle[L"z"], dst.placement.angle.z, t);
			MixValue(placement[L"path"], dst.placement.path, t);

			if(dst.placement.pos.auto_x || pos.HasValue(L"x")) 
			{
				dst.placement.pos.auto_x = !MixValue(pos[L"x"], dst.placement.pos.x, dst.placement.pos.auto_x ? 1 : t);
			}
		
			if(dst.placement.pos.auto_y || pos.HasValue(L"y")) 
			{
				dst.placement.pos.auto_y = !MixValue(pos[L"y"], dst.placement.pos.y, dst.placement.pos.auto_y ? 1 : t);
			}

			if(dst.placement.org.auto_x || org.HasValue(L"x")) 
			{
				dst.placement.org.auto_x = !MixValue(org[L"x"], dst.placement.org.x, dst.placement.org.auto_x ? 1 : t);
			}
		
			if(dst.placement.org.auto_y || org.HasValue(L"y")) 
			{
				dst.placement.org.auto_y = !MixValue(org[L"y"], dst.placement.org.y, dst.placement.org.auto_y ? 1 : t);
			}


			Definition& color = (*def)[L"color"];

			MixValue(color[L"a"], dst.color.a, t);
			MixValue(color[L"r"], dst.color.r, t);
			MixValue(color[L"g"], dst.color.g, t);
			MixValue(color[L"b"], dst.color.b, t);

			Definition& font = (*def)[L"font"];
			Definition& fontcolor = font[L"color"];
			Definition& fontscale = font[L"scale"];

			MixValue(font[L"face"], dst.font.face, t);
			MixValue(font[L"size"], dst.font.size, t);
			MixValue(font[L"weight"], dst.font.weight, t, &m_n2n.weight);
			MixValue(fontcolor[L"a"], dst.font.color.a, t);
			MixValue(fontcolor[L"r"], dst.font.color.r, t);
			MixValue(fontcolor[L"g"], dst.font.color.g, t);
			MixValue(fontcolor[L"b"], dst.font.color.b, t);
			MixValue(font[L"underline"], dst.font.underline, t);
			MixValue(font[L"strikethrough"], dst.font.strikethrough, t);
			MixValue(font[L"italic"], dst.font.italic, t);
			MixValue(font[L"spacing"], dst.font.spacing, t);
			MixValue(fontscale[L"cx"], dst.font.scale.cx, t);
			MixValue(fontscale[L"cy"], dst.font.scale.cy, t);
			MixValue(font[L"kerning"], dst.font.kerning, t);

			Definition& background = (*def)[L"background"];
			Definition& backgroundcolor = background[L"color"];

			MixValue(backgroundcolor[L"a"], dst.background.color.a, t);
			MixValue(backgroundcolor[L"r"], dst.background.color.r, t);
			MixValue(backgroundcolor[L"g"], dst.background.color.g, t);
			MixValue(backgroundcolor[L"b"], dst.background.color.b, t);
			MixValue(background[L"size"], dst.background.size, t);
			MixValue(background[L"type"], dst.background.type, t);
			MixValue(background[L"blur"], dst.background.blur, t);
			MixValue(background[L"scaled"], dst.background.scaled, t);

			Definition& shadow = (*def)[L"shadow"];
			Definition& shadowcolor = shadow[L"color"];

			MixValue(shadowcolor[L"a"], dst.shadow.color.a, t);
			MixValue(shadowcolor[L"r"], dst.shadow.color.r, t);
			MixValue(shadowcolor[L"g"], dst.shadow.color.g, t);
			MixValue(shadowcolor[L"b"], dst.shadow.color.b, t);
			MixValue(shadow[L"depth"], dst.shadow.depth, t);
			MixValue(shadow[L"angle"], dst.shadow.angle, t);
			MixValue(shadow[L"blur"], dst.shadow.blur, t);
			MixValue(shadow[L"scaled"], dst.shadow.scaled, t);

			Definition& fill = (*def)[L"fill"];
			Definition& fillcolor = fill[L"color"];
			Definition& fillwidth = fill[L"width"];

			MixValue(fillcolor[L"a"], dst.fill.color.a, t);
			MixValue(fillcolor[L"r"], dst.fill.color.r, t);
			MixValue(fillcolor[L"g"], dst.fill.color.g, t);
			MixValue(fillcolor[L"b"], dst.fill.color.b, t);
			MixValue(fillwidth, dst.fill.width, t);

			fillwidth_priority = fillwidth.m_priority;
		}
		else
		{
			fillwidth_priority = (*def)[L"fill"][L"width"].m_priority;
		}

		if(fillwidth_priority >= PNormal) // this assumes there is no way to set low priority inline overrides
		{
			if(dst.fill.id > 0) 
			{
				throw Exception(L"cannot apply fill more than once on the same text");
			}

			dst.fill.id = ++m_fill_id;
		}
	}

	void Subtitle::Parse(InputStream& s, Style style, float at, std::map<std::wstring, float> offset, Reference* parent)
	{
		Text text;

		text.style = style;

		for(int c = s.Peek(); c != Stream::EOS; c = s.Peek())
		{
			s.Get();

			if(c == '[')
			{
				AddText(text);

				style = text.style;

				std::map<std::wstring, float> inneroffset = offset;

				int default_id = 0;

				do
				{
					Definition* def = m_file->CreateDef(parent);

					m_file->ParseRefs(s, def, L",;]");

					if((*def)[L"time"][L"start"].IsValue() && (*def)[L"time"][L"stop"].IsValue())
					{
						m_animated = true;
					}

					MixStyle(def, style, GetMixWeight(def, at, offset, ++default_id));

					if(def->HasValue(L"@"))
					{
						WCharInputStream s((*def)[L"@"]);

						Parse(s, style, at, offset, parent);
					}

					s.SkipWhiteSpace();

					c = s.Get();
				}
				while(c == ',' || c == ';');

				if(c != ']')
				{
					s.ThrowError(L"unterminated override");
				}

				bool fWhiteSpaceNext = s.IsWhiteSpace(s.Peek());

				c = s.SkipWhiteSpace();

				if(c == '{') 
				{
					s.Get();

					Parse(s, style, at, inneroffset, parent);
				}
				else 
				{
					if(fWhiteSpaceNext) 
					{
						text.str += L" ";
					}

					text.style = style;
				}
			}
			else if(c == ']')
			{
				s.ThrowError(L"unexpected ] found");
			}
			else if(c == '{')
			{
				AddText(text);

				Parse(s, text.style, at, offset, parent);
			}
			else if(c == '}')
			{
				break;
			}
			else
			{
				if(c == '\\')
				{
					c = s.Get();

					if(c == Stream::EOS) 
					{
						break;
					}
					else if(c == 'n') 
					{
						AddText(text); 
						
						text.str = L"\n"; 
						
						AddText(text); 
						
						continue;
					}
					else if(c == 'h') 
					{
						c = L'\u00a0'; // hard space
					}
				}

				AddChar(text, (WCHAR)c);
			}
		}

		AddText(text);
	}

	void Subtitle::AddChar(Text& t, WCHAR c)
	{
		bool f1 = !t.str.empty() && Stream::IsWhiteSpace(t.str.back());
		bool f2 = Stream::IsWhiteSpace(c);

		if(f2) 
		{
			c = ' ';
		}

		if(!f1 || !f2) 
		{
			t.str += (WCHAR)c;
		}
	}

	void Subtitle::AddText(Text& t)
	{
		if(t.str.empty())
		{
			return;
		}

		if(t.style.content == L"text")
		{
			Split sa(' ', t.str, 0, Split::Max);

			for(int i = 0, j = sa.size(); i < j; i++)
			{
				const std::wstring& str = sa[i];

				if(!str.empty())
				{
					t.str = str;

					m_text.push_back(t);
				}

				if(i < j - 1 && (m_text.empty() || m_text.back().str != L" "))
				{
					t.str = L" ";

					m_text.push_back(t);
				}
			}
		}
		else if(t.style.content == L"polygon")
		{
			// TODO: verify polygon string?

			m_text.push_back(t);
		}
		
		t.str.clear();
	}

	//

	Color::operator DWORD()
	{
		DWORD c = 
			(min(max((DWORD)r, 0), 255) <<  0) |
			(min(max((DWORD)g, 0), 255) <<  8) |
			(min(max((DWORD)b, 0), 255) << 16) |
			(min(max((DWORD)a, 0), 255) << 24);

		return c;
	}

	Path& Path::operator = (LPCWSTR str)
	{
		Split s(' ', str);

		resize(s.size() / 2);

		for(int i = 0, j = size(); i < j; i++)
		{
			Point p;

			p.x = s.GetAsFloat(i * 2 + 0);
			p.y = s.GetAsFloat(i * 2 + 1);

			(*this)[i] = p;
		}

		return *this;
	}

	std::wstring Path::ToString()
	{
		std::wstring ret;

		for(auto i = begin(); i != end(); i++)
		{
			ret += Util::Format(L"%f %f ", i->x, i->y);
		}

		return ret;
	}

	bool Style::IsSimilar(const Style& s)
	{
		return 
		   color.r == s.color.r
		&& color.g == s.color.g
		&& color.b == s.color.b
		&& color.a == s.color.a
		&& font.color.r == s.font.color.r
		&& font.color.g == s.font.color.g
		&& font.color.b == s.font.color.b
		&& font.color.a == s.font.color.a
		&& background.color.r == s.background.color.r
		&& background.color.g == s.background.color.g
		&& background.color.b == s.background.color.b
		&& background.color.a == s.background.color.a
		&& background.size == s.background.size
		&& background.type == s.background.type
		&& background.blur == s.background.blur
		&& background.scaled == s.background.scaled
		&& shadow.color.r == s.shadow.color.r
		&& shadow.color.g == s.shadow.color.g
		&& shadow.color.b == s.shadow.color.b
		&& shadow.color.a == s.shadow.color.a
		&& shadow.depth == s.shadow.depth
		&& shadow.angle == s.shadow.angle
		&& shadow.blur == s.shadow.blur
		&& shadow.scaled == s.shadow.scaled
		&& fill.id == s.fill.id;
	}
}