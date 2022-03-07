#pragma once

#include "Seekbar2.dxui.h"
#include "../../DirectShow/HTTPReader.h"

namespace DXUI
{
	class SeekBar2 : public SeekBar2DXUI
	{
		void GetFillRect(Vector4i& value);

		bool OnActivated(Control* c, int dir);
		bool OnDeactivated(Control* c, int dir);
		bool OnThreadTimer(Control* c, UINT64 n);
		bool OnPaintBackground(Control* c, DeviceContext* dc);
		bool OnPaintForeground(Control* c, DeviceContext* dc);

		Thread m_thread;

		std::list<HttpFilePart> m_parts;
		__int64 m_length;

	public:
		SeekBar2();

		Property<Vector4i> FillRect;
	};
}
