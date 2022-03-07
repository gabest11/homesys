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
#include "Glyph.h"
#include "Split.h"

#define d2r(d) (float)(M_PI / 180 * (d))

namespace SSF
{
	Glyph::Glyph()
	{
		c = 0;
		row = -1;
		font = NULL;
		ascent = descent = width = spacing = fill = 0;
		tl.x = tl.y = tls.x = tls.y = 0;
	}

	void Glyph::ParsePolygon(LPCWSTR polygon)
	{
		Split split(' ', polygon, Split::Min);

		std::vector<BYTE> types;
		std::vector<Vector2i> points;

		BYTE t;
		Vector2i p;

		for(int i = 0, j = 0, bspline = -1; i < split.size(); i++)
		{
			switch(split[i][0])
			{
			case 'm': 
				t = PT_MOVETO;
				break;

			case 'n': 
				t = PT_MOVETONC;
				break;

			case 'l': 
				t = PT_LINETO;
				break;

			case 'b': 
				t = PT_BEZIERTO;
				break;

			case 's': 
				t = PT_BSPLINETO;
				bspline = types.size();
				break;

			case 'p': 
				t = PT_BSPLINEPATCHTO;
				break;

			case 'c': 

				if((t == PT_BSPLINETO || t == PT_BSPLINEPATCHTO) && bspline > 0)
				{
					for(int i = 0; i < 3; i++)
					{
						types.push_back(PT_BSPLINEPATCHTO);
						p = points[bspline + i - 1];
						points.push_back(p);
					}

					bspline = -1;
				}

				break;
			
			default:
			
				p.v[j++] = split.GetAsInt(i);
				
				if(j == 2) 
				{
					if(t == PT_BSPLINETO && types.size() - bspline >= 3)
					{
						t = PT_BSPLINEPATCHTO;
					}

					types.push_back(t); 
					points.push_back(p); 

					j = 0;
				}

				break;
			}
		}

		Vector4 s = Vector4(scale);

		Vector4i pmin = Vector4i::zero();
		Vector4i pmax = Vector4i::zero();

		if(!points.empty())
		{
			Vector4 a = Vector4(points[0]) * s;
			Vector4 b = a;

			for(auto i = points.begin(); i != points.end(); i++)
			{
				Vector4 p = Vector4(*i) * s;

				a = a.minf(p);
				b = b.maxf(p);

				*i = Vector4i(p).tl;
			}

			pmin = Vector4i(a);
			pmax = Vector4i(b);
		}

		Vector4i psize = pmax - pmin;

		int baseline = 0; // TODO

		baseline = (int)(s.y * baseline);

		width = (int)(style.font.scale.cx * psize.x);
		ascent = (int)(style.font.scale.cy * (psize.y - baseline));
		descent = (int)(style.font.scale.cy * baseline);

		if(!types.empty())
		{
			path.SetCount(types.size());

			memcpy(path.m_types, types.data(), sizeof(BYTE) * types.size());
			memcpy(path.m_points, points.data(), sizeof(Vector2i) * points.size());
		}
	}

	float Glyph::GetBackgroundSize() const
	{
		return style.background.size * (style.background.scaled ? (scale.x + scale.y) / 2 : 64.0f);
	}

	float Glyph::GetShadowDepth() const
	{
		return style.shadow.depth * (style.shadow.scaled ? (scale.x + scale.y) / 2 : 64.0f);
	}

	Vector4i Glyph::GetClipRect() const
	{
		int size = (int)(GetBackgroundSize() + 0.5f) + (int)(GetShadowDepth() + 0.5f);

		return (bbox + Vector4i(tl).xyxy() + Vector4i(-size, size + 32).xxyy()).sra32(6);
	}

