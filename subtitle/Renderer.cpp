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
 *  TODO: do something about bidi (at least handle these ranges: 0590-07BF, FB1D-FDFF, FE70-FEFF)
 *
 */

#include "stdafx.h"
#include "Renderer.h"
#include "Split.h"
#include "Arabic.h"
#include "../util/String.h"

namespace SSF
{
	static Vector2i GetAlignPoint(const Placement& placement, const Vector2& scale, const Vector4i& frame, const Vector2i& size)
	{
		Vector2i p;

		if(placement.pos.auto_x)
		{
			p.x = (int)(placement.align.h * (frame.width() - size.x) + frame.x);
		}
		else
		{
			// FIXME
			p.x = (int)(placement.pos.x * scale.x - placement.align.h * size.x);
		}

		if(placement.pos.auto_y)
		{
			p.y = (int)(placement.align.v * (frame.height() - size.y) + frame.y);
		}
		else
		{
			// FIXME
			p.y = (int)(placement.pos.y * scale.y - placement.align.v * size.y);
		}

		return p;
	}

	static Vector2i GetAlignPoint(const Placement& placement, const Vector2& scale, const Vector4i& frame)
	{
		return GetAlignPoint(placement, scale, frame, Vector2i(0, 0));
	}

	//

	Renderer::Renderer()
	{
		m_dc = CreateCompatibleDC(NULL);
		SetBkMode(m_dc, TRANSPARENT); 
		SetTextColor(m_dc, 0xffffff); 
		SetMapMode(m_dc, MM_TEXT);

		m_sra = new SubRectAllocator();
	}

	Renderer::~Renderer()
	{
		delete m_sra;

		DeleteDC(m_dc);
	}

	void Renderer::Reset()
	{
		m_sra->RemoveAll();
		m_rsc.RemoveAll();
	}

	void Renderer::NextSegment(const std::list<Subtitle*>& subs)
	{
		std::set<std::wstring> names;

		for(auto i = subs.begin(); i != subs.end(); i++)
		{
			names.insert((*i)->m_name);
		}

		for(auto i = m_sra->rects.begin(); i != m_sra->rects.end(); )
		{
			auto j = i++;

			if(names.find(j->first) == names.end())
			{
				delete j->second;

				m_sra->rects.erase(j);
			}
		}
	}

