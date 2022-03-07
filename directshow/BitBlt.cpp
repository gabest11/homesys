#include "stdafx.h"
#include "BitBlt.h"
#include "../util/Vector.h"

#pragma warning(disable : 4799) // no emms... blahblahblah

namespace Image
{

// TODO: cleanup

bool I420ToI420(int w, int h, BYTE* dsty, BYTE* dstu, BYTE* dstv, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch)
{
	if(w & 1) 
	{
		return false;
	}

	if(w > 0 && w == srcpitch && w == dstpitch)
	{
		memcpy(dsty, srcy, h * srcpitch);
		memcpy(dstu, srcu, h / 2 * srcpitch / 2);
		memcpy(dstv, srcv, h / 2 * srcpitch / 2);
	}
	else
	{
		int pitch = std::min<int>(abs(srcpitch), abs(dstpitch));

		for(int y = 0; y < h; y++, srcy += srcpitch, dsty += dstpitch)
		{
			memcpy(dsty, srcy, pitch);
		}

		srcpitch >>= 1;
		dstpitch >>= 1;

		pitch = std::min<int>(abs(srcpitch), abs(dstpitch));

		for(int y = 0; y < h; y += 2, srcu += srcpitch, dstu += dstpitch)
		{
			memcpy(dstu, srcu, pitch);
		}

		for(int y = 0; y < h; y += 2, srcv += srcpitch, dstv += dstpitch)
		{
			memcpy(dstv, srcv, pitch);
		}
	}

	return true;
}

bool YUY2ToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* src, int srcpitch)
{
	if(w > 0 && w == srcpitch && w == dstpitch)
	{
		memcpy(dst, src, h * srcpitch);
	}
	else
	{
		int pitch = min(abs(srcpitch), abs(dstpitch));

		for(int y = 0; y < h; y++, src += srcpitch, dst += dstpitch)
		{
			memcpy(dst, src, pitch);
		}
	}

	return true;
}

