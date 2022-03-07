#include "stdafx.h"
#include "Bitmap.h"

void SSF::Bitmap::Normalize(const Vector4i& bbox)
{
	if(bbox.rempty()) return;

	ASSERT((pitch & 15) == 0);

	Vector4i mask = Vector4i::x000000ff();
	
	int x = bbox.left & ~3;
	int row = (bbox.right - x) * 4;

	BYTE* src = (BYTE*)bits + bbox.top * pitch + x * 4;

	for(int y = bbox.top; y < bbox.bottom; y++, src += pitch)
	{
		BYTE* s = src;
		BYTE* e = s + row;

		for( ; s < e; s += 16)
		{
			Vector4i v = Vector4i::load<true>(s);

			Vector4i ri = (v >> 16) & mask;
			Vector4i gi = (v >> 8) & mask;
			Vector4i bi = (v) & mask;
			Vector4i ai = (v >> 24) & mask;

			Vector4 r = Vector4(ri);
			Vector4 b = Vector4(bi);
			Vector4 g = Vector4(gi);
			Vector4 a = Vector4(ai).rcp() * 256;

			ri = Vector4i(r * a);
			gi = Vector4i(g * a);
			bi = Vector4i(b * a);

			Vector4i rg = ri.ps32(gi);
			Vector4i ba = bi.ps32(ai);

			Vector4i rb = rg.upl16(ba);
			Vector4i ga = rg.uph16(ba);

			Vector4i c = rb.upl16(ga).pu16(rb.uph16(ga));

			Vector4i::store<true>(s, c);
		}
	}
}

void SSF::Bitmap::Normalize(const Vector4i& bbox, BYTE* dst, int dstpitch)
{
	if(bbox.rempty()) return;

	ASSERT((pitch & 15) == 0);
	ASSERT((dstpitch & 15) == 0);

	Vector4i mask = Vector4i::x000000ff();
	
	int x = bbox.left & ~3;
	int row = (bbox.right - x) * 4;

	BYTE* src = (BYTE*)bits + bbox.top * pitch + x * 4;

	for(int y = bbox.top; y < bbox.bottom; y++, src += pitch, dst += dstpitch)
	{
		BYTE* __restrict s = src;
		BYTE* __restrict d = dst;
		BYTE* e = s + row;

		for( ; s < e; s += 16, d += 16)
		{
			Vector4i v = Vector4i::load<true>(s);

			Vector4i ri = (v >> 16) & mask;
			Vector4i gi = (v >> 8) & mask;
			Vector4i bi = (v) & mask;
			Vector4i ai = (v >> 24) & mask;

			Vector4 r = Vector4(ri);
			Vector4 b = Vector4(bi);
			Vector4 g = Vector4(gi);
			Vector4 a = Vector4(ai).rcp() * 256;

			ri = Vector4i(r * a);
			gi = Vector4i(g * a);
			bi = Vector4i(b * a);

			Vector4i rg = ri.ps32(gi);
			Vector4i ba = bi.ps32(ai);

			Vector4i rb = rg.upl16(ba);
			Vector4i ga = rg.uph16(ba);

			Vector4i c = rb.upl16(ga).pu16(rb.uph16(ga));

			Vector4i::store<true>(d, c);
		}
	}
}