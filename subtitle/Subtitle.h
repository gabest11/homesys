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

namespace SSF
{
	struct Point {float x, y;};
	struct PointAuto : public Point {bool auto_x, auto_y;};
	struct Size {float cx, cy;};
	struct Rect {float l, t, r, b;};
	struct Align {float v, h;};
	struct Angle {float x, y, z;};
	struct Color {float r, g, b, a; operator DWORD(); void operator = (DWORD c);};
	struct Frame {std::wstring reference; Size resolution;};
	struct Direction {std::wstring primary, secondary;};
	struct Time {float start, stop;};
	struct Background {Color color; float size, blur; bool scaled; std::wstring type;};
	struct Shadow {Color color; float depth, angle, blur; bool scaled;};

	class Path : public std::vector<Point>
	{
	public: 
		Path() {} 
		Path(LPCWSTR str) {*this = str;} 
		Path& operator = (LPCWSTR str);
		std::wstring ToString();
	};

	struct Placement 
	{
		Rect clip, margin; 
		Align align; 
		PointAuto pos; 
		PointAuto org;
		Point offset;
		Angle angle; 
		Path path;
	};

	struct Font
	{
		std::wstring face;
		float size, weight;
		Color color;
		bool underline, strikethrough, italic;
		float spacing;
		Size scale;
		bool kerning;
	};

	struct Fill
	{
		int id;
		Color color;
		float width;

		struct Fill() : id(0) {}
	};

	struct Style
	{
		std::wstring linebreak;
		std::wstring content;
		Placement placement;
		Color color;
		Font font;
		Background background;
		Shadow shadow;
		Fill fill;

		bool IsSimilar(const Style& s);
	};

	struct Text
	{
		Style style;
		std::wstring str;
	};

	class Subtitle
	{
		File* m_file;
		struct {std::map<std::wstring, float> align[2], weight, transition;} m_n2n;
		int m_fill_id;

		void GetStyle(Definition* def, Style& style);
		float GetMixWeight(Definition* pDef, float at, std::map<std::wstring, float>& offset, int default_id);
		template<class T> bool MixValue(Definition& def, T& value, float t);
		template<> bool MixValue(Definition& def, float& value, float t);
		template<class T> bool MixValue(Definition& def, T& value, float t, std::map<std::wstring, T>* n2n);
		template<> bool MixValue(Definition& def, float& value, float t, std::map<std::wstring, float>* n2n);
		template<> bool MixValue(Definition& def, Path& value, float t);
		void MixStyle(Definition* def, Style& dst, float t);

		void Parse(InputStream& s, Style style, float at, std::map<std::wstring, float> offset, Reference* parent);

		void AddChar(Text& t, WCHAR c);
		void AddText(Text& t);

	public:
		std::wstring m_name;
		bool m_animated;
		bool m_clip;
		Frame m_frame;
		Direction m_direction;
		std::wstring m_wrap;
		float m_layer;
		float m_index;
		Time m_time;
		std::list<Text> m_text;

	public:
		Subtitle(File* file);
		virtual ~Subtitle();

		bool Parse(Definition* def, float start, float stop, float at);
	};
};