extern "C" void asm_YUVtoRGB32_row(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB24_row(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB16_row(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB32_row_MMX(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB24_row_MMX(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB16_row_MMX(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB32_row_ISSE(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB24_row_ISSE(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);
extern "C" void asm_YUVtoRGB16_row_ISSE(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width);

bool I420ToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch)
{
	if(w <= 0 || h <= 0 || (w & 1) || (h & 1))
	{
		return false;
	}

	void (*asm_YUVtoRGB_row)(void* ARGB1, void* ARGB2, BYTE* Y1, BYTE* Y2, BYTE* U, BYTE* V, long width) = NULL;

	if(!(w & 7))
	{
		switch(dbpp)
		{
		case 16: asm_YUVtoRGB_row = asm_YUVtoRGB16_row; break; // TODO: fix _ISSE (555->565)
		case 24: asm_YUVtoRGB_row = asm_YUVtoRGB24_row_ISSE; break;
		case 32: asm_YUVtoRGB_row = asm_YUVtoRGB32_row_ISSE; break;
		}
	}
	else
	{
		switch(dbpp)
		{
		case 16: asm_YUVtoRGB_row = asm_YUVtoRGB16_row; break;
		case 24: asm_YUVtoRGB_row = asm_YUVtoRGB24_row; break;
		case 32: asm_YUVtoRGB_row = asm_YUVtoRGB32_row; break;
		}
	}

	if(!asm_YUVtoRGB_row) 
	{
		return false;
	}

	do
	{
		asm_YUVtoRGB_row(dst + dstpitch, dst, srcy + srcpitch, srcy, srcu, srcv, w / 2);

		dst += 2 * dstpitch;

		srcy += srcpitch * 2;
		srcu += srcpitch / 2;
		srcv += srcpitch / 2;
	}
	while(h -= 2);

	_mm_empty();
	_mm_sfence();

	return true;
}

extern "C" void mmx_YUY2toRGB24(const BYTE* src, BYTE* dst, const BYTE* src_end, int src_pitch, int row_size, bool rec709);
extern "C" void mmx_YUY2toRGB32(const BYTE* src, BYTE* dst, const BYTE* src_end, int src_pitch, int row_size, bool rec709);

bool YUY2ToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* src, int srcpitch)
{
	if(dbpp == 32) 
	{
		mmx_YUY2toRGB32(src, dst, src + h * srcpitch, srcpitch, w, false);
	}
	else if(dbpp == 24) 
	{
		mmx_YUY2toRGB24(src, dst, src + h * srcpitch, srcpitch, w, false);
	}
	// else if(dbpp == 16) mmx_YUY2toRGB16(src, dst, src + h * srcpitch, srcpitch, w, false); // TODO

	return true;
}

static void yuvtoyuy2row_c(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width)
{
	WORD* dstw = (WORD*)dst;
	for(; width > 1; width -= 2)
	{
		*dstw++ = (*srcu++<<8)|*srcy++;
		*dstw++ = (*srcv++<<8)|*srcy++;
	}
}

static void __declspec(naked) yuvtoyuy2row_MMX(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width)
{
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		edi, [esp+20] // dst
		mov		ebp, [esp+24] // srcy
		mov		ebx, [esp+28] // srcu
		mov		esi, [esp+32] // srcv
		mov		ecx, [esp+36] // width

		shr		ecx, 3

yuvtoyuy2row_loop:

		movd		mm0, [ebx]
		punpcklbw	mm0, [esi]

		movq		mm1, [ebp]
		movq		mm2, mm1
		punpcklbw	mm1, mm0
		punpckhbw	mm2, mm0

		movq		[edi], mm1
		movq		[edi+8], mm2

		add		ebp, 8
		add		ebx, 4
		add		esi, 4
        add		edi, 16

		dec		ecx
		jnz		yuvtoyuy2row_loop

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

static void yuvtoyuy2row_avg_c(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv)
{
	WORD* dstw = (WORD*)dst;
	for(; width > 1; width -= 2, srcu++, srcv++)
	{
		*dstw++ = (((srcu[0]+srcu[pitchuv])>>1)<<8)|*srcy++;
		*dstw++ = (((srcv[0]+srcv[pitchuv])>>1)<<8)|*srcy++;
	}
}

static void __declspec(naked) yuvtoyuy2row_avg_MMX(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv)
{
	static const __int64 mask = 0x7f7f7f7f7f7f7f7fi64;

	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		movq	mm7, mask

		mov		edi, [esp+20] // dst
		mov		ebp, [esp+24] // srcy
		mov		ebx, [esp+28] // srcu
		mov		esi, [esp+32] // srcv
		mov		ecx, [esp+36] // width
		mov		eax, [esp+40] // pitchuv

		shr		ecx, 3

yuvtoyuy2row_avg_loop:

		movd		mm0, [ebx]
		punpcklbw	mm0, [esi]
		movq		mm1, mm0

		movd		mm2, [ebx + eax]
		punpcklbw	mm2, [esi + eax]
		movq		mm3, mm2

		// (x+y)>>1 == (x&y)+((x^y)>>1)

		pand		mm0, mm2
		pxor		mm1, mm3
		psrlq		mm1, 1
		pand		mm1, mm7
		paddb		mm0, mm1

		movq		mm1, [ebp]
		movq		mm2, mm1
		punpcklbw	mm1, mm0
		punpckhbw	mm2, mm0

		movq		[edi], mm1
		movq		[edi+8], mm2

		add		ebp, 8
		add		ebx, 4
		add		esi, 4
        add		edi, 16

		dec		ecx
		jnz		yuvtoyuy2row_avg_loop

		pop		ebx
		pop		esi
		pop		edi
		pop		ebp
		ret
	};
}

static void __declspec(naked) yv12_yuy2_row_sse2() {
  __asm {
    // ebx - Y
    // edx - U
    // esi - V
    // edi - dest
    // ecx - halfwidth
    xor     eax, eax

one:
    movdqa  xmm0, [ebx + eax*2]    // YYYYYYYY
    movdqa  xmm1, [ebx + eax*2 + 16]    // YYYYYYYY

    movdqa  xmm2, [edx + eax]      // UUUUUUUU
    movdqa  xmm3, [esi + eax]      // VVVVVVVV

    movdqa  xmm4, xmm2
    movdqa  xmm5, xmm0
    movdqa  xmm6, xmm1
    punpcklbw xmm2, xmm3          // VUVUVUVU
    punpckhbw xmm4, xmm3          // VUVUVUVU

    punpcklbw xmm0, xmm2          // VYUYVYUY
    punpcklbw xmm1, xmm4
    punpckhbw xmm5, xmm2
    punpckhbw xmm6, xmm4

    movntdq [edi + eax*4], xmm0
    movntdq [edi + eax*4 + 16], xmm5
    movntdq [edi + eax*4 + 32], xmm1
    movntdq [edi + eax*4 + 48], xmm6

    add     eax, 16
    cmp     eax, ecx

    jb      one

    ret
  };
}

static void __declspec(naked) yv12_yuy2_row_sse2_linear() {
  __asm {
    // ebx - Y
    // edx - U
    // esi - V
    // edi - dest
    // ecx - width
    // ebp - uv_stride
    xor     eax, eax

one:
    movdqa  xmm0, [ebx + eax*2]    // YYYYYYYY
    movdqa  xmm1, [ebx + eax*2 + 16]    // YYYYYYYY

    movdqa  xmm2, [edx]
    movdqa  xmm3, [esi]
    pavgb   xmm2, [edx + ebp]      // UUUUUUUU
    pavgb   xmm3, [esi + ebp]      // VVVVVVVV

    movdqa  xmm4, xmm2
    movdqa  xmm5, xmm0
    movdqa  xmm6, xmm1
    punpcklbw xmm2, xmm3          // VUVUVUVU
    punpckhbw xmm4, xmm3          // VUVUVUVU

    punpcklbw xmm0, xmm2          // VYUYVYUY
    punpcklbw xmm1, xmm4
    punpckhbw xmm5, xmm2
    punpckhbw xmm6, xmm4

    movntdq [edi + eax*4], xmm0
    movntdq [edi + eax*4 + 16], xmm5
    movntdq [edi + eax*4 + 32], xmm1
    movntdq [edi + eax*4 + 48], xmm6

    add     eax, 16
    add     edx, 16
    add     esi, 16
    cmp     eax, ecx

    jb      one

    ret
  };
}

static void __declspec(naked) yv12_yuy2_row_sse2_linear_interlaced() {
  __asm {
    // ebx - Y
    // edx - U
    // esi - V
    // edi - dest
    // ecx - width
    // ebp - uv_stride
    xor     eax, eax

one:
    movdqa  xmm0, [ebx + eax*2]    // YYYYYYYY
    movdqa  xmm1, [ebx + eax*2 + 16]    // YYYYYYYY

    movdqa  xmm2, [edx]
    movdqa  xmm3, [esi]
    pavgb   xmm2, [edx + ebp*2]      // UUUUUUUU
    pavgb   xmm3, [esi + ebp*2]      // VVVVVVVV

    movdqa  xmm4, xmm2
    movdqa  xmm5, xmm0
    movdqa  xmm6, xmm1
    punpcklbw xmm2, xmm3          // VUVUVUVU
    punpckhbw xmm4, xmm3          // VUVUVUVU

    punpcklbw xmm0, xmm2          // VYUYVYUY
    punpcklbw xmm1, xmm4
    punpckhbw xmm5, xmm2
    punpckhbw xmm6, xmm4

    movntdq [edi + eax*4], xmm0
    movntdq [edi + eax*4 + 16], xmm5
    movntdq [edi + eax*4 + 32], xmm1
    movntdq [edi + eax*4 + 48], xmm6

    add     eax, 16
    add     edx, 16
    add     esi, 16
    cmp     eax, ecx

    jb      one

    ret
  };
}

void __declspec(naked) yv12_yuy2_sse2(const BYTE *Y, const BYTE *U, const BYTE *V,
    int halfstride, unsigned halfwidth, unsigned height,
    BYTE *YUY2, int d_stride)
{
  __asm {
    push    ebx
    push    esi
    push    edi
    push    ebp

    mov     ebx, [esp + 20] // Y
    mov     edx, [esp + 24] // U
    mov     esi, [esp + 28] // V
    mov     edi, [esp + 44] // D
    mov     ebp, [esp + 32] // uv_stride
    mov     ecx, [esp + 36] // uv_width

    mov     eax, ecx
    add     eax, 15
    and     eax, 0xfffffff0
    sub     [esp + 32], eax

    cmp     dword ptr [esp + 40], 2
    jbe     last2

row:
    sub     dword ptr [esp + 40], 2
    call    yv12_yuy2_row_sse2

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]

    call    yv12_yuy2_row_sse2_linear

    add     edx, [esp + 32]
    add     esi, [esp + 32]

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]

    cmp     dword ptr [esp + 40], 2
    ja      row

last2:
    call    yv12_yuy2_row_sse2

    dec     dword ptr [esp + 40]
    jz      done

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]
    call    yv12_yuy2_row_sse2
done:

    pop     ebp
    pop     edi
    pop     esi
    pop     ebx

    ret
  };
}

void __declspec(naked) yv12_yuy2_sse2_interlaced(const BYTE *Y, const BYTE *U, const BYTE *V,
    int halfstride, unsigned halfwidth, unsigned height,
    BYTE *YUY2, int d_stride)
{
  __asm {
    push    ebx
    push    esi
    push    edi
    push    ebp

    mov     ebx, [esp + 20] // Y
    mov     edx, [esp + 24] // U
    mov     esi, [esp + 28] // V
    mov     edi, [esp + 44] // D
    mov     ebp, [esp + 32] // uv_stride
    mov     ecx, [esp + 36] // uv_width

    mov     eax, ecx
    add     eax, 15
    and     eax, 0xfffffff0
    sub     [esp + 32], eax

    cmp     dword ptr [esp + 40], 4
    jbe     last4

row:
    sub     dword ptr [esp + 40], 4
    call    yv12_yuy2_row_sse2	// first row, first field

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]

    add	    edx, ebp
    add	    esi, ebp

    call    yv12_yuy2_row_sse2	// first row, second field

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]

    sub	    edx, ebp
    sub	    esi, ebp

    call    yv12_yuy2_row_sse2_linear_interlaced // second row, first field

    add     edx, [esp + 32]
    add     esi, [esp + 32]

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]

    call    yv12_yuy2_row_sse2_linear_interlaced // second row, second field

    add     edx, [esp + 32]
    add     esi, [esp + 32]

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]

    cmp     dword ptr [esp + 40], 4
    ja      row