	RenderedSubtitle* Renderer::Lookup(const Subtitle* s, const Vector2i& vs, const Vector4i& vr, bool rasterize)
	{
		m_sra->UpdateTarget(vs, vr);

		if(s->m_text.empty())
		{
			return NULL;
		}

		Vector4i bmrc = s->m_frame.reference == L"video" ? vr : Vector4i(0, 0, vs.x, vs.y);

		if(bmrc.rempty())
		{
			return NULL;
		}

		RenderedSubtitle* rs = NULL;

		if(m_rsc.Lookup(s->m_name, rs))
		{
			if(!s->m_animated && rs->m_bmrc.eq(bmrc))
			{
				return rs;
			}

			m_rsc.Invalidate(s->m_name);
		}

		const Style& style = s->m_text.front().style;

		Vector2 scale;

		scale.x = (float)bmrc.width() / s->m_frame.resolution.cx;
		scale.y = (float)bmrc.height() / s->m_frame.resolution.cy;

		Vector4i frame;

		frame.left = (int)(64.0f * (bmrc.left + style.placement.margin.l * scale.x) + 0.5f);
		frame.top = (int)(64.0f * (bmrc.top + style.placement.margin.t * scale.y) + 0.5f);
		frame.right = (int)(64.0f * (bmrc.right - style.placement.margin.r * scale.x) + 0.5f);
		frame.bottom = (int)(64.0f * (bmrc.bottom - style.placement.margin.b * scale.y) + 0.5f);

		Vector4i clip;

		if(style.placement.clip.l == -1) clip.left = 0;
		else clip.left = (int)(bmrc.left + style.placement.clip.l * scale.x);
		if(style.placement.clip.t == -1) clip.top = 0;
		else clip.top = (int)(bmrc.top + style.placement.clip.t * scale.y); 
		if(style.placement.clip.r == -1) clip.right = vs.x;
		else clip.right = (int)(bmrc.left + style.placement.clip.r * scale.x);
		if(style.placement.clip.b == -1) clip.bottom = vs.y;
		else clip.bottom = (int)(bmrc.top + style.placement.clip.b * scale.y);

		clip = clip.rintersect(Vector4i(0, 0, vs.x, vs.y));

		scale.x *= 64;
		scale.y *= 64;

		bool vertical = s->m_direction.primary == L"down" || s->m_direction.primary == L"up";

		// create glyph paths

		WCHAR c_prev = 0, c_next;

		std::list<Glyph*> glyphs;

		LOGFONT lf;

		memset(&lf, 0, sizeof(lf));

		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfOutPrecision = OUT_TT_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality = ANTIALIASED_QUALITY;
		lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;

		for(auto i = s->m_text.begin(); i != s->m_text.end(); )
		{
			const Text& t = *i++;

			wcscpy(lf.lfFaceName, t.style.font.face.c_str());
			lf.lfHeight = (LONG)(t.style.font.size * scale.y + 0.5f);
			lf.lfWeight = (LONG)(t.style.font.weight + 0.5f);
			lf.lfItalic = !!t.style.font.italic;
			lf.lfUnderline = !!t.style.font.underline;
			lf.lfStrikeOut = !!t.style.font.strikethrough;

			FontWrapper* font = m_fc.Create(m_dc, lf);

			if(font == NULL)
			{
				wcscpy(lf.lfFaceName, L"Arial");

				font = m_fc.Create(m_dc, lf);

				if(font == NULL)
				{
					ASSERT(0);

					continue;
				}
			}

			HGDIOBJ old = SelectObject(m_dc, (HGDIOBJ)*font);

			const TEXTMETRIC& tm = font->GetTextMetric();

			if(t.style.content == L"text")
			{
				for(LPCWSTR c = t.str.c_str(); *c; c++)
				{
					Glyph* g = new Glyph();

					g->c = c[0];
					g->style = t.style;
					g->scale = scale;
					g->vertical = vertical;
					g->font = font;

					c_next = c[1] == 0 && i != s->m_text.end() ? i->str[0] : c[1];

					Arabic::Replace(g->c, c_prev, c_next);

					c_prev = c[0];

					Vector2i extent;

					font->GetTextExtentPoint(m_dc, g->c, extent);

					if(vertical) 
					{
						g->spacing = (int)(t.style.font.spacing * scale.y + 0.5f);
						g->ascent = (int)(t.style.font.scale.cx * extent.x / 2);
						g->descent = (int)(t.style.font.scale.cx * (extent.x - g->ascent));
						g->width = (int)(t.style.font.scale.cy * extent.y);

						if(g->c == ' ') // TESTME
						{
							g->width /= 2;
						}
					}
					else
					{
						g->spacing = (int)(t.style.font.spacing * scale.x + 0.5f);
						g->ascent = (int)(t.style.font.scale.cy * tm.tmAscent);
						g->descent = (int)(t.style.font.scale.cy * tm.tmDescent);
						g->width = (int)(t.style.font.scale.cx * extent.x);
					}

					if(g->c == '\n')
					{
						g->spacing = 0;
						g->width = 0;
						g->ascent /= 2;
						g->descent /= 2;
					}
					else
					{
						GlyphPath* path = m_gpc.Create(m_dc, font, g->c);
						
						if(path == NULL) {ASSERT(0); delete g; continue;}

						g->path = *path;
					}

					glyphs.push_back(g);
				}
			}
			else if(t.style.content == L"polygon")
			{
				Glyph* g = new Glyph();

				g->c = 0;
				g->style = t.style;
				g->scale = scale;
				g->vertical = vertical;
				g->font = font;

				g->ParsePolygon(t.str.c_str());

				glyphs.push_back(g);
			}

			SelectObject(m_dc, old);
		}

		// break glyphs into rows

		std::list<Row*> rows;

		Row* row = NULL;

		for(auto i = glyphs.begin(); i != glyphs.end(); i++)
		{
			Glyph* g = *i;

			if(row == NULL) 
			{
				row = new Row();
			}
			
			row->push_back(g);

			if(g->c == '\n' || g == glyphs.back()) 
			{
				rows.push_back(row);

				row = NULL;
			}
		}

		ASSERT(row == NULL);

		glyphs.clear();

		// kerning

		if(s->m_direction.primary == L"right") // || s->m_direction.primary == L"left"
		{
			for(auto i = rows.begin(); i != rows.end(); i++)
			{
				Row* r = *i;

				for(auto i = r->begin(); i != r->end(); )
				{
					Glyph* g1 = *i++;

					if(i != r->end())
					{
						Glyph* g2 = *i;

						if(g1->font == g2->font && g1->style.font.kerning && g2->style.font.kerning)
						{
							int size = g1->font->GetKernAmount(g1->c, g2->c);

							if(size != 0)
							{
								g2->path.MovePoints(Vector2i(size, 0));
								g2->width += size;
							}
						}
					}
				}
			}				
		}

		// wrap rows

		if(s->m_wrap == L"normal" || s->m_wrap == L"even")
		{
			int maxwidth = abs((int)(vertical ? frame.height() : frame.width()));
			int minwidth = 0;

			for(auto i = rows.begin(); i != rows.end(); i++)
			{
				Row* r = *i;

				if(s->m_wrap == L"even")
				{
					int fullwidth = 0;

					for(auto i = r->begin(); i != r->end(); i++)
					{
						const Glyph* g = *i;

						fullwidth += g->width + g->spacing;
					}

					if(fullwidth < 0)
					{
						fullwidth = -fullwidth;
					}
					
					if(fullwidth > maxwidth)
					{
						maxwidth = fullwidth / (fullwidth / maxwidth + 1);
						minwidth = maxwidth;
					}
				}

				int width = 0;

				for(auto j = r->begin(), br = r->end(); j != r->end(); )
				{
					const Glyph* g = *j++;

					width += g->width + g->spacing;

					if(br != r->end() && abs(width) > maxwidth && g->c != ' ')
					{
						row = new Row();
						row->splice(row->end(), *r, r->begin(), br);
						rows.insert(i, row);

						br = r->end();
						j = r->begin();

						g = *j++;

						width = g->width + g->spacing;
					}

					if(abs(width) >= minwidth)
					{
						if(g->style.linebreak == L"char" || g->style.linebreak == L"word" && g->c == ' ')
						{
							br = j;
						}
					}
				}
			}
		}

		// trim rows

		for(auto i = rows.begin(); i != rows.end(); i++)
		{
			Row* r = *i;

			while(!r->empty() && r->front()->c == ' ') {delete r->front(); r->pop_front();}
			while(!r->empty() && r->back()->c == ' ') {delete r->back(); r->pop_back();}
		}

		// assign row numbers to glyphs

		int row_num = 0;

		for(auto i = rows.begin(); i != rows.end(); i++, row_num++)
		{
			Row* r = *i;

			for(auto i = r->begin(); i != r->end(); i++)
			{
				Glyph* g = *i;

				g->row = row_num;
			}
		}

		// calc fill width for each glyph

		std::list<Glyph*> glyphs2fill;

		int fill_id = 0;
		int fill_width = 0;

		for(auto i = rows.begin(); i != rows.end(); i++)
		{
			Row* r = *i;

			for(auto i = r->begin(); i != r->end(); i++)
			{
				Glyph* g = *i;

				if(g->style.fill.id != fill_id)
				{
					Fill(glyphs2fill, fill_width);

					glyphs2fill.clear();

					fill_id = g->style.fill.id;
					fill_width = 0;
				}

				if(g->style.fill.id != 0)
				{
					glyphs2fill.push_back(g);

					fill_width += g->width;
				}
			}
		}

		Fill(glyphs2fill, fill_width);

		// calc row sizes and total subtitle size

		Vector2i size(0, 0);

		if(s->m_direction.secondary == L"left" || s->m_direction.secondary == L"up")
		{
			rows.reverse();
		}

		for(auto i = rows.begin(); i != rows.end(); i++)
		{
			Row* r = *i;

			if(s->m_direction.primary == L"left" || s->m_direction.primary == L"up")
			{
				r->reverse();
			}

			int w = 0, h = 0;

			r->width = 0;

			for(auto i = r->begin(); i != r->end(); i++)
			{
				Glyph* g = *i;

				w += g->width;

				if(g != r->back()) 
				{
					w += g->spacing;
				}
				
				h = max(h, g->ascent + g->descent);

				r->width += g->width;

				if(g != r->back()) 
				{
					r->width += g->spacing;
				}
				
				r->ascent = std::max<int>(r->ascent, g->ascent);
				r->descent = std::max<int>(r->descent, g->descent);
				r->border = std::max<int>(r->border, (int)g->GetBackgroundSize());
			}

			for(auto i = r->begin(); i != r->end(); i++)
			{
				Glyph* g = *i;

				g->row_ascent = r->ascent;
				g->row_descent = r->descent;
			}

			if(vertical)
			{
				size.x += h;
				size.y = std::max<int>(size.y, w);
			}
			else
			{
				size.x = std::max<int>(size.x, w);
				size.y += h;
			}
		}

		// align rows and calc glyph positions

		rs = new RenderedSubtitle(bmrc, clip);

		Vector2i p = GetAlignPoint(style.placement, scale, frame, size);
		Vector2i o = GetAlignPoint(style.placement, scale, frame);

		// collision detection

		if(/*!s->m_animated &&*/ style.placement.pos.auto_x && style.placement.pos.auto_y && style.placement.org.auto_x && style.placement.org.auto_y)
		{
			int tlb = !rows.empty() ? rows.front()->border : 0;
			int brb = !rows.empty() ? rows.back()->border : 0;

			Vector4i r(p.x, p.y, p.x + size.x, p.y + size.y);

			m_sra->GetRect(r, s, style.placement.align, tlb, brb);
			
			o.x += r.left - p.x;
			o.y += r.top - p.y;

			p = r.tl;
		}

		Vector4i subrect(p.x, p.y, p.x + size.x, p.y + size.y);

		// continue positioning

		for(auto i = rows.begin(); i != rows.end(); i++)
		{
			Row* r = *i;

			Vector2i rsize(r->width, r->width);

			int a = !vertical ? 0 : 1;
			int b = !vertical ? 1 : 0;

			p.v[a] = GetAlignPoint(style.placement, scale, frame, rsize).v[a];

			for(auto i = r->begin(); i != r->end(); i++)
			{
				Glyph* g = *i;

				g->tl.x = p.x + (int)(g->style.placement.offset.x * scale.x + 0.5f);
				g->tl.y = p.y + (int)(g->style.placement.offset.y * scale.y + 0.5f);

				g->tl.v[b] += r->ascent - g->ascent;
				
				p.v[a] += g->width + g->spacing;

				rs->m_glyphs.push_back(g);
			}

			p.v[b] += r->ascent + r->descent;

			r->clear();

			delete r;
		}

		// bkg, precalc style.placement.path, transform

		for(auto i = rs->m_glyphs.begin(); i != rs->m_glyphs.end(); i++)
		{
			Glyph* g = *i;

			g->CreateBkg();
			g->CreateSplineCoeffs(bmrc);
			g->Transform(o, subrect);
		}

		// merge glyphs

		Glyph* g0 = NULL;

		for(auto i = rs->m_glyphs.begin(); i != rs->m_glyphs.end(); )
		{
			auto j = i++;

			Glyph* g = *j;

			Vector4i r = g->bbox + Vector4i(g->tl, g->tl);

			int size = (int)(g->GetBackgroundSize() + g->GetShadowDepth() + 0.5f);

			r = (r + Vector4i(-size, size + 32).xxyy()).sra32(6);

			if(s->m_clip && r.rintersect(clip).rempty()) // clip
			{
				delete g;

				rs->m_glyphs.erase(j);
			}
			else if(g0 && g0->style.IsSimilar(g->style) && (g0->style.fill.id == 0 || g0->row == g->row)) // append (TODO: leave rows separate when filling)
			{
				Vector2i o(g->tl.x - g0->tl.x, g->tl.y - g0->tl.y);

				g->path.MovePoints(o);
				g->path_bkg.MovePoints(o);

				g0->path += g->path;
				g0->path_bkg += g->path_bkg;

				g0->bbox = g0->bbox.runion(g->bbox + Vector4i(o, o));

				g0->width += g->width;

				delete g;

				rs->m_glyphs.erase(j);
			}
			else // leave alone
			{
				g0 = g;
			}
		}

		// rasterize

		if(rasterize)
		{
			rs->Rasterize();
		}

		// cache

		m_rsc.Add(s->m_name, rs);

		return rs;
	}

