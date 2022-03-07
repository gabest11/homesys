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
 *  Based on the rasterizer of virtualdub's subtitler plugin
 */

#include "stdafx.h"
#include "Rasterizer.h"
#include "Glyph.h"

#define FONT_AA 3
#define FONT_SCALE (6 - FONT_AA)

// FONT_AA: 0 - 3

namespace SSF
{
	Rasterizer::Rasterizer()
	{
		memset(&m_overlay, 0, sizeof(m_overlay));
	}

	Rasterizer::~Rasterizer()
	{
		TrashOverlay();
	}

	void Rasterizer::TrashOverlay()
	{
		for(int i = 0; i < sizeof(m_overlay.buff) / sizeof(m_overlay.buff[0]); i++)
		{
			if(m_overlay.buff[i] != NULL)
			{
				_aligned_free(m_overlay.buff[i]);

				m_overlay.buff[i] = NULL;
			}
		}
	}

	void Rasterizer::ReallocEdgeBuffer(int edges)
	{
		m_edge.size = edges;
		m_edge.buff = (Edge*)_aligned_realloc(m_edge.buff, sizeof(Edge) * edges, 16);
	}

	void Rasterizer::EvaluateLine(const Vector4i& v01)
	{
		Vector4i v = v01;

		if(m_last != v.tl)
		{
			EvaluateLine(Vector4i(m_last, v.tl));
		}

		if(!m_first_set)
		{
			m_first = v.tl;
			m_first_set = true;
		}

		m_last = v.br;

		// TODO: ((1 << FONT_SCALE) / 2 +- 1)  

		if(v.bottom > v.top) // down
		{
			int xacc = v.left << (8 - FONT_SCALE);

			// prestep v.top down

			int dy = v.height();
			int y = ((v.top + ((1 << FONT_SCALE) / 2 - 1)) & ~((1 << FONT_SCALE) - 1)) + (1 << FONT_SCALE) / 2;
			int iy = y >> FONT_SCALE;

			v.bottom = (v.bottom - ((1 << FONT_SCALE) / 2 + 1)) >> FONT_SCALE;

			if(iy <= v.bottom)
			{
				int invslope = (v.width() << 8) / dy;

				while(m_edge.next + v.bottom + 1 - iy > m_edge.size)
				{
					ReallocEdgeBuffer(m_edge.size * 2);
				}

				xacc += (invslope * (y - v.top)) >> FONT_SCALE;

				while(iy <= v.bottom)
				{
					int ix = (xacc + 128) >> 8;

					m_edge.buff[m_edge.next].next = m_edge.scan[iy];
					m_edge.buff[m_edge.next].posandflag = ix * 2 + 1;

					m_edge.scan[iy] = m_edge.next++;

					iy++;
					xacc += invslope;
				}
			}
		}
		else if(v.bottom < v.top) // up
		{
			int xacc = v.right << (8 - FONT_SCALE);

			// prestep v.bottom down

			int dy = -v.height();
			int y = ((v.bottom + ((1 << FONT_SCALE) / 2 - 1)) & ~((1 << FONT_SCALE) - 1)) + (1 << FONT_SCALE) / 2;
			int iy = y >> FONT_SCALE;

			v.top = (v.top - ((1 << FONT_SCALE) / 2 + 1)) >> FONT_SCALE;

			if(iy <= v.top)
			{
				int invslope = (-v.width() << 8) / dy;

				while(m_edge.next + v.top + 1 - iy > m_edge.size)
				{
					ReallocEdgeBuffer(m_edge.size * 2);
				}

				xacc += (invslope * (y - v.bottom)) >> FONT_SCALE;

				while(iy <= v.top)
				{
					int ix = (xacc + 128) >> 8;

					m_edge.buff[m_edge.next].next = m_edge.scan[iy];
					m_edge.buff[m_edge.next].posandflag = ix * 2;

					m_edge.scan[iy] = m_edge.next++;

					iy++;
					xacc += invslope;
				}
			}
		}
	}