last4:
    call    yv12_yuy2_row_sse2

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]

    add     edx, ebp
    add     esi, ebp

    call    yv12_yuy2_row_sse2

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]

    sub     edx, ebp
    sub     esi, ebp

    call    yv12_yuy2_row_sse2

    lea     ebx, [ebx + ebp*2]
    add     edi, [esp + 48]

    add     edx, ebp
    add     esi, ebp

    call    yv12_yuy2_row_sse2

    pop     ebp
    pop     edi
    pop     esi
    pop     ebx

    ret
  };
}

bool I420ToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch, bool fInterlaced)
{
	if(w <= 0 || h <= 0 || (w & 1) || (h & 1))
	{
		return false;
	}

	if(srcpitch == 0) srcpitch = w;

	if(((DWORD_PTR)srcy&15) && ((DWORD_PTR)srcu&15) && ((DWORD_PTR)srcv&15) && !(srcpitch&15) 
	&& ((DWORD_PTR)dst&15) && !(dstpitch&15))
	{
		if(!fInterlaced) yv12_yuy2_sse2(srcy, srcu, srcv, srcpitch/2, w/2, h, dst, dstpitch);
		else yv12_yuy2_sse2_interlaced(srcy, srcu, srcv, srcpitch/2, w/2, h, dst, dstpitch);
		return true;
	}
	else
	{
		ASSERT(!fInterlaced);
	}

	void (*yuvtoyuy2row)(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width) = NULL;
	void (*yuvtoyuy2row_avg)(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv) = NULL;

	if(!(w & 7))
	{
		yuvtoyuy2row = yuvtoyuy2row_MMX;
		yuvtoyuy2row_avg = yuvtoyuy2row_avg_MMX;
	}
	else
	{
		yuvtoyuy2row = yuvtoyuy2row_c;
		yuvtoyuy2row_avg = yuvtoyuy2row_avg_c;
	}

	do
	{
		yuvtoyuy2row(dst, srcy, srcu, srcv, w);
		yuvtoyuy2row_avg(dst + dstpitch, srcy + srcpitch, srcu, srcv, w, srcpitch/2);

		dst += 2 * dstpitch;
		srcy += srcpitch * 2;
		srcu += srcpitch / 2;
		srcv += srcpitch / 2;
	}
	while((h -= 2) > 2);

	yuvtoyuy2row(dst, srcy, srcu, srcv, w);
	yuvtoyuy2row(dst + dstpitch, srcy + srcpitch, srcu, srcv, w);

	_mm_empty();

	return true;
}

