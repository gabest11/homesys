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

#include "SubtitleFile.h"
#include "GlyphPath.h"
#include "Bitmap.h"
#include "Array.h"
#include "../util/Vector.h"

namespace SSF
{
	class Rasterizer
	{
		struct Outline
		{
			Array<Vector4i> normal;
			Array<Vector4i> wide;
			Array<Vector4i> tmp;
		};

		struct Edge 
		{
			int next;
			int posandflag;
		};

		struct EdgeBuffer
		{
			Edge* buff; 
			unsigned int size, next; 
			unsigned int* scan;
		};

		struct Overlay
		{
			Vector2i size;
			Vector2i path;
			Vector2i offset;
			BYTE* buff[3];
		};

		Vector2i m_first;
		Vector2i m_last;
		bool m_first_set;
		Vector2i m_size;
		int m_border;
		Outline m_outline;
		EdgeBuffer m_edge;
		Overlay m_overlay;

		void TrashOverlay();
		void ReallocEdgeBuffer(int edges);
		void EvaluateLine(const Vector4i& v01);
		void EvaluateBezier(const Vector4i& v0, const Vector4i& v1);
		void EvaluateBSpline(const Vector2i* v);
		void OverlapRegion(Array<Vector4i>& dst, Array<Vector4i>& src, int dx, int dy);

	public:
		Rasterizer();
		virtual ~Rasterizer();

		bool ScanConvert(GlyphPath& path, const Vector4i& bbox);
		bool CreateWidenedRegion(int r);
		bool Rasterize(int xsub, int ysub);
		void Reuse(Rasterizer& r);
		void Blur(float n, int plane);
		Vector4i Draw(const Bitmap& bm, const Vector4i& clip, int xsub, int ysub, const DWORD* switchpts, int plane);
	};
}