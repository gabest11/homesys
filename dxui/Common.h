#pragma once

#include "Event.h"
#include "../util/Vector.h"

namespace DXUI
{
	class Color
	{
	public:
		static DWORD None;
		static DWORD Black;
		static DWORD White;
		static DWORD Gray;
		static DWORD Green;
		static DWORD Red;
		static DWORD Blue;
		static DWORD LightGreen;
		static DWORD LightRed;
		static DWORD LightBlue;
		static DWORD Text1;
		static DWORD Text2;
		static DWORD Text3;
		static DWORD Background;
		static DWORD DarkBackground;
		static DWORD SeparatorLine;
		static DWORD MenuBackground1;
		static DWORD MenuBackground2;
		static DWORD MenuBackground3;
	};

	class FontType
	{
	public:
		static std::wstring Normal;
		static std::wstring Bold;
	};

	class Align
	{
	public:
		enum
		{
			Left = 1, 
			Right = 2, 
			Center = 4,
			Top = 8, 
			Bottom = 16, 
			Middle = 32
		};
	};

	class Anchor
	{
	public:
		enum
		{
			Percent, 
			Center, 
			Middle,
			Left,
			Right,
			Top = Left, 
			Bottom = Right
		};

		int t, p, c, s;
	};

	class BackgroundStyleType
	{
	public:
		enum
		{
			Stretch, 
			Fit, 
			FitOutside, 
			Center,
			TopLeft,
		};
	};

	class TextStyles
	{
	public:
		enum
		{
			Bold = 1, 
			Italic = 2, 
			Underline = 4, 
			Outline = 8, 
			Shadow = 16, 
			Escaped = 32,
			Wrap = 64
		};
	};

	class KeyEventParam
	{
	public:
		struct {DWORD key, chr, app, remote;} cmd;
		DWORD count;
		DWORD flags;
		DWORD mods;

		enum {None, Shift = 1, Alt = 2, Ctrl = 4, AltGr = 8}; // mods

		KeyEventParam()
		{
			memset(this, 0, sizeof(*this));
		}
	};

	class RemoteKey
	{
	public:
		enum
		{
			MyTV = 0x46,
			MyMusic = 0x47, // TODO
			MyPictures = 0x49,
			MyVideos = 0x4a, // TODO
			MyRadio = 0x50, // TODO
			Details = 0x209,
			RecordedTV = 0x48,
			Guide = 0x8d,
			LiveTV = 0x25,
			DVDMenu = 0x24,
			Angle = 0x4b,
			Audio = 0x4c,
			Subtitle = 0x4d,
			Red = 0x5b,
			Green = 0x5c,
			Yellow = 0x5d,
			Blue = 0x5e,
			Teletext = 0x5a,
			MaxValue = Details,
			User = MaxValue + 1,
		};

		static DWORD GetKey(DWORD vid, DWORD pid, const BYTE* buff, DWORD size, DWORD code, DWORD mods);
	};

	class MouseEventParam
	{
	public:
		enum {None, Down, Up, Dbl, RDown, RUp, RDbl, MDown, MUp, MDbl, Move, Wheel} cmd;
		DWORD flags;
		Vector2i point;
		float delta;

		MouseEventParam()
		{
			memset(this, 0, sizeof(*this));
		}
	};

	class DeviceContext;
	class Texture;

	struct PaintItemBackgroundParam
	{
		DeviceContext* dc;
		Texture* t;
		Vector4i rect;
		Vector4i margin;
		float alpha;
		int index;
		bool selected;
		bool zoomed;
	};

	struct PaintItemParam
	{
		DeviceContext* dc;
		Vector4i rect;
		int index;
		bool selected;
		bool zoomed;
		std::wstring text;
	};

	struct ItemTextParam
	{
		int index;
		std::wstring text;

		struct ItemTextParam(int i) : index(i) {}
	};
}