bool RGBToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* src, int srcpitch, int sbpp)
{
	if(dbpp == sbpp)
	{
		int rowbytes = w * dbpp >> 3;

		if(rowbytes > 0 && rowbytes == srcpitch && rowbytes == dstpitch)
		{
			memcpy(dst, src, h * rowbytes);
		}
		else
		{
			for(int y = 0; y < h; y++, src += srcpitch, dst += dstpitch)
			{
				memcpy(dst, src, rowbytes);
			}
		}

		return true;
	}
	
	if(sbpp != 16 && sbpp != 24 && sbpp != 32 || dbpp != 16 && dbpp != 24 && dbpp != 32)
	{
		return false;
	}

	if(dbpp == 16)
	{
		for(int y = 0; y < h; y++, src += srcpitch, dst += dstpitch)
		{
			if(sbpp == 24)
			{
				BYTE* s = (BYTE*)src;
				WORD* d = (WORD*)dst;

				for(int x = 0; x < w; x++, s+=3, d++)
				{
					*d = (WORD)(((*((DWORD*)s)>>8)&0xf800)|((*((DWORD*)s)>>5)&0x07e0)|((*((DWORD*)s)>>3)&0x1f));
				}
			}
			else if(sbpp == 32)
			{
				DWORD* s = (DWORD*)src;
				WORD* d = (WORD*)dst;

				for(int x = 0; x < w; x++, s++, d++)
				{
					*d = (WORD)(((*s>>8)&0xf800)|((*s>>5)&0x07e0)|((*s>>3)&0x1f));
				}
			}
		}
	}
	else if(dbpp == 24)
	{
		for(int y = 0; y < h; y++, src += srcpitch, dst += dstpitch)
		{
			if(sbpp == 16)
			{
				WORD* s = (WORD*)src;
				BYTE* d = (BYTE*)dst;

				for(int x = 0; x < w; x++, s++, d += 3)
				{
					// not tested, r-g-b might be in reverse

					d[0] = (*s&0x001f) << 3;
					d[1] = (*s&0x07e0) << 5;
					d[2] = (*s&0xf800) << 8;
				}
			}
			else if(sbpp == 32)
			{
				BYTE* s = (BYTE*)src;
				BYTE* d = (BYTE*)dst;

				for(int x = 0; x < w; x++, s += 4, d += 3)
				{
					d[0] = s[0]; 
					d[1] = s[1]; 
					d[2] = s[2];
				}
			}
		}
	}
	else if(dbpp == 32)
	{
		for(int y = 0; y < h; y++, src += srcpitch, dst += dstpitch)
		{
			if(sbpp == 16)
			{
				WORD* s = (WORD*)src;
				DWORD* d = (DWORD*)dst;

				for(int x = 0; x < w; x++, s++, d++)
				{
					*d = ((*s&0xf800)<<8)|((*s&0x07e0)<<5)|((*s&0x001f)<<3);
				}
			}
			else if(sbpp == 24)
			{	
				BYTE* s = (BYTE*)src;
				DWORD* d = (DWORD*)dst;

				for(int x = 0; x < w; x++, s+=3, d++)
				{
					*d = *((DWORD*)s) & 0xffffff;
				}
			}
		}
	}

	return true;
}

