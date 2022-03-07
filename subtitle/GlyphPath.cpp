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
#include "GlyphPath.h"

namespace SSF
{
	GlyphPath::GlyphPath()
	{
		m_count = 0;
		m_reserved = 0;
		m_types = NULL;
		m_points = NULL;
	}

	GlyphPath::~GlyphPath()
	{
		RemoveAll();
	}

	GlyphPath::GlyphPath(const GlyphPath& path)
	{
		*this = path;
	}

	void GlyphPath::operator = (const GlyphPath& path)
	{
		RemoveAll();

		*this += path;
	}

	void GlyphPath::operator += (const GlyphPath& path)
	{
		int count = m_count + path.m_count;

		if(count > m_reserved)
		{
			m_reserved = count * 3 / 2;

			m_types = (BYTE*)_aligned_realloc(m_types, sizeof(m_types[0]) * m_reserved, 16);
			m_points = (Vector2i*)_aligned_realloc(m_points, sizeof(m_points[0]) * m_reserved, 16);
		}

		memcpy(&m_types[m_count], path.m_types, sizeof(m_types[0]) * path.m_count);
		memcpy(&m_points[m_count], path.m_points, sizeof(m_points[0]) * path.m_count);

		m_count = count;
	}

	bool GlyphPath::IsEmpty()
	{
		return m_count == 0;
	}

	void GlyphPath::RemoveAll()
	{
		m_count = 0;
		m_reserved = 0;

		if(m_types != NULL)
		{
			_aligned_free(m_types);

			m_types = NULL;
		}

		if(m_points != NULL)
		{
			_aligned_free(m_points);

			m_points = NULL;
		}
	}

	void GlyphPath::SetCount(int count)
	{
		m_count = count;
		m_reserved = count;
		m_types = (BYTE*)_aligned_realloc(m_types, sizeof(m_types[0]) * m_count, 16);
		m_points = (Vector2i*)_aligned_realloc(m_points, sizeof(m_points[0]) * m_count, 16);
	}

	void GlyphPath::MovePoints(const Vector2i& o)
	{
		Vector2i* p = m_points;

		Vector4i oo = Vector4i(o).xyxy();

		int i = 0;

		for(int j = m_count & ~1; i < j; i += 2)
		{
			*(Vector4i*)&p[i] += oo;
		}

		if(m_count & 1)
		{
			p[i].x += o.x;
			p[i].y += o.y;
		}
	}

	void GlyphPath::Enlarge(const GlyphPath& path, float size)
	{
		m_count = path.m_count;
		m_types = (BYTE*)_aligned_realloc(m_types, sizeof(m_types[0]) * m_count, 16);
		m_points = (Vector2i*)_aligned_realloc(m_points, sizeof(m_points[0]) * m_count, 16);

		memcpy(m_types, path.m_types, sizeof(m_types[0]) * m_count);

		const BYTE* t = &path.m_types[0];
		const Vector2i* p = &path.m_points[0];

		int start = 0;
		int end = 0;

		for(size_t i = 0, j = m_count; i <= j; i++)
		{
			if(i > 0 && (i == j || (t[i] & ~PT_CLOSEFIGURE) == PT_MOVETO))
			{
				end = i - 1;

				const bool cw = true; // TODO: determine orientation (ttf is always cw and we are sill before Transform)

				Vector2i prev = p[end];
				Vector2i cur = p[start];

				for(int k = start; k <= end; k++)
				{
					Vector2i next = k < end ? p[k + 1] : p[start];

					for(int l = k - 1; prev == cur; l--)
					{
						if(l < start) l = end;
						
						prev = p[l];
					}

					for(int l = k+1; next == cur; l++)
					{
						if(l > (int)end) l = start;

						next = p[l];
					}

					Vector2i in = Vector2i(cur.x - prev.x, cur.y - prev.y);
					Vector2i out = Vector2i(next.x - cur.x, next.y - cur.y);

					float angle_in = atan2((float)in.y, (float)in.x);
					float angle_out = atan2((float)out.y, (float)out.x);
					float angle_diff = angle_out - angle_in;

					if(angle_diff < 0) angle_diff += (float)(M_PI * 2);
					if(angle_diff > M_PI) angle_diff -= (float)(M_PI * 2);

					float scale = cos(angle_diff / 2);

					Vector2i p;

					if(angle_diff < 0)
					{
						if(angle_diff > (float)(-M_PI / 8)) 
						{
							if(scale < 1.0f) scale = 1.0f;
						}
						else 
						{
							if(scale < 0.5f) scale = 0.5f;
						}
					}
					else
					{
						if(angle_diff < M_PI / 8) 
						{
							if(scale < 0.75f) scale = 0.75f;
						}
						else 
						{
							if(scale < 0.25f) scale = 0.25f;
						}
					}

					if(scale < 0.1f) scale = 0.1f;

					float angle = angle_in + angle_diff / 2 - (float)(cw ? -M_PI_2 : M_PI_2);
					float radius = -size / scale; // FIXME

					p.x = (int)(radius * cos(angle) + 0.5f);
					p.y = (int)(radius * sin(angle) + 0.5f);

					m_points[k] = Vector2i(cur.x + p.x, cur.y + p.y);

					prev = cur;
					cur = next;
				}

				start = end + 1;
			}
		}
	}
}