	Vector4i Renderer::Draw(SubtitleFile& sf, float at, Bitmap& bm, const Vector2i& vs, const Vector4i& vr)
	{
		std::list<SSF::Subtitle*> subs;

		sf.Lookup(at, subs);

		NextSegment(subs);

		Vector4i bbox = Vector4i::zero();

		for(auto i = subs.begin(); i != subs.end(); i++)
		{
			SSF::Subtitle* s = *i;

			if(SSF::RenderedSubtitle* rs = Lookup(s, vs, vr))
			{
				Vector4i r = rs->Draw(bm);

				bbox = bbox.runion(r);
			}

			delete s;
		}

		return bbox;
	}

	void Renderer::Fill(const std::list<Glyph*>& glyphs, int width)
	{
		if(!glyphs.empty())
		{
			Glyph* g = glyphs.front();

			if(g->style.fill.id != 0)
			{
				int w = (int)(g->style.fill.width * width + 0.5f);

				for(auto i = glyphs.rbegin(); i != glyphs.rend(); i++)
				{
					width -= (*i)->width;

					(*i)->fill = w - width;
				}
			}
		}
	}

	//

	void RenderedSubtitle::Rasterize()
	{
		for(auto i = m_glyphs.begin(); i != m_glyphs.end(); i++)
		{
			(*i)->Rasterize();
		}
	}