bool RGBToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* src, int srcpitch)
{
	if(w & 1) return false;

	static const Vector4 ys(0.098f, 0.504f, 0.257f, 0.0f);
	static const Vector4 us(0.439f / 2, -0.291f / 2, -0.148f / 2, 0.0f);
	static const Vector4 vs(-0.071f / 2, -0.368f / 2, 0.439f / 2, 0.0f);
	static const Vector4 offset(16, 128, 16, 128);

	for(int j = 0; j < h; j++, dst += dstpitch, src += srcpitch)
	{
		DWORD* s = (DWORD*)src;
		WORD* d = (WORD*)dst;

		for(int i = 0; i < w; i += 2)
		{
			Vector4 c0 = Vector4(s[i + 0]);
			Vector4 c1 = Vector4(s[i + 1]);
			Vector4 c2 = c0 + c1;

			Vector4 lo = (c0 * ys).hadd(c2 * vs);
			Vector4 hi = (c1 * ys).hadd(c2 * us);

			Vector4 c = lo.hadd(hi) + offset;

			*((DWORD*)&d[i]) = Vector4i(c).rgba32();
		}
	}

	return true;
}

static void DeinterlaceBlend2(BYTE* RESTRICT dst, BYTE* RESTRICT src, DWORD w, DWORD srcpitch)
{
	DWORD w2 = 0;

	if(((DWORD_PTR)src & 15) == 0 && ((DWORD_PTR)dst & 15) == 0 && (srcpitch & 15) == 0)
	{
		w2 = w & ~15;

		for(DWORD i = 0; i < w2; i += 16)
		{
			Vector4i a = Vector4i::load<true>(&src[i + srcpitch * 0]);
			Vector4i b = Vector4i::load<true>(&src[i + srcpitch * 1]);

			Vector4i::store<true>(&dst[i], a.avg8(b));
		}
	}
	else
	{
		w2 = w & ~7;

		for(DWORD i = 0; i < w2; i += 8)
		{
			Vector4i a = Vector4i::loadl(&src[i + srcpitch * 0]);
			Vector4i b = Vector4i::loadl(&src[i + srcpitch * 1]);

			Vector4i::storel(&dst[i], a.avg8(b));
		}
	}

	for(DWORD i = w2; i < w; i++)
	{
		DWORD a = src[i + srcpitch * 0];
		DWORD b = src[i + srcpitch * 1];

		dst[i] = (BYTE)((a + b + 1) >> 1);
	}
}