	void Glyph::CreateBkg()
	{
		path_bkg.RemoveAll();

		if(style.background.type == L"enlarge" && style.background.size > 0)
		{
			path_bkg.Enlarge(path, GetBackgroundSize());
		}
		else if(style.background.type == L"box" && style.background.size >= 0)
		{
			if(c != '\n')
			{
				int s = (int)(GetBackgroundSize() + 0.5f);

				int x0 = (!vertical ? -spacing / 2 : ascent - row_ascent);
				int y0 = (!vertical ? ascent - row_ascent : -spacing / 2);
				int x1 = x0 + (!vertical ? width + spacing : row_ascent + row_descent);
				int y1 = y0 + (!vertical ? row_ascent + row_descent : width + spacing);

				path_bkg.SetCount(4);
				path_bkg.m_types[0] = PT_MOVETO;
				path_bkg.m_types[1] = PT_LINETO;
				path_bkg.m_types[2] = PT_LINETO;
				path_bkg.m_types[3] = PT_LINETO;
				path_bkg.m_points[0] = Vector2i(x0 - s, y0 - s);
				path_bkg.m_points[1] = Vector2i(x1 + s, y0 - s);
				path_bkg.m_points[2] = Vector2i(x1 + s, y1 + s);
				path_bkg.m_points[3] = Vector2i(x0 - s, y1 + s);
			}
		}
	}

	void Glyph::CreateSplineCoeffs(const Vector4i& spdrc)
	{
		spline.clear();

		if(style.placement.path.empty())
		{
			return;
		}

		int i = 0;
		int j = style.placement.path.size();

		std::vector<Point> pts(j + 2);

		Point p;

		while(i < j)
		{
			p.x = style.placement.path[i].x * scale.x + spdrc.left * 64;
			p.y = style.placement.path[i].y * scale.y + spdrc.top * 64;

			pts[++i] = p;
		}

		if(pts.size() >= 4)
		{
			if(pts[1].x == pts[j].x && pts[1].y == pts[j].y)
			{
				pts[0] = pts[j - 1];
				pts[j + 1] = pts[2];
			}
			else
			{
				p.x = pts[1].x * 2 - pts[2].x;
				p.y = pts[1].y * 2 - pts[2].y;

				pts[0] = p;

				p.x = pts[j].x * 2 - pts[j - 1].x;
				p.y = pts[j].y * 2 - pts[j - 1].y;

				pts[j + 1] = p;
			}

			spline.resize(pts.size() - 3);

			for(int i = 0, j = pts.size() - 4; i <= j; i++)
			{
				static const float _1div6 = 1.0f / 6;

				SplineCoeffs sc;

				sc.cx[3] = _1div6 * (-     pts[i].x + 3 * pts[i + 1].x - 3 * pts[i + 2].x + pts[i + 3].x);
				sc.cx[2] = _1div6 * (  3 * pts[i].x - 6 * pts[i + 1].x + 3 * pts[i + 2].x);
				sc.cx[1] = _1div6 * (- 3 * pts[i].x	                   + 3 * pts[i + 2].x);
				sc.cx[0] = _1div6 * (      pts[i].x + 4 * pts[i + 1].x + 1 * pts[i + 2].x);

				sc.cy[3] = _1div6 * (-     pts[i].y + 3 * pts[i + 1].y - 3 * pts[i + 2].y + pts[i + 3].y);
				sc.cy[2] = _1div6 * (  3 * pts[i].y - 6 * pts[i + 1].y + 3 * pts[i + 2].y);
				sc.cy[1] = _1div6 * (- 3 * pts[i].y                    + 3 * pts[i + 2].y);
				sc.cy[0] = _1div6 * (      pts[i].y + 4 * pts[i + 1].y + 1 * pts[i + 2].y);

				spline[i] = sc;
			}
		}
	}