	Vector4i RenderedSubtitle::Measure() const
	{
		Vector4i r = Vector4i::zero();

		for(auto i = m_glyphs.begin(); i != m_glyphs.end(); i++)
		{
			r = r.runion((*i)->GetClipRect());
		}

		return r;
	}

	Vector4i RenderedSubtitle::Draw(Bitmap& bm) const
	{
		Vector4i bbox = Vector4i::zero();

		// shadow

		for(auto i = m_glyphs.begin(); i != m_glyphs.end(); i++)
		{
			Glyph* g = *i;

			if(g->style.shadow.depth > 0)
			{
				DWORD color = g->style.shadow.color;
				DWORD sw[2] = {color, INT_MAX};

				bool outline = g->style.background.type == L"outline" && g->style.background.size > 0;

				Vector4i r = g->ras.shadow.Draw(bm, m_clip, g->tls.x, g->tls.y, sw, outline ? 1 : 0);

				bbox = bbox.runion(r);
			}
		}

		// background

		for(auto i = m_glyphs.begin(); i != m_glyphs.end(); i++)
		{
			Glyph* g = *i;

			if(g->style.background.size >= 0)
			{
				DWORD color = g->style.background.color;
				DWORD sw[2] = {color, INT_MAX};

				Vector4i r;

				if(g->style.background.type == L"outline" && g->style.background.size > 0)
				{
					r = g->ras.fg.Draw(bm, m_clip, g->tl.x, g->tl.y, sw, g->style.font.color.a < 255 ? 2 : 1);
				}
				else if(g->style.background.type == L"enlarge" && g->style.background.size > 0 || g->style.background.type == L"box")
				{
					r = g->ras.bg.Draw(bm, m_clip, g->tl.x, g->tl.y, sw, 0);
				}
				else
				{
					continue;
				}

				bbox = bbox.runion(r);
			}
		}

		// body

		for(auto i = m_glyphs.begin(); i != m_glyphs.end(); i++)
		{
			Glyph* g = *i;

			DWORD color = g->style.font.color;
			DWORD sw[4] = {color, INT_MAX};

			if(g->style.fill.id != 0)
			{
				if(g->fill > 0)
				{
					if(g->fill < g->width)
					{
						sw[0] = g->style.font.color;
						sw[1] = g->fill >> 6;
						sw[2] = g->style.fill.color;
						sw[3] = INT_MAX;
					}
					else
					{
						sw[0] = g->style.font.color;
						sw[1] = INT_MAX;
					}
				}
				else
				{
					sw[0] = g->style.fill.color;
					sw[1] = INT_MAX;
				}
			}

			Vector4i r = g->ras.fg.Draw(bm, m_clip, g->tl.x, g->tl.y, &sw[0], 0);

			bbox = bbox.runion(r);
		}

		return bbox;
	}