static void DeinterlaceBlend3(BYTE* dst, BYTE* src, DWORD w, DWORD h, DWORD srcpitch, DWORD dstpitch)
{
	BYTE* RESTRICT s = src;
	BYTE* RESTRICT d = dst;

	DWORD w2 = 0;

	if(((DWORD_PTR)src & 15) == 0 && ((DWORD_PTR)dst & 15) == 0 && (srcpitch & 15) == 0 && (dstpitch & 15) == 0)
	{
		w2 = w & ~15;

		for(DWORD j = 0; j < h; j++)
		{
			for(DWORD i = 0; i < w2; i += 16)
			{
				Vector4i a = Vector4i::load<true>(&s[i + srcpitch * 0]);
				Vector4i b = Vector4i::load<true>(&s[i + srcpitch * 1]);
				Vector4i c = Vector4i::load<true>(&s[i + srcpitch * 2]);

				Vector4i::store<true>(&d[i], a.avg8(b).avg8(b.avg8(c)));
			}

			s += srcpitch;
			d += dstpitch;
		}
	}
	else
	{
		w2 = w & ~7;

		for(DWORD j = 0; j < h; j++)
		{
			for(DWORD i = 0; i < w2; i += 8)
			{
				Vector4i a = Vector4i::loadl(&s[i + srcpitch * 0]);
				Vector4i b = Vector4i::loadl(&s[i + srcpitch * 1]);
				Vector4i c = Vector4i::loadl(&s[i + srcpitch * 2]);

				Vector4i::storel(&d[i], a.avg8(b).avg8(b.avg8(c)));
			}

			s += srcpitch;
			d += dstpitch;
		}
	}

	if(w > w2)
	{
		s = src;
		d = dst;

		for(DWORD j = 0; j < h; j++)
		{
			for(DWORD i = w2; i < w; i++)
			{
				DWORD a = s[i + srcpitch * 0];
				DWORD b = s[i + srcpitch * 1];
				DWORD c = s[i + srcpitch * 2];

				d[i] = (BYTE)((a + (b << 1) + c + 2) >> 2);
			}

			s += srcpitch;
			d += dstpitch;
		}
	}
}