	void Rasterizer::EvaluateBezier(const Vector4i& v0, const Vector4i& v1)
	{
		// v0: p0 (0) c0 (1)
		// v1: c1 (3) p1 (2)

		Vector4i v2 = v0.uph64(v1.zwxy()); // v2: c0 c1

		int s = (v0 + v1 - (v2 << 1)).abs32().sum32();

		if(s <= max(2, 1 << FONT_AA))
		{
			EvaluateLine(v0.upl64(v1.zwxy()));
		}
		else
		{
			// bezier subdivision

			Vector4i one = Vector4i::x00000001();

			Vector4i a = (v0 + v2 + one).sra32(1); // 01 12
			Vector4i b = (v2 + v1 + one).sra32(1); // 12 23
			Vector4i c = (a + b + one).sra32(1); // 012 123
			Vector4i d = (c + c.zwxy() + one).sra32(1); // 0123

			Vector4i v00 = v0.upl64(a); // 0 01
			Vector4i v01 = c.upl64(d); // 012 0123
			Vector4i v10 = d.uph64(c); // 0123 123
			Vector4i v11 = b.uph64(v1); // 23 3

			EvaluateBezier(v00, v01);
			EvaluateBezier(v10, v11);
		}
	}

	static const Vector4 bspline[5] = 
	{
		Vector4(-1, +3, -3, +1) / 6,
		Vector4(+3, -6,  0, +4) / 6,
		Vector4(-3, +3, +3, +1) / 6,
		Vector4(+1,  0,  0,  0) / 6,
		Vector4(+6, +2,  0,  0),
	};

	void Rasterizer::EvaluateBSpline(const Vector2i* v)
	{
		Vector4 v01 = Vector4(Vector4i::load<false>(v + 0));
		Vector4 v23 = Vector4(Vector4i::load<false>(v + 2));

		Vector4 cx = v01.xxxx() * bspline[0] + v01.zzzz() * bspline[1] + v23.xxxx() * bspline[2] + v23.zzzz() * bspline[3];
		Vector4 cy = v01.yyyy() * bspline[0] + v01.wwww() * bspline[1] + v23.yyyy() * bspline[2] + v23.wwww() * bspline[3];

		if(!m_first_set)
		{
			Vector4i first(cx.uph(cy));
			Vector4i::storeh(&m_first, first);
			Vector4i::storeh(&m_last, first);
			m_first_set = true;
		}

		// This equation is from Graphics Gems I.
		//
		// The idea is that since we're approximating a cubic curve with lines,
		// any error we incur is due to the curvature of the line, which we can
		// estimate by calculating the maximum acceleration of the curve.  For
		// a cubic, the acceleration (second derivative) is a line, meaning that
		// the absolute maximum acceleration must occur at either the beginning
		// (|c2|) or the end (|c2+c3|).  Our bounds here are a little more
		// conservative than that, but that's okay.
		//
		// If the acceleration of the parametric formula is zero (c2 = c3 = 0),
		// that component of the curve is linear and does not incur any error.
		// If a=0 for both X and Y, the curve is a line segment and we can
		// use a step size of 1.

		Vector4 maxaccel1 = (cx.abs() * bspline[4]).hadd();
		Vector4 maxaccel2 = (cy.abs() * bspline[4]).hadd();

		float maxaccel = maxaccel1.blend8(maxaccel2, maxaccel1 < maxaccel2).x;

		float h = maxaccel > 8.0f ? sqrt(8.0f / maxaccel) : 1.0f;

		for(float t = 0; t < 1.0f; t += h)
		{
			float x = ((cx.x * t + cx.y) * t + cx.z) * t + cx.w;
			float y = ((cy.x * t + cy.y) * t + cy.z) * t + cy.w;

			EvaluateLine(Vector4i(m_last, Vector2i((int)x, (int)y)));
		}

		EvaluateLine(Vector4i(m_last).upl64(Vector4i(cx.hadd(cy).hadd()))); // t = 1.0f
	}

