#pragma once

#include "../util/Vector.h"

namespace SSF
{
	struct Bitmap
	{
		int w, h, pitch;
		void* bits;

		struct Bitmap() {w = h = pitch = 0; bits = NULL;}

		// converts alpha pre-multiplied rgb back to full range

		void Normalize(const Vector4i& bbox);
		void Normalize(const Vector4i& bbox, BYTE* dst, int dstpitch);
	};
}