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

#include "Glyph.h"
#include "../util/Vector.h"

namespace SSF
{
	template<class T> class Cache
	{
	protected:
		std::map<std::wstring, T> m_map;
		std::list<std::wstring> m_objs;
		int m_limit;

	public:
		Cache(int limit) {m_limit = std::max<int>(limit, 1);}
		virtual ~Cache() {RemoveAll();}

		void RemoveAll()
		{
			for(auto i = m_map.begin(); i != m_map.end(); i++)
			{
				delete i->second;
			}

			m_map.clear();
			m_objs.clear();
		}

		void Add(const std::wstring& key, T& obj, bool flush = true)
		{
			auto i = m_map.find(key);

			if(i != m_map.end())
			{
				delete i->second;

				m_map.erase(i);
			}
			else
			{
				ASSERT(std::find(m_objs.begin(), m_objs.end(), key) == m_objs.end());

				m_objs.push_back(key);
			}

			m_map[key] = obj;

			if(flush) Flush();
		}

		void Flush()
		{
			while((int)m_objs.size() > m_limit)
			{
				auto i = m_map.find(m_objs.front());

				delete i->second;

				m_map.erase(i);

				m_objs.pop_front();
			}
		}

		bool Lookup(const std::wstring& key, T& val)
		{
			auto i = m_map.find(key);

			if(i != m_map.end())
			{
				val = i->second;

				return val != NULL;
			}

			return false;
		}

		void Invalidate(const std::wstring& key)
		{
			auto i = m_map.find(key);

			if(i != m_map.end())
			{
				delete i->second;

				m_map.erase(i);
			}

			m_objs.remove(key);
		}
	};

	class FontCache : public Cache<FontWrapper*> 
	{
	public:
		FontCache() : Cache(20) {}
		FontWrapper* Create(HDC hDC, const LOGFONT& lf);
	};

	class GlyphPathCache : public Cache<GlyphPath*>
	{
	public:
		GlyphPathCache() : Cache(100) {}
		GlyphPath* Create(HDC hDC, const FontWrapper* f, WCHAR c);
	};

	class Row : public std::list<Glyph*>
	{
	public:
		int ascent, descent, border, width;

		Row() {ascent = descent = border = width = 0;}
		virtual ~Row() {for(auto i = begin(); i != end(); i++) delete *i;}
	};

	class RenderedSubtitle : public AlignedClass<16>
	{
	public:
		Vector4i m_bmrc;
		Vector4i m_clip;
		std::list<Glyph*> m_glyphs;

		RenderedSubtitle(const Vector4i& bmrc, const Vector4i& clip) : m_bmrc(bmrc), m_clip(clip) {}
		virtual ~RenderedSubtitle() {for(auto i = m_glyphs.begin(); i != m_glyphs.end(); i++) delete *i;}

		void Rasterize();

		Vector4i Measure() const;
		Vector4i Draw(Bitmap& bm) const;
	};

	class RenderedSubtitleCache : public Cache<RenderedSubtitle*>
	{
	public:
		RenderedSubtitleCache() : Cache(10) {}
	};

	class SubRect : public AlignedClass<16>
	{
	public:
		Vector4i rect;
		float layer;

		SubRect() {}
		SubRect(const Vector4i& r, float l) : rect(r), layer(l) {}
	};

	class SubRectAllocator : public AlignedClass<16>
	{
		Vector4i vr;
		Vector2i vs;

	public:
		std::map<std::wstring, SubRect*> rects;

	public:
		void UpdateTarget(const Vector2i& vs, const Vector4i& vr);
		void GetRect(Vector4i& rect, const Subtitle* s, const Align& align, int tlb, int brb);
		void RemoveAll() {for(auto i = rects.begin(); i != rects.end(); i++) delete i->second; rects.clear();}
	};

	class Renderer
	{
		HDC m_dc;
		FontCache m_fc;
		GlyphPathCache m_gpc;
		RenderedSubtitleCache m_rsc;
		SubRectAllocator* m_sra;

		void Fill(const std::list<Glyph*>& glyphs, int width); 

	public:
		Renderer();
		virtual ~Renderer();

		void Reset();
		void NextSegment(const std::list<Subtitle*>& subs);
		RenderedSubtitle* Lookup(const Subtitle* s, const Vector2i& vs, const Vector4i& vr, bool rasterize = true);
		Vector4i Draw(SubtitleFile& sf, float at, Bitmap& bm, const Vector2i& vs, const Vector4i& vr);
	};
}