	void Rasterizer::OverlapRegion(Array<Vector4i>& dst, Array<Vector4i>& src, int dx, int dy)
	{
		m_outline.tmp.Move(dst);

		Vector4i* RESTRICT a = m_outline.tmp.GetData();
		Vector4i* RESTRICT ae = a + m_outline.tmp.GetCount();
		Vector4i* RESTRICT b = src.GetData();
		Vector4i* RESTRICT be = b + src.GetCount();

		Vector4i oy = Vector4i(0, dy);
		Vector4i ox = Vector4i(dx, 0);
		Vector4i o = oy.sub64(ox).upl64(oy.add64(ox));

		while(a != ae && b != be)
		{
			Vector4i x = o.add64(*b);

			if(x.u64[0] < a->u64[0])
			{
				// B span is earlier.  Use it.

				b++;

				// B spans don't overlap, so begin merge loop with A first.

				for(;;)
				{
					// If we run out of A spans or the A span doesn't overlap,
					// then the next B span can't either (because B spans don't
					// overlap) and we exit.

					if(a == ae || a->u64[0] > x.u64[1])
					{
						break;
					}

					do {x.u64[1] = std::max<UINT64>(x.u64[1], a->u64[1]);}
					while(++a != ae && a->u64[0] <= x.u64[1]);

					// If we run out of B spans or the B span doesn't overlap,
					// then the next A span can't either (because A spans don't
					// overlap) and we exit.

					Vector4i x2 = o.add64(*b);

					if(b == be || x2.u64[0] > x.u64[1])
					{
						break;
					}

					do {x.u64[1] = std::max<UINT64>(x.u64[1], x2.u64[1]);}
					while(++b != be && (x2 = o.add64(*b)).u64[0] <= x.u64[1]);
				}
			}
			else
			{
				// A span is earlier.  Use it.

				x = *a;

				a++;

				// A spans don't overlap, so begin merge loop with B first.

				for(;;)
				{
					// If we run out of B spans or the B span doesn't overlap,
					// then the next A span can't either (because A spans don't
					// overlap) and we exit.

					Vector4i x2 = o.add64(*b);

					if(b == be || x2.u64[0] > x.u64[1])
					{
						break;
					}

					do {x.u64[1] = std::max<UINT64>(x.u64[1], x2.u64[1]);}
					while(++b != be && (x2 = o.add64(*b)).u64[0] <= x.u64[1]);

					// If we run out of A spans or the A span doesn't overlap,
					// then the next B span can't either (because B spans don't
					// overlap) and we exit.

					if(a == ae || a->u64[0] > x.u64[1])
					{
						break;
					}

					do {x.u64[1] = std::max<UINT64>(x.u64[1], a->u64[1]);}
					while(++a != ae && a->u64[0] <= x.u64[1]);
				}
			}

			// Flush span.

			dst.Add(x);
		}

		// Copy over leftover spans.

		dst.Append(a, ae - a);

		for(; b != be; b++)
		{
			dst.Add(o.add64(*b));
		}
	}

