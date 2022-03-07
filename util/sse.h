#pragma once

#ifndef ASSERT

	#if defined(_DEBUG) && defined(_MSC_VER)

		#include <assert.h>
		#define ASSERT assert

	#else

		#define ASSERT(exp) ((void)0)

	#endif

#endif

#if _M_SSE >= 0x200

	#include <xmmintrin.h>
	#include <emmintrin.h>

	#ifndef _MM_DENORMALS_ARE_ZERO
	#define _MM_DENORMALS_ARE_ZERO 0x0040
	#endif

	#define MXCSR_FAST (_MM_DENORMALS_ARE_ZERO | _MM_MASK_MASK | _MM_ROUND_NEAREST | _MM_FLUSH_ZERO_ON)

	#if _MSC_VER < 1500

	__forceinline __m128i _mm_castps_si128(__m128 a) {return *(__m128i*)&a;}
	__forceinline __m128 _mm_castsi128_ps(__m128i a) {return *(__m128*)&a;}
	__forceinline __m128i _mm_castpd_si128(__m128d a) {return *(__m128i*)&a;}
	__forceinline __m128d _mm_castsi128_pd(__m128i a) {return *(__m128d*)&a;}
	__forceinline __m128d _mm_castps_pd(__m128 a) {return *(__m128d*)&a;}
	__forceinline __m128 _mm_castpd_ps(__m128d a) {return *(__m128*)&a;}

	#endif

	#define _MM_TRANSPOSE4_SI128(row0, row1, row2, row3) \
	{ \
		__m128 tmp0 = _mm_shuffle_ps(_mm_castsi128_ps(row0), _mm_castsi128_ps(row1), 0x44); \
		__m128 tmp2 = _mm_shuffle_ps(_mm_castsi128_ps(row0), _mm_castsi128_ps(row1), 0xEE); \
		__m128 tmp1 = _mm_shuffle_ps(_mm_castsi128_ps(row2), _mm_castsi128_ps(row3), 0x44); \
		__m128 tmp3 = _mm_shuffle_ps(_mm_castsi128_ps(row2), _mm_castsi128_ps(row3), 0xEE); \
		(row0) = _mm_castps_si128(_mm_shuffle_ps(tmp0, tmp1, 0x88)); \
		(row1) = _mm_castps_si128(_mm_shuffle_ps(tmp0, tmp1, 0xDD)); \
		(row2) = _mm_castps_si128(_mm_shuffle_ps(tmp2, tmp3, 0x88)); \
		(row3) = _mm_castps_si128(_mm_shuffle_ps(tmp2, tmp3, 0xDD)); \
	}

#else

#error TODO: Vector4 needs SSE2

#endif

#if _M_SSE >= 0x301

	#include <tmmintrin.h>

#endif

#if _M_SSE >= 0x401

	#include <smmintrin.h>

#endif