	//

	void SubRectAllocator::UpdateTarget(const Vector2i& s, const Vector4i& r)
	{
		if(vs != s || !vr.eq(r)) 
		{
			RemoveAll();
		}

		vs = s;
		vr = r;
	}
	
	void SubRectAllocator::GetRect(Vector4i& rect, const Subtitle* s, const Align& align, int tlb, int brb)
	{
		SubRect* sr = new SubRect(rect, s->m_layer);

		sr->rect += Vector4i(-tlb, brb).xxyy();

		auto i = rects.find(s->m_name);

		if(i != rects.end() && !i->second->rect.eq(sr->rect))
		{
			rects.erase(i);

			i = rects.end();
		}

		if(i != rects.end())
		{
			return;
		}

		bool vertical = s->m_direction.primary == L"down" || s->m_direction.primary == L"up";

		bool ok = false;

		while(!ok)
		{
			ok = true;

			for(auto i = rects.begin(); i != rects.end(); i++)
			{
				const SubRect* sr2 = i->second;

				if(sr->layer == sr2->layer && !sr->rect.rintersect(sr2->rect).rempty())
				{
					if(vertical)
					{
						if(align.h < 0.5f)
						{
							sr->rect.right = sr2->rect.right + sr->rect.width();
							sr->rect.left = sr2->rect.right;
						}
						else
						{
							sr->rect.left = sr2->rect.left - sr->rect.width();
							sr->rect.right = sr2->rect.left;
						}
					}
					else
					{
						if(align.v < 0.5f)
						{
							sr->rect.bottom = sr2->rect.bottom + sr->rect.height();
							sr->rect.top = sr2->rect.bottom;
						}
						else
						{
							sr->rect.top = sr2->rect.top - sr->rect.height();
							sr->rect.bottom = sr2->rect.top;
						}
					}

					ok = false;
				}
			}
		}

		rects[s->m_name] = sr;

		rect = sr->rect + Vector4i(tlb, -brb).xxyy();
	}