	void Glyph::Transform(GlyphPath& path, const Vector2i& org, const Vector4i& subrect)
	{
		// TODO: sse

		float sx = style.font.scale.cx;
		float sy = style.font.scale.cy;

		bool brotate = style.placement.angle.x != 0 || style.placement.angle.y != 0 || style.placement.angle.z != 0;
		bool bspline = !spline.empty();
		bool bscale = brotate || bspline || sx != 1 || sy != 1;

		float caz = cos(d2r(style.placement.angle.z));
		float saz = sin(d2r(style.placement.angle.z));
		float cax = cos(d2r(style.placement.angle.x));
		float sax = sin(d2r(style.placement.angle.x));
		float cay = cos(d2r(style.placement.angle.y));
		float say = sin(d2r(style.placement.angle.y));

		for(int i = 0, j = path.m_count; i < j; i++)
		{
			Vector2i p = path.m_points[i];

			if(bscale)
			{
				float x, y, z, xx, yy, zz;

				x = sx * p.x - org.x;
				y = sy * p.y - org.y;
				z = 0;

				if(bspline)
				{
					float pos = vertical ? y + org.y + tl.y - subrect.top : x + org.x + tl.x - subrect.left;
					float size = vertical ? (float)subrect.height() : (float)subrect.width();
					float dist = vertical ? x : y;

					const SplineCoeffs* sc;
					float t;

					if(pos >= size)
					{
						sc = &spline[spline.size() - 1];
						t = 1;
					}
					else
					{
						float u = size / spline.size();
						sc = &spline[std::max<int>((int)(pos / u), 0)];
						t = fmod(pos, u) / u;
					}

					float nx = sc->cx[1] + 2 * t * sc->cx[2] + 3 * t * t * sc->cx[3];
					float ny = sc->cy[1] + 2 * t * sc->cy[2] + 3 * t * t * sc->cy[3];
					float nl = 1.0f / sqrt(nx * nx + ny * ny);

					nx *= nl;
					ny *= nl;

					x = sc->cx[0] + t * (sc->cx[1] + t * (sc->cx[2] + t * sc->cx[3])) - ny * dist - org.x - tl.x;
					y = sc->cy[0] + t * (sc->cy[1] + t * (sc->cy[2] + t * sc->cy[3])) + nx * dist - org.y - tl.y;
				}

				if(brotate)
				{
					xx = x * caz + y * saz;
					yy = -(x * saz - y * caz);
					zz = z;

					x = xx;
					y = yy * cax + zz * sax;
					z = yy * sax - zz * cax;

					xx = x * cay + z * say;
					yy = y;
					zz = x * say - z * cay;

					zz = 1.0f / (max(zz, -19000) + 20000);

					x = (xx * 20000) * zz;
					y = (yy * 20000) * zz;
				}

				p.x = (int)(x + org.x + 0.5f);
				p.y = (int)(y + org.y + 0.5f);

				path.m_points[i] = p;
			}

			if(p.x < bbox.left) bbox.left = p.x;
			if(p.x > bbox.right) bbox.right = p.x;
			if(p.y < bbox.top) bbox.top = p.y;
			if(p.y > bbox.bottom) bbox.bottom = p.y;
		}
	}

	void Glyph::Transform(Vector2i org, const Vector4i& subrect)
	{
		if(!style.placement.org.auto_x) org.x = (int)(style.placement.org.x * scale.x);
		if(!style.placement.org.auto_y) org.y = (int)(style.placement.org.y * scale.y);

		org.x -= tl.x;
		org.y -= tl.y;

		bbox = Vector4i(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

		Transform(path_bkg, org, subrect);
		Transform(path, org, subrect);

		if(bbox.rempty())
		{
			bbox = Vector4i::zero();
		}
	}

	void Glyph::Rasterize()
	{
		if(!path_bkg.IsEmpty())
		{
			ras.bg.ScanConvert(path_bkg, bbox);

			ras.bg.Rasterize(tl.x, tl.y);
		}

		ras.fg.ScanConvert(path, bbox);

		bool outline = false;

		if(style.background.type == L"outline" && style.background.size > 0)
		{
			ras.fg.CreateWidenedRegion((int)(GetBackgroundSize() + 0.5));

			outline = true;
		}

		ras.fg.Rasterize(tl.x, tl.y);

		Rasterizer* r = !path_bkg.IsEmpty() ? &ras.bg : &ras.fg;

		int plane = !outline ? 0 : style.font.color.a < 255 ? 2 : 1;

		if(style.background.blur > 0)
		{
			r->Blur(style.background.blur, plane);
		}

		if(style.shadow.depth > 0)
		{
			ras.shadow.Reuse(*r);

			float depth = GetShadowDepth();

			tls.x = tl.x + (int)(depth * cos(d2r(style.shadow.angle)) + 0.5f);
			tls.y = tl.y + (int)(depth * -sin(d2r(style.shadow.angle)) + 0.5f);

			ras.shadow.Rasterize(tls.x, tls.y);

			if(style.shadow.blur > 0)
			{
				ras.shadow.Blur(style.shadow.blur, plane);
			}
		}
	}
}