void DeinterlaceBlend(BYTE* dst, BYTE* src, DWORD w, DWORD h, DWORD dstpitch, DWORD srcpitch)
{
	DeinterlaceBlend2(dst, src, w, srcpitch);
	DeinterlaceBlend3(dst + dstpitch, src, w, h - 2, srcpitch, dstpitch);
	DeinterlaceBlend2(dst + dstpitch * (h - 1), src + srcpitch * (h - 2), w, srcpitch);
}

static void AvgLines8(BYTE* dst, DWORD h, DWORD pitch)
{
	if(h <= 1) return;

	BYTE* s = dst;
	BYTE* d = dst + (h-2)*pitch;

	for(; s < d; s += pitch*2)
	{
		BYTE* tmp = s;

		if(!((DWORD)tmp & 0xf) && !((DWORD)pitch & 0xf))
		{
			__asm
			{
				mov		esi, tmp
				mov		ebx, pitch

				mov		ecx, ebx
				shr		ecx, 4

AvgLines8_sse2_loop:
				movdqa	xmm0, [esi]
				pavgb	xmm0, [esi+ebx * 2]
				movdqa	[esi+ebx], xmm0
				add		esi, 16

				dec		ecx
				jnz		AvgLines8_sse2_loop

				mov		tmp, esi
			}

			for(int i = pitch & 7; i--; tmp++)
			{
				tmp[pitch] = (tmp[0] + tmp[pitch << 1] + 1) >> 1;
			}
		}
		else
		{
			__asm
			{
				mov		esi, tmp
				mov		ebx, pitch

				mov		ecx, ebx
				shr		ecx, 3

				pxor	mm7, mm7
AvgLines8_mmx_loop:
				movq	mm0, [esi]
				movq	mm1, mm0

				punpcklbw	mm0, mm7
				punpckhbw	mm1, mm7

				movq	mm2, [esi+ebx*2]
				movq	mm3, mm2

				punpcklbw	mm2, mm7
				punpckhbw	mm3, mm7

				paddw	mm0, mm2
				psrlw	mm0, 1

				paddw	mm1, mm3
				psrlw	mm1, 1

				packuswb	mm0, mm1

				movq	[esi+ebx], mm0

				lea		esi, [esi+8]

				dec		ecx
				jnz		AvgLines8_mmx_loop

				mov		tmp, esi
			}

			for(int i = pitch&7; i--; tmp++)
			{
				tmp[pitch] = (tmp[0] + tmp[pitch << 1] + 1) >> 1;
			}
		}
/*
		else
		{
			for(int i = pitch; i--; tmp++)
			{
				tmp[pitch] = (tmp[0] + tmp[pitch << 1] + 1) >> 1;
			}
		}
*/
	}

	if(!(h & 1) && h >= 2)
	{
		dst += (h - 2) * pitch;

		memcpy(dst + pitch, dst, pitch);
	}

	_mm_empty();
}

void DeinterlaceBob(BYTE* dst, BYTE* src, DWORD w, DWORD h, DWORD dstpitch, DWORD srcpitch, bool topfield)
{
	// TODO: one pass

	if(topfield)
	{
		RGBToRGB(w, h / 2, dst, dstpitch * 2, 8, src, srcpitch * 2, 8);
		AvgLines8(dst, h, dstpitch);
	}
	else
	{
		RGBToRGB(w, h / 2, dst + dstpitch, dstpitch * 2, 8, src + srcpitch, srcpitch * 2, 8);
		AvgLines8(dst + dstpitch, h - 1, dstpitch);
	}
}

}