	//

	FontWrapper* FontCache::Create(HDC dc, const LOGFONT& lf)
	{
		int flags = ((lf.lfItalic & 1) << 2) | ((lf.lfUnderline & 1) << 1) | ((lf.lfStrikeOut & 1) << 0);

		std::wstring key(lf.lfFaceName);

		key += Util::Format(L":%d:%d:%d", lf.lfHeight, lf.lfWeight, flags);

		auto i = m_map.find(key);

		if(i != m_map.end())
		{
			return i->second;
		}

		HFONT font = CreateFontIndirect(&lf);

		if(font == NULL)
		{
			return NULL;
		}

		FontWrapper* fw = new FontWrapper(dc, font, key.c_str());

		Add(key, fw, false);

		return fw;
	}

	//

	GlyphPath* GlyphPathCache::Create(HDC dc, const FontWrapper* f, WCHAR c)
	{
		std::wstring key = std::wstring(*f) + c;

		auto i = m_map.find(key);

		if(i != m_map.end())
		{
			return i->second;
		}

		BeginPath(dc);

		TextOutW(dc, 0, 0, &c, 1);

		CloseFigure(dc);

		if(!EndPath(dc)) 
		{
			AbortPath(dc); 

			return NULL;
		}

		GlyphPath* path = new GlyphPath();

		int count = GetPath(dc, NULL, NULL, 0);

		if(count > 0)
		{
			path->SetCount(count);

			if(count != GetPath(dc, (POINT*)path->m_points, path->m_types, count))
			{
				delete path;

				return NULL;
			}
		}

		Add(key, path);

		return path;
	}
}