	bool Rasterizer::ScanConvert(GlyphPath& path, const Vector4i& bbox)
	{
		// Drop any outlines we may have.

		m_outline.normal.RemoveAll();
		m_outline.wide.RemoveAll();

		if(path.m_count == 0 || bbox.rempty())
		{
			m_size.x = m_size.y = 0;
			m_overlay.path.x = m_overlay.path.y = 0;
			return 0;
		}

		int minx = (bbox.left >> FONT_SCALE) & ~((1 << FONT_SCALE) - 1);
		int miny = (bbox.top >> FONT_SCALE) & ~((1 << FONT_SCALE) - 1);
		int maxx = (bbox.right + ((1 << FONT_SCALE) - 1)) >> FONT_SCALE;
		int maxy = (bbox.bottom + ((1 << FONT_SCALE) - 1)) >> FONT_SCALE;

		path.MovePoints(Vector2i(-minx * (1 << FONT_SCALE), -miny * (1 << FONT_SCALE)));

		if(minx > maxx || miny > maxy)
		{
			m_size.x = m_size.y = 0;
			m_overlay.path.x = m_overlay.path.y = 0;
			return true;
		}

		m_size.x = maxx + 1 - minx;
		m_size.y = maxy + 1 - miny;

		m_overlay.path.x = minx;
		m_overlay.path.y = miny;

		// Initialize edge buffer.  We use edge 0 as a sentinel.

		m_edge.next = 1;
		m_edge.size = 0x10000;
		m_edge.buff = (Edge*)_aligned_malloc(sizeof(Edge) * m_edge.size, 16);

		// Initialize scanline list.

		m_edge.scan = new unsigned int[m_size.y];

		memset(m_edge.scan, 0, m_size.y * sizeof(unsigned int));

		// Scan convert the outline.  Yuck, Bezier curves....

		// Unfortunately, Windows 95/98 GDI has a bad habit of giving us text
		// paths with all but the first figure left open, so we can't rely
		// on the PT_CLOSEFIGURE flag being used appropriately.

		m_first_set = false;

		m_first.x = m_first.y = 0;
		m_last.x = m_last.y = 0;

		int lastmoveto = -1;

		BYTE* t = path.m_types;
		Vector2i* p = path.m_points;

		for(int i = 0, j = path.m_count; i < j; i++)
		{
			switch(t[i] & ~PT_CLOSEFIGURE)
			{
			case PT_MOVETO:
				if(lastmoveto >= 0 && m_first != m_last) EvaluateLine(Vector4i(m_last, m_first));
				lastmoveto = i;
				m_first_set = false;
				m_last = p[i];
				break;
			case PT_MOVETONC:
				break;
			case PT_LINETO:
				if(j - (i - 1) >= 2) EvaluateLine(Vector4i::load<false>(&p[i - 1]));
				break;
			case PT_BEZIERTO:
				if(j - (i - 1) >= 4) EvaluateBezier(Vector4i::load<false>(&p[i - 1]), Vector4i::load<false>(&p[i + 1]));
				i += 2;
				break;
			case PT_BSPLINETO:
				if(j - (i - 1) >= 4) EvaluateBSpline(&p[i - 1]);
				i += 2;
				break;
			case PT_BSPLINEPATCHTO:
				if(j - (i - 3) >= 4) EvaluateBSpline(&p[i - 3]);
				break;
			}
		}

		if(lastmoveto >= 0 && m_first != m_last) 
		{
			EvaluateLine(Vector4i(m_last, m_first));
		}

		// Convert the edges to spans.  We couldn't do this before because some of
		// the regions may have winding numbers >+1 and it would have been a pain
		// to try to adjust the spans on the fly.  We use one heap to detangle
		// a scanline's worth of edges from the singly-linked lists, and another
		// to collect the actual scans.

		std::vector<int> heap;

		if(m_size.y > 0)
		{
			heap.reserve(m_edge.next / m_size.y);
		}

		m_outline.normal.SetCount(0, m_edge.next / 2);

		for(int y = 0; y < m_size.y; y++)
		{
			int count = 0;

			// Detangle scanline into edge heap.

			for(unsigned int ptr = m_edge.scan[y]; ptr; ptr = m_edge.buff[ptr].next)
			{
				heap.push_back(m_edge.buff[ptr].posandflag);
			}

			// Sort edge heap.  Note that we conveniently made the opening edges
			// one more than closing edges at the same spot, so we won't have any
			// problems with abutting spans.

			std::sort(heap.begin(), heap.end());

			// Process edges and add spans.  Since we only check for a non-zero
			// winding number, it doesn't matter which way the outlines go!

			int x1, x2;

			for(auto i = heap.begin(); i != heap.end(); i++)
			{
				int x = *i;

				if(count == 0) 
				{
					x1 = x >> 1;
				}

				if(x & 1) 
				{
					count++;
				}
				else 
				{
					count--;
				}

				if(count == 0)
				{
					x2 = x >> 1;

					if(x2 > x1)
					{
						m_outline.normal.Add(Vector4i(x1, y, x2, y).add64(Vector4i(0x40000000)));
					}
				}
			}

			heap.clear();
		}

		// Dump the edge and scan buffers, since we no longer need them.

		_aligned_free(m_edge.buff);

		delete [] m_edge.scan;

		// All done!

		return true;
	}

	bool Rasterizer::CreateWidenedRegion(int r)
	{
		if(r < 0) r = 0;

		r >>= FONT_SCALE;

		for(int y = -r; y <= r; ++y)
		{
			int x = (int)(0.5f + sqrt(float(r * r - y * y)));

			OverlapRegion(m_outline.wide, m_outline.normal, x, y);
		}

		m_border = r;

		return true;
	}

