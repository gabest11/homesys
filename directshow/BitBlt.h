#pragma once

namespace Image
{
	extern bool I420ToI420(int w, int h, BYTE* dsty, BYTE* dstu, BYTE* dstv, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch);
	extern bool I420ToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch, bool fInterlaced = false);
	extern bool I420ToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch /* TODO: , bool fInterlaced = false */);
	extern bool YUY2ToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* src, int srcpitch);
	extern bool YUY2ToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* src, int srcpitch);
	extern bool RGBToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* src, int srcpitch, int sbpp);
	extern bool RGBToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* src, int srcpitch);

	extern void AvgLines8(BYTE* dst, DWORD h, DWORD pitch);
	extern void AvgLines555(BYTE* dst, DWORD h, DWORD pitch);
	extern void AvgLines565(BYTE* dst, DWORD h, DWORD pitch);

	extern void DeinterlaceBlend(BYTE* dst, BYTE* src, DWORD w, DWORD h, DWORD dstpitch, DWORD srcpitch);
	extern void DeinterlaceBob(BYTE* dst, BYTE* src, DWORD w, DWORD h, DWORD dstpitch, DWORD srcpitch, bool topfield);
}