	bool Rasterizer::Rasterize(int xsub, int ysub)
	{
		TrashOverlay();

		if(m_size.x == 0 || m_size.y == 0)
		{
			m_overlay.size.x = m_overlay.size.y = 0;

			return true;
		}

		xsub >>= FONT_SCALE;
		ysub >>= FONT_SCALE;

		xsub &= (1 << FONT_AA) - 1;
		ysub &= (1 << FONT_AA) - 1;

		int width = m_size.x + xsub;
		int height = m_size.y + ysub;

		m_overlay.offset.x = m_overlay.path.x - xsub;
		m_overlay.offset.y = m_overlay.path.y - ysub;

		int border = ((m_border + ((1 << FONT_AA) - 1)) & ~((1 << FONT_AA) - 1)) + (1 << FONT_AA) * 4;

		if(!m_outline.wide.IsEmpty())
		{
			width += 2 * border;
			height += 2 * border;

			xsub += border;
			ysub += border;

			m_overlay.offset.x -= border;
			m_overlay.offset.y -= border;
		}

		m_overlay.size.x = ((width + ((1 << FONT_AA) - 1)) >> FONT_AA) + 1;
		m_overlay.size.y = ((height + ((1 << FONT_AA) - 1)) >> FONT_AA) + 1;

		int size = m_overlay.size.x * m_overlay.size.y;

		for(int i = 0; i < sizeof(m_overlay.buff) / sizeof(m_overlay.buff[0]); i++)
		{
			m_overlay.buff[i] = (BYTE*)_aligned_malloc(size, 16);

			memset(m_overlay.buff[i], 0, size);
		}

		Vector4i o = Vector4i(0x40000000 - xsub, 0x40000000 - ysub).xyxy();

		Array<Vector4i>* outline[] = {&m_outline.normal, &m_outline.wide};
		
		for(int i = 0; i < sizeof(outline) / sizeof(outline[0]); i++)
		{
			const Vector4i* RESTRICT s = outline[i]->GetData();

			BYTE* RESTRICT buff = m_overlay.buff[i];

			for(int j = 0, k = outline[i]->GetCount(); j < k; j++)
			{
				Vector4i v = s[j] - o;

				int x1 = v.extract32<0>();
				int x2 = v.extract32<2>();
				int y = v.extract32<1>();
				int w = x2 - x1;

				if(w > 0)
				{
					int first = x1 >> FONT_AA;
					int last = (x2 - 1) >> FONT_AA;

					BYTE* RESTRICT dst = &buff[m_overlay.size.x * (y >> FONT_AA)];

					if(first == last)
					{
						dst[first] += w;
					}
					else
					{
						dst[first] += (((first + 1) << FONT_AA) - x1) << (6 - FONT_AA * 2);

						first++;
						
						for(; first < last; first++)
						{
							dst[first] += (1 << FONT_AA) << (6 - FONT_AA * 2);
						}

						dst[first] += (x2 - (last << FONT_AA)) << (6 - FONT_AA * 2);
					}
				}
			}
		}

		if(!m_outline.wide.IsEmpty())
		{
			BYTE* RESTRICT p0 = m_overlay.buff[0];
			BYTE* RESTRICT p1 = m_overlay.buff[1];
			BYTE* RESTRICT p2 = m_overlay.buff[2];

			int size = m_overlay.size.x * m_overlay.size.y;

			int i = 0;

			Vector4i v2(0x40404040);

			for(int j = size & ~15; i < j; i += 16)
			{
				Vector4i v0 = Vector4i::load<true>(&p0[i]);
				Vector4i v1 = Vector4i::load<true>(&p1[i]);

				Vector4i::store<true>(&p2[i], v1.min_u8(v2.subus8(v0)));
			}

			for(; i < size; i++)
			{
				p2[i] = std::min<BYTE>(p1[i], (1 << 6) - p0[i]);
			}
		}

		return true;
	}

	void Rasterizer::Blur(float n, int plane)
	{
		// TODO: sse

		if(n <= 0 || m_overlay.size.x == 0 || m_overlay.size.y == 0 || m_overlay.buff[plane] == NULL)
		{
			return;
		}

		int w = m_overlay.size.x;
		int h = m_overlay.size.y;

		BYTE* q0 = new BYTE[w * h];

		for(int pass = 0; pass < n; pass++)
		{
			BYTE* RESTRICT p = m_overlay.buff[plane];
			BYTE* RESTRICT q = q0;

			for(int y = 0; y < h; y++, p += w, q += w)
			{
				q[0] = (p[0] + p[1] + 1) >> 1;

				int x = 0;
				
				for(x = 1; x < w - 1; x++)
				{
					q[x] = (p[x - 1] + 2 * p[x] + p[x + 1] + 2) >> 2;
				}

				q[x] = (p[x - 1] + p[x] + 1) >> 1;
			}

			p = m_overlay.buff[plane];
			q = q0;

			for(int x = 0; x < w; x++, p++, q++)
			{
				p[0] = (q[0] + q[w] + 1) >> 1;

				int y = 0, yp, yq;
				
				for(y = 1, yp = y * w, yq = y * w; y < h - 1; y++, yp += w, yq += w)
				{
					p[yp] = (q[yq - w] + 2 * q[yq] + q[yq + w] + 2) >> 2;
				}

				p[yp] = (q[yq - w] + q[yq]) >> 1;
			}
		}

		delete [] q0;
	}

	void Rasterizer::Reuse(Rasterizer& r)
	{
		m_size = r.m_size;
		m_overlay.path = r.m_overlay.path;
		m_border = r.m_border;
		m_outline.normal.Move(r.m_outline.normal);
		m_outline.wide.Move(r.m_outline.wide);
	}

	static __forceinline void setcolor(const DWORD* sw, Vector4i& rb, Vector4i& ga, Vector4i& aa, Vector4i& c, DWORD& a)
	{
		Vector4i color((int)((sw[0] & 0xffffff) | 0xff000000));
		DWORD alpha = sw[0] >> 24;

		rb = color.sll16(8).srl16(8);
		ga = color.srl16(8);
		aa = Vector4i((int)(alpha | (alpha << 16)));

		c = color.upl8();
		a = alpha;
	}

	static __forceinline void blend(DWORD* RESTRICT dst, const Vector4i& c, DWORD a, DWORD f)
	{
		// C = Cs * As + Cd * (1 - As)
		// A = 1 * As + Ad * (1 - As)

		DWORD alpha = ((a * f) >> 6) & 0xff;

		Vector4i v0 = Vector4i::load((int)(*dst)).upl8().upl16(c);
		Vector4i v1 = Vector4i((int)((alpha << 16) | (0xff - alpha)));

		*dst = (DWORD)(v0.madd(v1) >> 8).rgba32();
	}

	static __forceinline void blend4(DWORD* RESTRICT dst, const Vector4i& rb, const Vector4i& ga, const Vector4i& a, DWORD f)
	{
		Vector4i v = Vector4i::load((int)f);

		Vector4i alpha = v.upl8(v).upl8().mul16l(a); // 2:14 fixed
		Vector4i color = Vector4i::load<false>(dst);

		Vector4i drb = color.sll16(8).srl16(8);
		Vector4i dga = color.srl16(8);

		drb = drb.lerp16<1>(rb, alpha);
		dga = dga.lerp16<1>(ga, alpha);

		drb = drb.pu16();
		dga = dga.pu16();

		color = drb.upl8(dga);

		Vector4i::store<false>(dst, color);
	}

	Vector4i Rasterizer::Draw(const Bitmap& bm, const Vector4i& clip, int xsub, int ysub, const DWORD* switchpts, int plane)
	{
		if(switchpts == NULL)
		{
			return Vector4i::zero();
		}

		// clip

		Vector4i br(0, 0, bm.w, bm.h);

		Vector4i r = br.rintersect(clip);

		xsub >>= FONT_SCALE;
		ysub >>= FONT_SCALE;

		int x = (xsub + m_overlay.offset.x + (1 << FONT_AA) / 2) >> FONT_AA;
		int y = (ysub + m_overlay.offset.y + (1 << FONT_AA) / 2) >> FONT_AA;
		int w = m_overlay.size.x;
		int h = m_overlay.size.y;
		int xo = 0;
		int yo = 0;

		if(x < r.left) {xo = r.left - x; w -= r.left - x; x = r.left;}
		if(y < r.top) {yo = r.top - y; h -= r.top - y; y = r.top;}
		if(x + w > r.right) w = r.right - x;
		if(y + h > r.bottom) h = r.bottom - y;

		if(w <= 0 || h <= 0) 
		{
			return Vector4i::zero();
		}

		Vector4i bbox = Vector4i(x, y, x + w, y + h).rintersect(br);

		// draw

		const BYTE* RESTRICT src = &m_overlay.buff[plane][m_overlay.size.x * yo + xo];
		DWORD* RESTRICT dst = (DWORD*)((BYTE*)bm.bits + bm.pitch * y) + x;

		Vector4i c, rb, ga, aa;
		DWORD a;

		int w4 = w & ~3;

		if(switchpts[1] == INT_MAX)
		{
			setcolor(switchpts, rb, ga, aa, c, a);

			while(h--)
			{
				int i = 0;
				
				for(; i < w4; i += 4)
				{
					blend4(&dst[i], rb, ga, aa, *(DWORD*)&src[i]);
				}

				for(; i < w; i++)
				{
					blend(&dst[i], c, a, src[i]);
				}

				src += m_overlay.size.x;
				dst = (DWORD*)((BYTE*)dst + bm.pitch);
			}
		}
		else
		{
			// TODO: blend4

			while(h--)
			{
				const DWORD* sw = switchpts;

				setcolor(sw, rb, ga, aa, c, a);

				for(int i = 0; i < w; i++)
				{
					while(i + xo >= (int)sw[1]) 
					{
						sw += 2; 

						setcolor(sw, rb, ga, aa, c, a);
					}

					blend(&dst[i], c, a, src[i]);
				}

				src += m_overlay.size.x;
				dst = (DWORD*)((BYTE*)dst + bm.pitch);
			}
		}

		return bbox;
	}
}