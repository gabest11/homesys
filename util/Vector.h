#pragma once

#include "sse.h"
#include "AlignedClass.h"

// NOTE: x64 version of the _mm_set_* functions are terrible, first they store components into memory then reload in one piece (VS2008 SP1)

#pragma pack(push, 1)

template<class T> class Vector2T
{
public:
	union 
	{
		struct {T x, y;}; 
		struct {T r, g;}; 
		struct {T v[2];};
	};

	Vector2T()
	{
	}

	explicit Vector2T(T t)
	{
		x = t;
		y = t;
	}
	
	Vector2T(T x, T y) 
	{
		this->x = x; 
		this->y = y;
	}

	bool operator == (const Vector2T& v) const
	{
		return x == v.x && y == v.y;
	}

	bool operator != (const Vector2T& v) const
	{
		return x != v.x || y != v.y;
	}
};

typedef Vector2T<float> Vector2;
typedef Vector2T<int> Vector2i;

class Vector4;

__declspec(align(16)) class Vector4i : public AlignedClass<16>
{
public:
	union 
	{
		struct {int x, y, z, w;}; 
		struct {int r, g, b, a;};
		struct {int left, top, right, bottom;}; 
		struct {Vector2i tl, br;};
		int v[4];
		float f32[4];
		char i8[16];
		short i16[8];
		int i32[4];
		__int64 i64[2];
		unsigned char u8[16];
		unsigned short u16[8];
		unsigned int u32[4];
		unsigned __int64 u64[2];
		__m128i m;
	};

	Vector4i() 
	{
	}

	Vector4i(int x, int y, int z, int w) 
	{
		// 4 gprs

		// m = _mm_set_epi32(w, z, y, x); 

		// 2 gprs

		Vector4i xz = load(x).upl32(load(z));
		Vector4i yw = load(y).upl32(load(w));

		*this = xz.upl32(yw);
	}

	Vector4i(int x, int y) 
	{
		*this = load(x).upl32(load(y));
	}

	Vector4i(short s0, short s1, short s2, short s3, short s4, short s5, short s6, short s7) 
	{
		m = _mm_set_epi16(s7, s6, s5, s4, s3, s2, s1, s0);
	}

	Vector4i(char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, char b8, char b9, char b10, char b11, char b12, char b13, char b14, char b15) 
	{
		m = _mm_set_epi8(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0);
	}

	Vector4i(const Vector4i& v) 
	{
		m = v.m;
	}

	explicit Vector4i(const Vector2i& v)
	{
		m = _mm_loadl_epi64((__m128i*)&v);
	}

	Vector4i(const Vector2i& v0, const Vector2i& v1)
	{
		*this = Vector4i(v0).upl64(Vector4i(v1));
	}

	explicit Vector4i(int i) 
	{
		m = _mm_set1_epi32(i);
	}

	explicit Vector4i(__m128i m)
	{
		this->m = m;
	}

	explicit Vector4i(const Vector4& v);

	Vector4i& operator = (const Vector4i& v) 
	{
		m = v.m;

		return *this;
	}

	Vector4i& operator = (int i) 
	{
		m = _mm_set1_epi32(i);

		return *this;
	}

	Vector4i& operator = (__m128i m) 
	{
		this->m = m;

		return *this;
	}

	operator __m128i() const 
	{
		return m;
	}

	// rect

	int width() const
	{
		return right - left;
	}

	int height() const
	{
		return bottom - top;
	}

	Vector4i rsize() const
	{
		return *this - xyxy(); // same as Vector4i(0, 0, width(), height());
	}

	bool rempty() const
	{
		return (*this < zwzw()).mask() != 0x00ff;
	}

	Vector4i runion(const Vector4i& a) const 
	{
		int i = (upl64(a) < uph64(a)).mask();

		if(i == 0xffff)
		{
			#if _M_SSE >= 0x401

			return min_i32(a).upl64(max_i32(a).srl<8>());

			#else

			return Vector4i(min(x, a.x), min(y, a.y), max(z, a.z), max(w, a.w));

			#endif
		}

		if((i & 0x00ff) == 0x00ff)
		{
			return *this;
		}

		if((i & 0xff00) == 0xff00)
		{
			return a;
		}

		return Vector4i::zero();
	}

	Vector4i rintersect(const Vector4i& a) const 
	{
		return sat_i32(a);
	}

	Vector4i inflate(const Vector4i& a) const 
	{
		return (*this - a).upl64((*this + a).zwxy());
	}

	Vector4i deflate(const Vector4i& a) const 
	{
		return (*this + a).upl64((*this - a).zwxy());
	}

	Vector4i inflate(int i) const 
	{
		return inflate(Vector4i(i));
	}

	Vector4i deflate(int i) const 
	{
		return deflate(Vector4i(i));
	}

	bool contains(const Vector2i& a) const
	{
		return x <= a.x && a.x < z && y <= a.y && a.y < w;
	}

	enum RoundMode {Outside, Inside, NegInf, PosInf};

	template<int mode> Vector4i ralign(const Vector2i& a) const 
	{
		// a must be 1 << n

		Vector4i mask = Vector4i(a) - Vector4i(1, 1);

		Vector4i v;

		switch(mode)
		{
		case Inside: v = *this + mask; break; 
		case Outside: v = *this + mask.zwxy(); break;
		case NegInf: v = *this; break; 
		case PosInf: v = *this + mask.zwzw(); break;
		default: ASSERT(0); break;
		}

		return v.andnot(mask.xyxy());
	}

	Vector4i fit(const Vector2i& ar) const;

	Vector4i fit(int arx, int ary) const;

	Vector4i fit(int preset) const;

	#ifdef _WINDOWS

	operator LPCRECT() const
	{
		return (LPCRECT)this;
	}

	operator LPRECT()
	{
		return (LPRECT)this;
	}

	#endif

	//

	unsigned int rgba32() const
	{
		Vector4i v = *this;

		v = v.ps32(v);
		v = v.pu16(v);

		return (unsigned int)store(v);
	}

	static Vector4i cast(const Vector4& v);

	#if _M_SSE >= 0x301

	Vector4i abs8() const 
	{
		return Vector4i(_mm_abs_epi8(m));
	}

	Vector4i abs16() const 
	{
		return Vector4i(_mm_abs_epi16(m));
	}

	Vector4i abs32() const 
	{
		return Vector4i(_mm_abs_epi32(m));
	}
	
	#else

	Vector4i abs8() const 
	{
		Vector4i v = zero().sub8(*this);

		return blend8(v, v.gt8(zero()));
	}

	Vector4i abs16() const 
	{
		Vector4i v = zero().sub16(*this);

		return blend8(v, v.gt16(zero()));
	}

	Vector4i abs32() const 
	{
		Vector4i v = zero().sub32(*this);

		return blend8(v, v.gt32(zero()));
	}

	#endif

	int sum32() const
	{
		Vector4i v = xyxy() + zwzw();
		
		return (v + v.yxwz()).extract32<0>();
	}

	#if _M_SSE >= 0x401

	Vector4i sat_i8(const Vector4i& a, const Vector4i& b) const 
	{
		return max_i8(a).min_i8(b);
	}

	Vector4i sat_i8(const Vector4i& a) const 
	{
		return max_i8(a.xyxy()).min_i8(a.zwzw());
	}

	#endif

	Vector4i sat_i16(const Vector4i& a, const Vector4i& b) const 
	{
		return max_i16(a).min_i16(b);
	}

	Vector4i sat_i16(const Vector4i& a) const 
	{
		return max_i16(a.xyxy()).min_i16(a.zwzw());
	}

	#if _M_SSE >= 0x401

	Vector4i sat_i32(const Vector4i& a, const Vector4i& b) const 
	{
		return max_i32(a).min_i32(b);
	}

	Vector4i sat_i32(const Vector4i& a) const 
	{
		return max_i32(a.xyxy()).min_i32(a.zwzw());
	}

	#else

	Vector4i sat_i32(const Vector4i& a, const Vector4i& b) const 
	{
		Vector4i v;

		v.x = min(max(x, a.x), b.x);
		v.y = min(max(y, a.y), b.y);
		v.z = min(max(z, a.z), b.z);
		v.w = min(max(w, a.w), b.w);

		return v;
	}

	Vector4i sat_i32(const Vector4i& a) const 
	{
		Vector4i v;

		v.x = min(max(x, a.x), a.z);
		v.y = min(max(y, a.y), a.w);
		v.z = min(max(z, a.x), a.z);
		v.w = min(max(w, a.y), a.w);

		return v;
	}

	#endif

	Vector4i sat_u8(const Vector4i& a, const Vector4i& b) const 
	{
		return max_u8(a).min_u8(b);
	}

	Vector4i sat_u8(const Vector4i& a) const 
	{
		return max_u8(a.xyxy()).min_u8(a.zwzw());
	}

	#if _M_SSE >= 0x401

	Vector4i sat_u16(const Vector4i& a, const Vector4i& b) const 
	{
		return max_u16(a).min_u16(b);
	}

	Vector4i sat_u16(const Vector4i& a) const 
	{
		return max_u16(a.xyxy()).min_u16(a.zwzw());
	}

	#endif

	#if _M_SSE >= 0x401

	Vector4i sat_u32(const Vector4i& a, const Vector4i& b) const 
	{
		return max_u32(a).min_u32(b);
	}

	Vector4i sat_u32(const Vector4i& a) const 
	{
		return max_u32(a.xyxy()).min_u32(a.zwzw());
	}

	#endif

	#if _M_SSE >= 0x401

	Vector4i min_i8(const Vector4i& a) const
	{
		return Vector4i(_mm_min_epi8(m, a));
	}

	Vector4i max_i8(const Vector4i& a) const
	{
		return Vector4i(_mm_max_epi8(m, a));
	}

	#endif

	Vector4i min_i16(const Vector4i& a) const
	{
		return Vector4i(_mm_min_epi16(m, a));
	}

	Vector4i max_i16(const Vector4i& a) const
	{
		return Vector4i(_mm_max_epi16(m, a));
	}

	#if _M_SSE >= 0x401

	Vector4i min_i32(const Vector4i& a) const
	{
		return Vector4i(_mm_min_epi32(m, a));
	}

	Vector4i max_i32(const Vector4i& a) const
	{
		return Vector4i(_mm_max_epi32(m, a));
	}

	#endif

	Vector4i min_u8(const Vector4i& a) const
	{
		return Vector4i(_mm_min_epu8(m, a));
	}

	Vector4i max_u8(const Vector4i& a) const
	{
		return Vector4i(_mm_max_epu8(m, a));
	}

	#if _M_SSE >= 0x401

	Vector4i min_u16(const Vector4i& a) const
	{
		return Vector4i(_mm_min_epu16(m, a));
	}

	Vector4i max_u16(const Vector4i& a) const
	{
		return Vector4i(_mm_max_epu16(m, a));
	}

	Vector4i min_u32(const Vector4i& a) const
	{
		return Vector4i(_mm_min_epu32(m, a));
	}

	Vector4i max_u32(const Vector4i& a) const
	{
		return Vector4i(_mm_max_epu32(m, a));
	}

	#endif

	static int min_i16(int a, int b)
	{
		 return store(load(a).min_i16(load(b)));
	}

	Vector4i clamp8() const
	{
		return pu16().upl8();
	}

	Vector4i blend8(const Vector4i& a, const Vector4i& mask) const
	{
		#if _M_SSE >= 0x401

		return Vector4i(_mm_blendv_epi8(m, a, mask));

		#else

		return Vector4i(_mm_or_si128(_mm_andnot_si128(mask, m), _mm_and_si128(mask, a)));

		#endif
	}

	#if _M_SSE >= 0x401

	template<int mask> Vector4i blend16(const Vector4i& a) const
	{
		return Vector4i(_mm_blend_epi16(m, a, mask));
	}

	#endif

	Vector4i blend(const Vector4i& a, const Vector4i& mask) const
	{
		return Vector4i(_mm_or_si128(_mm_andnot_si128(mask, m), _mm_and_si128(mask, a)));
	}

	Vector4i mix16(const Vector4i& a) const
	{
		#if _M_SSE >= 0x401

		return blend16<0xaa>(a);
		
		#else
		
		return blend8(a, Vector4i::xffff0000());

		#endif
	}

	#if _M_SSE >= 0x301

	Vector4i shuffle8(const Vector4i& mask) const
	{
		return Vector4i(_mm_shuffle_epi8(m, mask));
	}

	#endif

	Vector4i ps16(const Vector4i& a) const
	{
		return Vector4i(_mm_packs_epi16(m, a));
	}

	Vector4i ps16() const
	{
		return Vector4i(_mm_packs_epi16(m, m));
	}

	Vector4i pu16(const Vector4i& a) const
	{
		return Vector4i(_mm_packus_epi16(m, a));
	}

	Vector4i pu16() const
	{
		return Vector4i(_mm_packus_epi16(m, m));
	}

	Vector4i ps32(const Vector4i& a) const
	{
		return Vector4i(_mm_packs_epi32(m, a));
	}
	
	Vector4i ps32() const
	{
		return Vector4i(_mm_packs_epi32(m, m));
	}
	
	#if _M_SSE >= 0x401

	Vector4i pu32(const Vector4i& a) const
	{
		return Vector4i(_mm_packus_epi32(m, a));
	}

	Vector4i pu32() const
	{
		return Vector4i(_mm_packus_epi32(m, m));
	}

	#endif

	Vector4i upl8(const Vector4i& a) const
	{
		return Vector4i(_mm_unpacklo_epi8(m, a));
	}

	Vector4i uph8(const Vector4i& a) const
	{
		return Vector4i(_mm_unpackhi_epi8(m, a));
	}

	Vector4i upl16(const Vector4i& a) const
	{
		return Vector4i(_mm_unpacklo_epi16(m, a));
	}

	Vector4i uph16(const Vector4i& a) const
	{
		return Vector4i(_mm_unpackhi_epi16(m, a));
	}

	Vector4i upl32(const Vector4i& a) const
	{
		return Vector4i(_mm_unpacklo_epi32(m, a));
	}

	Vector4i uph32(const Vector4i& a) const
	{
		return Vector4i(_mm_unpackhi_epi32(m, a));
	}

	Vector4i upl64(const Vector4i& a) const
	{
		return Vector4i(_mm_unpacklo_epi64(m, a));
	}

	Vector4i uph64(const Vector4i& a) const
	{
		return Vector4i(_mm_unpackhi_epi64(m, a));
	}

	Vector4i upl8() const
	{
		#if 0 // _M_SSE >= 0x401 // TODO: compiler bug

		return Vector4i(_mm_cvtepu8_epi16(m));
		
		#else

		return Vector4i(_mm_unpacklo_epi8(m, _mm_setzero_si128()));

		#endif
	}

	Vector4i uph8() const
	{
		return Vector4i(_mm_unpackhi_epi8(m, _mm_setzero_si128()));
	}

	Vector4i upl16() const
	{
		#if 0 //_M_SSE >= 0x401 // TODO: compiler bug

		return Vector4i(_mm_cvtepu16_epi32(m));
		
		#else

		return Vector4i(_mm_unpacklo_epi16(m, _mm_setzero_si128()));

		#endif
	}

	Vector4i uph16() const
	{
		return Vector4i(_mm_unpackhi_epi16(m, _mm_setzero_si128()));
	}

	Vector4i upl32() const
	{
		#if 0 //_M_SSE >= 0x401 // TODO: compiler bug

		return Vector4i(_mm_cvtepu32_epi64(m));
		
		#else

		return Vector4i(_mm_unpacklo_epi32(m, _mm_setzero_si128()));

		#endif
	}

	Vector4i uph32() const
	{
		return Vector4i(_mm_unpackhi_epi32(m, _mm_setzero_si128()));
	}

	Vector4i upl64() const
	{
		return Vector4i(_mm_unpacklo_epi64(m, _mm_setzero_si128()));
	}

	Vector4i uph64() const
	{
		return Vector4i(_mm_unpackhi_epi64(m, _mm_setzero_si128()));
	}

	#if _M_SSE >= 0x401

	// WARNING!!!
	//
	// MSVC (2008, 2010 ctp) believes that there is a "mem, reg" form of the pmovz/sx* instructions,
	// turning these intrinsics into a minefield, don't spill regs when using them...

	Vector4i i8to16() const
	{
		return Vector4i(_mm_cvtepi8_epi16(m));
	}

	Vector4i u8to16() const
	{
		return Vector4i(_mm_cvtepu8_epi16(m));
	}

	Vector4i i8to32() const
	{
		return Vector4i(_mm_cvtepi8_epi32(m));
	}

	Vector4i u8to32() const
	{
		return Vector4i(_mm_cvtepu8_epi32(m));
	}

	Vector4i i8to64() const
	{
		return Vector4i(_mm_cvtepi8_epi64(m));
	}

	Vector4i u8to64() const
	{
		return Vector4i(_mm_cvtepu16_epi64(m));
	}

	Vector4i i16to32() const
	{
		return Vector4i(_mm_cvtepi16_epi32(m));
	}

	Vector4i u16to32() const
	{
		return Vector4i(_mm_cvtepu16_epi32(m));
	}

	Vector4i i16to64() const
	{
		return Vector4i(_mm_cvtepi16_epi64(m));
	}

	Vector4i u16to64() const
	{
		return Vector4i(_mm_cvtepu16_epi64(m));
	}

	Vector4i i32to64() const
	{
		return Vector4i(_mm_cvtepi32_epi64(m));
	}

	Vector4i u32to64() const
	{
		return Vector4i(_mm_cvtepu32_epi64(m));
	}

	#else

	Vector4i u8to16() const
	{
		return upl8();
	}

	Vector4i u8to32() const
	{
		return upl8().upl16();
	}

	Vector4i u8to64() const
	{
		return upl8().upl16().upl32();
	}

	Vector4i u16to32() const
	{
		return upl16();
	}

	Vector4i u16to64() const
	{
		return upl16().upl32();
	}

	Vector4i u32to64() const
	{
		return upl32();
	}

	#endif

	template<int i> Vector4i srl() const
	{
		#pragma warning(push)
		#pragma warning(disable: 4556)

		return Vector4i(_mm_srli_si128(m, i));

		#pragma warning(pop)
	}

	template<int i> Vector4i srl(const Vector4i& v)
	{
		#if _M_SSE >= 0x301

		return Vector4i(_mm_alignr_epi8(v.m, m, i));

		#else

		if(i == 0) return *this;
		else if(i < 16) return srl<i>() | v.sll<16 - i>();
		else if(i == 16) return v;
		else if(i < 32) return v.srl<i - 16>();
		else return zero();

		#endif
	}

	template<int i> Vector4i sll() const
	{
		#pragma warning(push)
		#pragma warning(disable: 4556)

		return Vector4i(_mm_slli_si128(m, i));

		#pragma warning(pop)
	}

	Vector4i sra16(int i) const
	{
		return Vector4i(_mm_srai_epi16(m, i));
	}

	Vector4i sra32(int i) const
	{
		return Vector4i(_mm_srai_epi32(m, i));
	}

	Vector4i sll16(int i) const
	{
		return Vector4i(_mm_slli_epi16(m, i));
	}

	Vector4i sll32(int i) const
	{
		return Vector4i(_mm_slli_epi32(m, i));
	}

	Vector4i sll64(int i) const
	{
		return Vector4i(_mm_slli_epi64(m, i));
	}

	Vector4i srl16(int i) const
	{
		return Vector4i(_mm_srli_epi16(m, i));
	}

	Vector4i srl32(int i) const
	{
		return Vector4i(_mm_srli_epi32(m, i));
	}

	Vector4i srl64(int i) const
	{
		return Vector4i(_mm_srli_epi64(m, i));
	}

	Vector4i add8(const Vector4i& v) const
	{
		return Vector4i(_mm_add_epi8(m, v.m));
	}

	Vector4i add16(const Vector4i& v) const
	{
		return Vector4i(_mm_add_epi16(m, v.m));
	}
	
	Vector4i add32(const Vector4i& v) const
	{
		return Vector4i(_mm_add_epi32(m, v.m));
	}

	Vector4i add64(const Vector4i& v) const
	{
		return Vector4i(_mm_add_epi64(m, v.m));
	}

	Vector4i adds8(const Vector4i& v) const
	{
		return Vector4i(_mm_adds_epi8(m, v.m));
	}

	Vector4i adds16(const Vector4i& v) const
	{
		return Vector4i(_mm_adds_epi16(m, v.m));
	}

	Vector4i addus8(const Vector4i& v) const
	{
		return Vector4i(_mm_adds_epu8(m, v.m));
	}

	Vector4i addus16(const Vector4i& v) const
	{
		return Vector4i(_mm_adds_epu16(m, v.m));
	}

	Vector4i sub8(const Vector4i& v) const
	{
		return Vector4i(_mm_sub_epi8(m, v.m));
	}

	Vector4i sub16(const Vector4i& v) const
	{
		return Vector4i(_mm_sub_epi16(m, v.m));
	}

	Vector4i sub32(const Vector4i& v) const
	{
		return Vector4i(_mm_sub_epi32(m, v.m));
	}

	Vector4i sub64(const Vector4i& v) const
	{
		return Vector4i(_mm_sub_epi64(m, v.m));
	}

	Vector4i subs8(const Vector4i& v) const
	{
		return Vector4i(_mm_subs_epi8(m, v.m));
	}

	Vector4i subs16(const Vector4i& v) const
	{
		return Vector4i(_mm_subs_epi16(m, v.m));
	}

	Vector4i subus8(const Vector4i& v) const
	{
		return Vector4i(_mm_subs_epu8(m, v.m));
	}

	Vector4i subus16(const Vector4i& v) const
	{
		return Vector4i(_mm_subs_epu16(m, v.m));
	}

	Vector4i avg8(const Vector4i& v) const
	{
		return Vector4i(_mm_avg_epu8(m, v.m));
	}

	Vector4i avg16(const Vector4i& v) const
	{
		return Vector4i(_mm_avg_epu16(m, v.m));
	}

	Vector4i mul16hs(const Vector4i& v) const
	{
		return Vector4i(_mm_mulhi_epi16(m, v.m));
	}

	Vector4i mul16hu(const Vector4i& v) const
	{
		return Vector4i(_mm_mulhi_epu16(m, v.m));
	}

	Vector4i mul16l(const Vector4i& v) const
	{
		return Vector4i(_mm_mullo_epi16(m, v.m));
	}

	Vector4i madd(const Vector4i& v) const
	{
		return Vector4i(_mm_madd_epi16(m, v.m));
	}

	#if _M_SSE >= 0x301

	Vector4i mul16hrs(const Vector4i& v) const
	{
		return Vector4i(_mm_mulhrs_epi16(m, v.m));
	}

	#endif

	template<int shift> Vector4i lerp16(const Vector4i& a, const Vector4i& f) const
	{
		// (a - this) * f << shift + this

		return add16(a.sub16(*this).modulate16<shift>(f));
	}

	template<int shift> static Vector4i lerp16(const Vector4i& a, const Vector4i& b, const Vector4i& c)
	{
		// (a - b) * c << shift

		return a.sub16(b).modulate16<shift>(c);
	}

	template<int shift> static Vector4i lerp16(const Vector4i& a, const Vector4i& b, const Vector4i& c, const Vector4i& d)
	{
		// (a - b) * c << shift + d

		return d.add16(a.sub16(b).modulate16<shift>(c));
	}

	template<int shift> Vector4i modulate16(const Vector4i& f) const
	{
		// a * f << shift

		#if _M_SSE >= 0x301

		if(shift == 0) 
		{
			return mul16hrs(f);
		}

		#endif

		return sll16(shift + 1).mul16hs(f);
	}

	bool eq(const Vector4i& v) const
	{
		#if _M_SSE >= 0x401
		// pxor, ptest, je
		Vector4i t = *this ^ v;
		return _mm_testz_si128(t, t) != 0;
		#else
		// pcmpeqd, pmovmskb, cmp, je
		return eq32(v).alltrue();
		#endif
	}

	Vector4i eq8(const Vector4i& v) const
	{
		return Vector4i(_mm_cmpeq_epi8(m, v.m));
	}

	Vector4i eq16(const Vector4i& v) const
	{
		return Vector4i(_mm_cmpeq_epi16(m, v.m));
	}

	Vector4i eq32(const Vector4i& v) const
	{
		return Vector4i(_mm_cmpeq_epi32(m, v.m));
	}

	Vector4i neq8(const Vector4i& v) const
	{
		return ~eq8(v);
	}

	Vector4i neq16(const Vector4i& v) const
	{
		return ~eq16(v);
	}

	Vector4i neq32(const Vector4i& v) const
	{
		return ~eq32(v);
	}

	Vector4i gt8(const Vector4i& v) const
	{
		return Vector4i(_mm_cmpgt_epi8(m, v.m));
	}

	Vector4i gt16(const Vector4i& v) const
	{
		return Vector4i(_mm_cmpgt_epi16(m, v.m));
	}

	Vector4i gt32(const Vector4i& v) const
	{
		return Vector4i(_mm_cmpgt_epi32(m, v.m));
	}

	Vector4i lt8(const Vector4i& v) const
	{
		return Vector4i(_mm_cmplt_epi8(m, v.m));
	}

	Vector4i lt16(const Vector4i& v) const
	{
		return Vector4i(_mm_cmplt_epi16(m, v.m));
	}

	Vector4i lt32(const Vector4i& v) const
	{
		return Vector4i(_mm_cmplt_epi32(m, v.m));
	}

	Vector4i andnot(const Vector4i& v) const
	{
		return Vector4i(_mm_andnot_si128(v.m, m));
	}

	int mask() const
	{
		return _mm_movemask_epi8(m);
	}

	bool alltrue() const
	{
		return _mm_movemask_epi8(m) == 0xffff;
	}

	bool anytrue() const
	{
		return !allfalse();
	}

	bool allfalse() const
	{
		#if _M_SSE >= 0x401
		return _mm_testz_si128(m, m) != 0;
		#else
		return _mm_movemask_epi8(m) == 0;
		#endif
	}

	#if _M_SSE >= 0x401

	template<int i> Vector4i insert8(int a) const
	{
		return Vector4i(_mm_insert_epi8(m, a, i));
	}

	#endif

	template<int i> int extract8() const
	{
		#if _M_SSE >= 0x401
		return _mm_extract_epi8(m, i);
		#else
		return (int)u8[i];
		#endif
	}

	template<int i> Vector4i insert16(int a) const
	{
		return Vector4i(_mm_insert_epi16(m, a, i));
	}

	template<int i> int extract16() const
	{
		return _mm_extract_epi16(m, i);
	}

	#if _M_SSE >= 0x401

	template<int i> Vector4i insert32(int a) const
	{
		return Vector4i(_mm_insert_epi32(m, a, i));
	}

	#endif

	template<int i> int extract32() const
	{
		if(i == 0) return Vector4i::store(*this);
		#if _M_SSE >= 0x401
		return _mm_extract_epi32(m, i);
		#else
		return i32[i];
		#endif
	}

	#ifdef _M_AMD64

	#if _M_SSE >= 0x401

	template<int i> Vector4i insert64(int64 a) const
	{
		return Vector4i(_mm_insert_epi64(m, a, i));
	}

	#endif

	template<int i> int64 extract64() const
	{
		if(i == 0) return Vector4i::storeq(*this);
		#if _M_SSE >= 0x401
		return _mm_extract_epi64(m, i);
		#else
		return i64[i];
		#endif
	}

	#endif

	#if _M_SSE >= 0x401

	template<int src, class T> __forceinline Vector4i gather8_4(const T* ptr) const
	{
		Vector4i v;

		v = load((int)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert8<1>((int)ptr[extract8<src + 0>() >> 4]);
		v = v.insert8<2>((int)ptr[extract8<src + 1>() & 0xf]);
		v = v.insert8<3>((int)ptr[extract8<src + 1>() >> 4]);
		v = v.insert8<4>((int)ptr[extract8<src + 2>() & 0xf]);
		v = v.insert8<5>((int)ptr[extract8<src + 2>() >> 4]);
		v = v.insert8<6>((int)ptr[extract8<src + 3>() & 0xf]);
		v = v.insert8<7>((int)ptr[extract8<src + 3>() >> 4]);
		v = v.insert8<8>((int)ptr[extract8<src + 4>() & 0xf]);
		v = v.insert8<9>((int)ptr[extract8<src + 4>() >> 4]);
		v = v.insert8<10>((int)ptr[extract8<src + 5>() & 0xf]);
		v = v.insert8<11>((int)ptr[extract8<src + 5>() >> 4]);
		v = v.insert8<12>((int)ptr[extract8<src + 6>() & 0xf]);
		v = v.insert8<13>((int)ptr[extract8<src + 6>() >> 4]);
		v = v.insert8<14>((int)ptr[extract8<src + 7>() & 0xf]);
		v = v.insert8<15>((int)ptr[extract8<src + 7>() >> 4]);

		return v;
	}

	template<class T> __forceinline Vector4i gather8_8(const T* ptr) const
	{
		Vector4i v;

		v = load((int)ptr[extract8<0>()]);
		v = v.insert8<1>((int)ptr[extract8<1>()]);
		v = v.insert8<2>((int)ptr[extract8<2>()]);
		v = v.insert8<3>((int)ptr[extract8<3>()]);
		v = v.insert8<4>((int)ptr[extract8<4>()]);
		v = v.insert8<5>((int)ptr[extract8<5>()]);
		v = v.insert8<6>((int)ptr[extract8<6>()]);
		v = v.insert8<7>((int)ptr[extract8<7>()]);
		v = v.insert8<8>((int)ptr[extract8<8>()]);
		v = v.insert8<9>((int)ptr[extract8<9>()]);
		v = v.insert8<10>((int)ptr[extract8<10>()]);
		v = v.insert8<11>((int)ptr[extract8<11>()]);
		v = v.insert8<12>((int)ptr[extract8<12>()]);
		v = v.insert8<13>((int)ptr[extract8<13>()]);
		v = v.insert8<14>((int)ptr[extract8<14>()]);
		v = v.insert8<15>((int)ptr[extract8<15>()]);

		return v;
	}

	template<int dst, class T> __forceinline Vector4i gather8_16(const T* ptr, const Vector4i& a) const
	{
		Vector4i v = a;

		v = v.insert8<dst + 0>((int)ptr[extract16<0>()]);
		v = v.insert8<dst + 1>((int)ptr[extract16<1>()]);
		v = v.insert8<dst + 2>((int)ptr[extract16<2>()]);
		v = v.insert8<dst + 3>((int)ptr[extract16<3>()]);
		v = v.insert8<dst + 4>((int)ptr[extract16<4>()]);
		v = v.insert8<dst + 5>((int)ptr[extract16<5>()]);
		v = v.insert8<dst + 6>((int)ptr[extract16<6>()]);
		v = v.insert8<dst + 7>((int)ptr[extract16<7>()]);

		return v;
	}

	template<int dst, class T> __forceinline Vector4i gather8_32(const T* ptr, const Vector4i& a) const
	{
		Vector4i v = a;

		v = v.insert8<dst + 0>((int)ptr[extract32<0>()]);
		v = v.insert8<dst + 1>((int)ptr[extract32<1>()]);
		v = v.insert8<dst + 2>((int)ptr[extract32<2>()]);
		v = v.insert8<dst + 3>((int)ptr[extract32<3>()]);

		return v;
	}

	#endif

	template<int src, class T> __forceinline Vector4i gather16_4(const T* ptr) const
	{
		Vector4i v;

		v = load((int)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert16<1>((int)ptr[extract8<src + 0>() >> 4]);
		v = v.insert16<2>((int)ptr[extract8<src + 1>() & 0xf]);
		v = v.insert16<3>((int)ptr[extract8<src + 1>() >> 4]);
		v = v.insert16<4>((int)ptr[extract8<src + 2>() & 0xf]);
		v = v.insert16<5>((int)ptr[extract8<src + 2>() >> 4]);
		v = v.insert16<6>((int)ptr[extract8<src + 3>() & 0xf]);
		v = v.insert16<7>((int)ptr[extract8<src + 3>() >> 4]);

		return v;
	}

	template<int src, class T> __forceinline Vector4i gather16_8(const T* ptr) const
	{
		Vector4i v;

		v = load((int)ptr[extract8<src + 0>()]);
		v = v.insert16<1>((int)ptr[extract8<src + 1>()]);
		v = v.insert16<2>((int)ptr[extract8<src + 2>()]);
		v = v.insert16<3>((int)ptr[extract8<src + 3>()]);
		v = v.insert16<4>((int)ptr[extract8<src + 4>()]);
		v = v.insert16<5>((int)ptr[extract8<src + 5>()]);
		v = v.insert16<6>((int)ptr[extract8<src + 6>()]);
		v = v.insert16<7>((int)ptr[extract8<src + 7>()]);

		return v;
	}

	template<class T>__forceinline Vector4i gather16_16(const T* ptr) const
	{
		Vector4i v;

		v = load((int)ptr[extract16<0>()]);
		v = v.insert16<1>((int)ptr[extract16<1>()]);
		v = v.insert16<2>((int)ptr[extract16<2>()]);
		v = v.insert16<3>((int)ptr[extract16<3>()]);
		v = v.insert16<4>((int)ptr[extract16<4>()]);
		v = v.insert16<5>((int)ptr[extract16<5>()]);
		v = v.insert16<6>((int)ptr[extract16<6>()]);
		v = v.insert16<7>((int)ptr[extract16<7>()]);

		return v;
	}

	template<class T1, class T2>__forceinline Vector4i gather16_16(const T1* ptr1, const T2* ptr2) const
	{
		Vector4i v;

		v = load((int)ptr2[ptr1[extract16<0>()]]);
		v = v.insert16<1>((int)ptr2[ptr1[extract16<1>()]]);
		v = v.insert16<2>((int)ptr2[ptr1[extract16<2>()]]);
		v = v.insert16<3>((int)ptr2[ptr1[extract16<3>()]]);
		v = v.insert16<4>((int)ptr2[ptr1[extract16<4>()]]);
		v = v.insert16<5>((int)ptr2[ptr1[extract16<5>()]]);
		v = v.insert16<6>((int)ptr2[ptr1[extract16<6>()]]);
		v = v.insert16<7>((int)ptr2[ptr1[extract16<7>()]]);

		return v;
	}

	template<int dst, class T> __forceinline Vector4i gather16_32(const T* ptr, const Vector4i& a) const
	{
		Vector4i v = a;

		v = v.insert16<dst + 0>((int)ptr[extract32<0>()]);
		v = v.insert16<dst + 1>((int)ptr[extract32<1>()]);
		v = v.insert16<dst + 2>((int)ptr[extract32<2>()]);
		v = v.insert16<dst + 3>((int)ptr[extract32<3>()]);

		return v;
	}

	#if _M_SSE >= 0x401

	template<int src, class T> __forceinline Vector4i gather32_4(const T* ptr) const
	{
		Vector4i v;

		v = load((int)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert32<1>((int)ptr[extract8<src + 0>() >> 4]);
		v = v.insert32<2>((int)ptr[extract8<src + 1>() & 0xf]);
		v = v.insert32<3>((int)ptr[extract8<src + 1>() >> 4]);
		return v;
	}

	template<int src, class T> __forceinline Vector4i gather32_8(const T* ptr) const
	{
		Vector4i v;

		v = load((int)ptr[extract8<src + 0>()]);
		v = v.insert32<1>((int)ptr[extract8<src + 1>()]);
		v = v.insert32<2>((int)ptr[extract8<src + 2>()]);
		v = v.insert32<3>((int)ptr[extract8<src + 3>()]);

		return v;
	}

	template<int src, class T> __forceinline Vector4i gather32_16(const T* ptr) const
	{
		Vector4i v;

		v = load((int)ptr[extract16<src + 0>()]);
		v = v.insert32<1>((int)ptr[extract16<src + 1>()]);
		v = v.insert32<2>((int)ptr[extract16<src + 2>()]);
		v = v.insert32<3>((int)ptr[extract16<src + 3>()]);

		return v;
	}

	template<class T> __forceinline Vector4i gather32_32(const T* ptr) const
	{
		Vector4i v;

		v = load((int)ptr[extract32<0>()]);
		v = v.insert32<1>((int)ptr[extract32<1>()]);
		v = v.insert32<2>((int)ptr[extract32<2>()]);
		v = v.insert32<3>((int)ptr[extract32<3>()]);

		return v;
	}

	template<class T1, class T2> __forceinline Vector4i gather32_32(const T1* ptr1, const T2* ptr2) const
	{
		Vector4i v;

		v = load((int)ptr2[ptr1[extract32<0>()]]);
		v = v.insert32<1>((int)ptr2[ptr1[extract32<1>()]]);
		v = v.insert32<2>((int)ptr2[ptr1[extract32<2>()]]);
		v = v.insert32<3>((int)ptr2[ptr1[extract32<3>()]]);

		return v;
	}

	#else

	template<int src, class T> __forceinline Vector4i gather32_4(const T* ptr) const
	{
		return Vector4i(
			(int)ptr[extract8<src + 0>() & 0xf],
			(int)ptr[extract8<src + 0>() >> 4],
			(int)ptr[extract8<src + 1>() & 0xf],
			(int)ptr[extract8<src + 1>() >> 4]);
	}

	template<int src, class T> __forceinline Vector4i gather32_8(const T* ptr) const
	{
		return Vector4i(
			(int)ptr[extract8<src + 0>()], 
			(int)ptr[extract8<src + 1>()],
			(int)ptr[extract8<src + 2>()],
			(int)ptr[extract8<src + 3>()]);
	}

	template<int src, class T> __forceinline Vector4i gather32_16(const T* ptr) const
	{
		return Vector4i(
			(int)ptr[extract16<src + 0>()],
			(int)ptr[extract16<src + 1>()],
			(int)ptr[extract16<src + 2>()],
			(int)ptr[extract16<src + 3>()]);
	}

	template<class T> __forceinline Vector4i gather32_32(const T* ptr) const
	{
		return Vector4i(
			(int)ptr[extract32<0>()], 
			(int)ptr[extract32<1>()], 
			(int)ptr[extract32<2>()],
			(int)ptr[extract32<3>()]);
	}

	template<class T1, class T2> __forceinline Vector4i gather32_32(const T1* ptr1, const T2* ptr2) const
	{
		return Vector4i(
			(int)ptr2[ptr1[extract32<0>()]], 
			(int)ptr2[ptr1[extract32<1>()]],
			(int)ptr2[ptr1[extract32<2>()]],
			(int)ptr2[ptr1[extract32<3>()]]);
	}

	#endif

	#if defined(_M_AMD64) && _M_SSE >= 0x401

	template<int src, class T> __forceinline Vector4i gather64_4(const T* ptr) const
	{
		Vector4i v;

		v = loadq((int64)ptr[extract8<src + 0>() & 0xf]);
		v = v.insert64<1>((int64)ptr[extract8<src + 0>() >> 4]);

		return v;
	}

	template<int src, class T> __forceinline Vector4i gather64_8(const T* ptr) const
	{
		Vector4i v;

		v = loadq((int64)ptr[extract8<src + 0>()]);
		v = v.insert64<1>((int64)ptr[extract8<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline Vector4i gather64_16(const T* ptr) const
	{
		Vector4i v;

		v = loadq((int64)ptr[extract16<src + 0>()]);
		v = v.insert64<1>((int64)ptr[extract16<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline Vector4i gather64_32(const T* ptr) const
	{
		Vector4i v;

		v = loadq((int64)ptr[extract32<src + 0>()]);
		v = v.insert64<1>((int64)ptr[extract32<src + 1>()]);

		return v;
	}

	template<class T> __forceinline Vector4i gather64_64(const T* ptr) const
	{
		Vector4i v;

		v = loadq((int64)ptr[extract64<0>()]);
		v = v.insert64<1>((int64)ptr[extract64<1>()]);

		return v;
	}

	#else

	template<int src, class T> __forceinline Vector4i gather64_4(const T* ptr) const
	{
		Vector4i v;

		v = loadu(&ptr[extract8<src + 0>() & 0xf], &ptr[extract8<src + 0>() >> 4]);

		return v;
	}

	template<int src, class T> __forceinline Vector4i gather64_8(const T* ptr) const
	{
		Vector4i v;

		v = load(&ptr[extract8<src + 0>()], &ptr[extract8<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline Vector4i gather64_16(const T* ptr) const
	{
		Vector4i v;

		v = load(&ptr[extract16<src + 0>()], &ptr[extract16<src + 1>()]);

		return v;
	}

	template<int src, class T> __forceinline Vector4i gather64_32(const T* ptr) const
	{
		Vector4i v;

		v = load(&ptr[extract32<src + 0>()], &ptr[extract32<src + 1>()]);

		return v;
	}

	#endif

	#if _M_SSE >= 0x401

	template<class T> __forceinline void gather8_4(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather8_4<0>(ptr);
		dst[1] = gather8_4<8>(ptr);
	}

	__forceinline void gather8_8(const uint8* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather8_8<>(ptr);
	}

	#endif

	template<class T> __forceinline void gather16_4(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather16_4<0>(ptr);
		dst[1] = gather16_4<4>(ptr);
		dst[2] = gather16_4<8>(ptr);
		dst[3] = gather16_4<12>(ptr);
	}

	template<class T> __forceinline void gather16_8(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather16_8<0>(ptr);
		dst[1] = gather16_8<8>(ptr);
	}

	template<class T> __forceinline void gather16_16(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather16_16<>(ptr);
	}

	template<class T> __forceinline void gather32_4(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather32_4<0>(ptr);
		dst[1] = gather32_4<2>(ptr);
		dst[2] = gather32_4<4>(ptr);
		dst[3] = gather32_4<6>(ptr);
		dst[4] = gather32_4<8>(ptr);
		dst[5] = gather32_4<10>(ptr);
		dst[6] = gather32_4<12>(ptr);
		dst[7] = gather32_4<14>(ptr);
	}

	template<class T> __forceinline void gather32_8(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather32_8<0>(ptr);
		dst[1] = gather32_8<4>(ptr);
		dst[2] = gather32_8<8>(ptr);
		dst[3] = gather32_8<12>(ptr);
	}

	template<class T> __forceinline void gather32_16(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather32_16<0>(ptr);
		dst[1] = gather32_16<4>(ptr);
	}

	template<class T> __forceinline void gather32_32(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather32_32<>(ptr);
	}

	template<class T> __forceinline void gather64_4(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather64_4<0>(ptr);
		dst[1] = gather64_4<1>(ptr);
		dst[2] = gather64_4<2>(ptr);
		dst[3] = gather64_4<3>(ptr);
		dst[4] = gather64_4<4>(ptr);
		dst[5] = gather64_4<5>(ptr);
		dst[6] = gather64_4<6>(ptr);
		dst[7] = gather64_4<7>(ptr);
		dst[8] = gather64_4<8>(ptr);
		dst[9] = gather64_4<9>(ptr);
		dst[10] = gather64_4<10>(ptr);
		dst[11] = gather64_4<11>(ptr);
		dst[12] = gather64_4<12>(ptr);
		dst[13] = gather64_4<13>(ptr);
		dst[14] = gather64_4<14>(ptr);
		dst[15] = gather64_4<15>(ptr);
	}

	template<class T> __forceinline void gather64_8(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather64_8<0>(ptr);
		dst[1] = gather64_8<2>(ptr);
		dst[2] = gather64_8<4>(ptr);
		dst[3] = gather64_8<6>(ptr);
		dst[4] = gather64_8<8>(ptr);
		dst[5] = gather64_8<10>(ptr);
		dst[6] = gather64_8<12>(ptr);
		dst[7] = gather64_8<14>(ptr);
	}

	template<class T> __forceinline void gather64_16(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather64_16<0>(ptr);
		dst[1] = gather64_16<2>(ptr);
		dst[2] = gather64_16<4>(ptr);
		dst[3] = gather64_16<8>(ptr);
	}

	template<class T> __forceinline void gather64_32(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather64_32<0>(ptr);
		dst[1] = gather64_32<2>(ptr);
	}

	#ifdef _M_AMD64

	template<class T> __forceinline void gather64_64(const T* RESTRICT ptr, Vector4i* RESTRICT dst) const
	{
		dst[0] = gather64_64<>(ptr);
	}

	#endif

	static Vector4i loadnt(const void* p)
	{
		#if _M_SSE >= 0x401

		return Vector4i(_mm_stream_load_si128((__m128i*)p));

		#else

		return Vector4i(_mm_load_si128((__m128i*)p));

		#endif
	}

	static Vector4i loadl(const void* p)
	{
		return Vector4i(_mm_loadl_epi64((__m128i*)p));
	}

	static Vector4i loadh(const void* p)
	{
		return Vector4i(_mm_castps_si128(_mm_loadh_pi(_mm_setzero_ps(), (__m64*)p)));
	}

	static Vector4i loadh(const void* p, const Vector4i& v)
	{
		return Vector4i(_mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(v.m), (__m64*)p)));
	}

	static Vector4i load(const void* pl, const void* ph)
	{
		return loadh(ph, loadl(pl));
	}
/*
	static Vector4i load(const void* pl, const void* ph)
	{
		__m128i lo = _mm_loadl_epi64((__m128i*)pl);
		__m128i hi = _mm_loadl_epi64((__m128i*)ph);

		return Vector4i(_mm_unpacklo_epi64(lo, hi));
	}
*/
	template<bool aligned> static Vector4i load(const void* p)
	{
		return Vector4i(aligned ? _mm_load_si128((__m128i*)p) : _mm_loadu_si128((__m128i*)p));
	}

	static Vector4i load(int i)
	{
		return Vector4i(_mm_cvtsi32_si128(i));
	}

	#ifdef _M_AMD64

	static Vector4i loadq(int64 i)
	{
		return Vector4i(_mm_cvtsi64_si128(i));
	}

	#endif

	static void storent(void* p, const Vector4i& v)
	{
		_mm_stream_si128((__m128i*)p, v.m);
	}

	static void storel(void* p, const Vector4i& v)
	{
		_mm_storel_epi64((__m128i*)p, v.m);
	}

	static void storeh(void* p, const Vector4i& v)
	{
		_mm_storeh_pi((__m64*)p, _mm_castsi128_ps(v.m));
	}

	static void store(void* pl, void* ph, const Vector4i& v)
	{
		Vector4i::storel(pl, v);
		Vector4i::storeh(ph, v);
	}

	template<bool aligned> static void store(void* p, const Vector4i& v)
	{
		if(aligned) _mm_store_si128((__m128i*)p, v.m);
		else _mm_storeu_si128((__m128i*)p, v.m);
	}

	static int store(const Vector4i& v)
	{
		return _mm_cvtsi128_si32(v.m);
	}

	#ifdef _M_AMD64

	static int64 storeq(const Vector4i& v)
	{
		return _mm_cvtsi128_si64(v.m);
	}

	#endif

	static void storent(void* RESTRICT dst, const void* RESTRICT src, size_t size)
	{
		const Vector4i* s = (const Vector4i*)src;
		Vector4i* d = (Vector4i*)dst;

		if(size == 0) return;

		size_t i = 0;
		size_t j = size >> 6;

		for(; i < j; i++, s += 4, d += 4)
		{
			storent(&d[0], s[0]);
			storent(&d[1], s[1]);
			storent(&d[2], s[2]);
			storent(&d[3], s[3]);
		}

		size &= 63;

		if(size == 0) return;

		memcpy(d, s, size);
	}

	__forceinline static void transpose(Vector4i& a, Vector4i& b, Vector4i& c, Vector4i& d)
	{
		_MM_TRANSPOSE4_SI128(a.m, b.m, c.m, d.m);
	}

	__forceinline static void sw4(Vector4i& a, Vector4i& b, Vector4i& c, Vector4i& d)
	{
		const __m128i epi32_0f0f0f0f = _mm_set1_epi32(0x0f0f0f0f);

		Vector4i mask(epi32_0f0f0f0f);

		Vector4i e = (b << 4).blend(a, mask);
		Vector4i f = b.blend(a >> 4, mask);
		Vector4i g = (d << 4).blend(c, mask);
		Vector4i h = d.blend(c >> 4, mask);

		a = e.upl8(f);
		c = e.uph8(f);
		b = g.upl8(h);
		d = g.uph8(h);
	}

	__forceinline static void sw8(Vector4i& a, Vector4i& b, Vector4i& c, Vector4i& d)
	{
		Vector4i e = a;
		Vector4i f = c;

		a = e.upl8(b);
		c = e.uph8(b);
		b = f.upl8(d);
		d = f.uph8(d);
	}

	__forceinline static void sw16(Vector4i& a, Vector4i& b, Vector4i& c, Vector4i& d)
	{
		Vector4i e = a;
		Vector4i f = c;

		a = e.upl16(b);
		c = e.uph16(b);
		b = f.upl16(d);
		d = f.uph16(d);
	}

	__forceinline static void sw16rl(Vector4i& a, Vector4i& b, Vector4i& c, Vector4i& d)
	{
		Vector4i e = a;
		Vector4i f = c;

		a = b.upl16(e);
		c = e.uph16(b);
		b = d.upl16(f);
		d = f.uph16(d);
	}

	__forceinline static void sw16rh(Vector4i& a, Vector4i& b, Vector4i& c, Vector4i& d)
	{
		Vector4i e = a;
		Vector4i f = c;

		a = e.upl16(b);
		c = b.uph16(e);
		b = f.upl16(d);
		d = d.uph16(f);
	}

	__forceinline static void sw32(Vector4i& a, Vector4i& b, Vector4i& c, Vector4i& d)
	{
		Vector4i e = a;
		Vector4i f = c;

		a = e.upl32(b);
		c = e.uph32(b);
		b = f.upl32(d);
		d = f.uph32(d);
	}

	__forceinline static void sw64(Vector4i& a, Vector4i& b, Vector4i& c, Vector4i& d)
	{
		Vector4i e = a;
		Vector4i f = c;

		a = e.upl64(b);
		c = e.uph64(b);
		b = f.upl64(d);
		d = f.uph64(d);
	}

	__forceinline static bool compare16(const void* dst, const void* src, int size)
	{
		ASSERT((size & 15) == 0);

		size >>= 4;

		Vector4i* s = (Vector4i*)src;
		Vector4i* d = (Vector4i*)dst;

		for(int i = 0; i < size; i++)
		{
			if(!d[i].eq(s[i])) 
			{
				return false;
			}
		}

		return true;
	}

	__forceinline static bool compare64(const void* dst, const void* src, int size)
	{
		ASSERT((size & 63) == 0);

		size >>= 6;

		Vector4i* s = (Vector4i*)src;
		Vector4i* d = (Vector4i*)dst;

		for(int i = 0; i < size; i += 4)
		{
			Vector4i v0 = (d[i * 4 + 0] == s[i * 4 + 0]);
			Vector4i v1 = (d[i * 4 + 1] == s[i * 4 + 1]);
			Vector4i v2 = (d[i * 4 + 2] == s[i * 4 + 2]);
			Vector4i v3 = (d[i * 4 + 3] == s[i * 4 + 3]);

			v0 = v0 & v1;
			v2 = v2 & v3;

			if(!(v0 & v2).alltrue())
			{
				return false;
			}
		}

		return true;
	}

	__forceinline static bool update(const void* dst, const void* src, int size)
	{
		ASSERT((size & 15) == 0);

		size >>= 4;

		Vector4i* s = (Vector4i*)src;
		Vector4i* d = (Vector4i*)dst;

		Vector4i v = Vector4i::xffffffff();

		for(int i = 0; i < size; i++)
		{
			v &= d[i] == s[i];

			d[i] = s[i];
		}

		return v.alltrue();
	}

	void operator += (const Vector4i& v) 
	{
		m = _mm_add_epi32(m, v);
	}

	void operator -= (const Vector4i& v) 
	{
		m = _mm_sub_epi32(m, v);
	}

	void operator += (int i) 
	{
		*this += Vector4i(i);
	}

	void operator -= (int i) 
	{
		*this -= Vector4i(i);
	}

	void operator <<= (const int i) 
	{
		m = _mm_slli_epi32(m, i);
	}

	void operator >>= (const int i) 
	{
		m = _mm_srli_epi32(m, i);
	}

	void operator &= (const Vector4i& v)
	{
		m = _mm_and_si128(m, v);
	}

	void operator |= (const Vector4i& v) 
	{
		m = _mm_or_si128(m, v);
	}

	void operator ^= (const Vector4i& v) 
	{
		m = _mm_xor_si128(m, v);
	}

	friend Vector4i operator + (const Vector4i& v1, const Vector4i& v2)
	{
		return Vector4i(_mm_add_epi32(v1, v2));
	}

	friend Vector4i operator - (const Vector4i& v1, const Vector4i& v2) 
	{
		return Vector4i(_mm_sub_epi32(v1, v2));
	}

	friend Vector4i operator + (const Vector4i& v, int i)
	{
		return v + Vector4i(i);
	}
	
	friend Vector4i operator - (const Vector4i& v, int i)
	{
		return v - Vector4i(i);
	}
	
	friend Vector4i operator << (const Vector4i& v, const int i) 
	{
		return Vector4i(_mm_slli_epi32(v, i));
	}
	
	friend Vector4i operator >> (const Vector4i& v, const int i) 
	{
		return Vector4i(_mm_srli_epi32(v, i));
	}
	
	friend Vector4i operator & (const Vector4i& v1, const Vector4i& v2)
	{
		return Vector4i(_mm_and_si128(v1, v2));
	}
	
	friend Vector4i operator | (const Vector4i& v1, const Vector4i& v2)
	{
		return Vector4i(_mm_or_si128(v1, v2));
	}
	
	friend Vector4i operator ^ (const Vector4i& v1, const Vector4i& v2) 
	{
		return Vector4i(_mm_xor_si128(v1, v2));
	}
	
	friend Vector4i operator & (const Vector4i& v, int i) 
	{
		return v & Vector4i(i);
	}
	
	friend Vector4i operator | (const Vector4i& v, int i) 
	{
		return v | Vector4i(i);
	}
	
	friend Vector4i operator ^ (const Vector4i& v, int i) 
	{
		return v ^ Vector4i(i);
	}
	
	friend Vector4i operator ~ (const Vector4i& v) 
	{
		return v ^ (v == v);
	}

	friend Vector4i operator == (const Vector4i& v1, const Vector4i& v2) 
	{
		return Vector4i(_mm_cmpeq_epi32(v1, v2));
	}
	
	friend Vector4i operator != (const Vector4i& v1, const Vector4i& v2) 
	{
		return ~(v1 == v2);
	}
	
	friend Vector4i operator > (const Vector4i& v1, const Vector4i& v2) 
	{
		return Vector4i(_mm_cmpgt_epi32(v1, v2));
	}
	
	friend Vector4i operator < (const Vector4i& v1, const Vector4i& v2) 
	{
		return Vector4i(_mm_cmplt_epi32(v1, v2));
	}
	
	friend Vector4i operator >= (const Vector4i& v1, const Vector4i& v2) 
	{
		return (v1 > v2) | (v1 == v2);
	}
	
	friend Vector4i operator <= (const Vector4i& v1, const Vector4i& v2) 
	{
		return (v1 < v2) | (v1 == v2);
	}

	template<int i> Vector4i shuffle() const
	{
		return Vector4i(_mm_shuffle_epi32(m, _MM_SHUFFLE(i, i, i, i)));
	}

	#define VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
		Vector4i xs##ys##zs##ws() const {return Vector4i(_mm_shuffle_epi32(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		Vector4i xs##ys##zs##ws##l() const {return Vector4i(_mm_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		Vector4i xs##ys##zs##ws##h() const {return Vector4i(_mm_shufflehi_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		Vector4i xs##ys##zs##ws##lh() const {return Vector4i(_mm_shufflehi_epi16(_mm_shufflelo_epi16(m, _MM_SHUFFLE(wn, zn, yn, xn)), _MM_SHUFFLE(wn, zn, yn, xn)));} \

	#define VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
		VECTOR4i_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

	#define VECTOR4i_SHUFFLE_2(xs, xn, ys, yn) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
		VECTOR4i_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

	#define VECTOR4i_SHUFFLE_1(xs, xn) \
		Vector4i xs##4##() const {return Vector4i(_mm_shuffle_epi32(m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		Vector4i xs##4##l() const {return Vector4i(_mm_shufflelo_epi16(m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		Vector4i xs##4##h() const {return Vector4i(_mm_shufflehi_epi16(m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		VECTOR4i_SHUFFLE_2(xs, xn, x, 0) \
		VECTOR4i_SHUFFLE_2(xs, xn, y, 1) \
		VECTOR4i_SHUFFLE_2(xs, xn, z, 2) \
		VECTOR4i_SHUFFLE_2(xs, xn, w, 3) \

	VECTOR4i_SHUFFLE_1(x, 0)
	VECTOR4i_SHUFFLE_1(y, 1)
	VECTOR4i_SHUFFLE_1(z, 2)
	VECTOR4i_SHUFFLE_1(w, 3)

	static Vector4i zero() {return Vector4i(_mm_setzero_si128());}

	static Vector4i xffffffff() {return zero() == zero();}

	static Vector4i x00000001() {return xffffffff().srl32(31);}
	static Vector4i x00000003() {return xffffffff().srl32(30);}
	static Vector4i x00000007() {return xffffffff().srl32(29);}
	static Vector4i x0000000f() {return xffffffff().srl32(28);}
	static Vector4i x0000001f() {return xffffffff().srl32(27);}
	static Vector4i x0000003f() {return xffffffff().srl32(26);}
	static Vector4i x0000007f() {return xffffffff().srl32(25);}
	static Vector4i x000000ff() {return xffffffff().srl32(24);}
	static Vector4i x000001ff() {return xffffffff().srl32(23);}
	static Vector4i x000003ff() {return xffffffff().srl32(22);}
	static Vector4i x000007ff() {return xffffffff().srl32(21);}
	static Vector4i x00000fff() {return xffffffff().srl32(20);}
	static Vector4i x00001fff() {return xffffffff().srl32(19);}
	static Vector4i x00003fff() {return xffffffff().srl32(18);}
	static Vector4i x00007fff() {return xffffffff().srl32(17);}
	static Vector4i x0000ffff() {return xffffffff().srl32(16);}
	static Vector4i x0001ffff() {return xffffffff().srl32(15);}
	static Vector4i x0003ffff() {return xffffffff().srl32(14);}
	static Vector4i x0007ffff() {return xffffffff().srl32(13);}
	static Vector4i x000fffff() {return xffffffff().srl32(12);}
	static Vector4i x001fffff() {return xffffffff().srl32(11);}
	static Vector4i x003fffff() {return xffffffff().srl32(10);}
	static Vector4i x007fffff() {return xffffffff().srl32( 9);}
	static Vector4i x00ffffff() {return xffffffff().srl32( 8);}
	static Vector4i x01ffffff() {return xffffffff().srl32( 7);}
	static Vector4i x03ffffff() {return xffffffff().srl32( 6);}
	static Vector4i x07ffffff() {return xffffffff().srl32( 5);}
	static Vector4i x0fffffff() {return xffffffff().srl32( 4);}
	static Vector4i x1fffffff() {return xffffffff().srl32( 3);}
	static Vector4i x3fffffff() {return xffffffff().srl32( 2);}
	static Vector4i x7fffffff() {return xffffffff().srl32( 1);}

	static Vector4i x80000000() {return xffffffff().sll32(31);}
	static Vector4i xc0000000() {return xffffffff().sll32(30);}
	static Vector4i xe0000000() {return xffffffff().sll32(29);}
	static Vector4i xf0000000() {return xffffffff().sll32(28);}
	static Vector4i xf8000000() {return xffffffff().sll32(27);}
	static Vector4i xfc000000() {return xffffffff().sll32(26);}
	static Vector4i xfe000000() {return xffffffff().sll32(25);}
	static Vector4i xff000000() {return xffffffff().sll32(24);}
	static Vector4i xff800000() {return xffffffff().sll32(23);}
	static Vector4i xffc00000() {return xffffffff().sll32(22);}
	static Vector4i xffe00000() {return xffffffff().sll32(21);}
	static Vector4i xfff00000() {return xffffffff().sll32(20);}
	static Vector4i xfff80000() {return xffffffff().sll32(19);}
	static Vector4i xfffc0000() {return xffffffff().sll32(18);}
	static Vector4i xfffe0000() {return xffffffff().sll32(17);}
	static Vector4i xffff0000() {return xffffffff().sll32(16);}
	static Vector4i xffff8000() {return xffffffff().sll32(15);}
	static Vector4i xffffc000() {return xffffffff().sll32(14);}
	static Vector4i xffffe000() {return xffffffff().sll32(13);}
	static Vector4i xfffff000() {return xffffffff().sll32(12);}
	static Vector4i xfffff800() {return xffffffff().sll32(11);}
	static Vector4i xfffffc00() {return xffffffff().sll32(10);}
	static Vector4i xfffffe00() {return xffffffff().sll32( 9);}
	static Vector4i xffffff00() {return xffffffff().sll32( 8);}
	static Vector4i xffffff80() {return xffffffff().sll32( 7);}
	static Vector4i xffffffc0() {return xffffffff().sll32( 6);}
	static Vector4i xffffffe0() {return xffffffff().sll32( 5);}
	static Vector4i xfffffff0() {return xffffffff().sll32( 4);}
	static Vector4i xfffffff8() {return xffffffff().sll32( 3);}
	static Vector4i xfffffffc() {return xffffffff().sll32( 2);}
	static Vector4i xfffffffe() {return xffffffff().sll32( 1);}

	static Vector4i x0001() {return xffffffff().srl16(15);}
	static Vector4i x0003() {return xffffffff().srl16(14);}
	static Vector4i x0007() {return xffffffff().srl16(13);}
	static Vector4i x000f() {return xffffffff().srl16(12);}
	static Vector4i x001f() {return xffffffff().srl16(11);}
	static Vector4i x003f() {return xffffffff().srl16(10);}
	static Vector4i x007f() {return xffffffff().srl16( 9);}
	static Vector4i x00ff() {return xffffffff().srl16( 8);}
	static Vector4i x01ff() {return xffffffff().srl16( 7);}
	static Vector4i x03ff() {return xffffffff().srl16( 6);}
	static Vector4i x07ff() {return xffffffff().srl16( 5);}
	static Vector4i x0fff() {return xffffffff().srl16( 4);}
	static Vector4i x1fff() {return xffffffff().srl16( 3);}
	static Vector4i x3fff() {return xffffffff().srl16( 2);}
	static Vector4i x7fff() {return xffffffff().srl16( 1);}

	static Vector4i x8000() {return xffffffff().sll16(15);}
	static Vector4i xc000() {return xffffffff().sll16(14);}
	static Vector4i xe000() {return xffffffff().sll16(13);}
	static Vector4i xf000() {return xffffffff().sll16(12);}
	static Vector4i xf800() {return xffffffff().sll16(11);}
	static Vector4i xfc00() {return xffffffff().sll16(10);}
	static Vector4i xfe00() {return xffffffff().sll16( 9);}
	static Vector4i xff00() {return xffffffff().sll16( 8);}
	static Vector4i xff80() {return xffffffff().sll16( 7);}
	static Vector4i xffc0() {return xffffffff().sll16( 6);}
	static Vector4i xffe0() {return xffffffff().sll16( 5);}
	static Vector4i xfff0() {return xffffffff().sll16( 4);}
	static Vector4i xfff8() {return xffffffff().sll16( 3);}
	static Vector4i xfffc() {return xffffffff().sll16( 2);}
	static Vector4i xfffe() {return xffffffff().sll16( 1);}

	static Vector4i xffffffff(const Vector4i& v) {return v == v;}

	static Vector4i x00000001(const Vector4i& v) {return xffffffff(v).srl32(31);}
	static Vector4i x00000003(const Vector4i& v) {return xffffffff(v).srl32(30);}
	static Vector4i x00000007(const Vector4i& v) {return xffffffff(v).srl32(29);}
	static Vector4i x0000000f(const Vector4i& v) {return xffffffff(v).srl32(28);}
	static Vector4i x0000001f(const Vector4i& v) {return xffffffff(v).srl32(27);}
	static Vector4i x0000003f(const Vector4i& v) {return xffffffff(v).srl32(26);}
	static Vector4i x0000007f(const Vector4i& v) {return xffffffff(v).srl32(25);}
	static Vector4i x000000ff(const Vector4i& v) {return xffffffff(v).srl32(24);}
	static Vector4i x000001ff(const Vector4i& v) {return xffffffff(v).srl32(23);}
	static Vector4i x000003ff(const Vector4i& v) {return xffffffff(v).srl32(22);}
	static Vector4i x000007ff(const Vector4i& v) {return xffffffff(v).srl32(21);}
	static Vector4i x00000fff(const Vector4i& v) {return xffffffff(v).srl32(20);}
	static Vector4i x00001fff(const Vector4i& v) {return xffffffff(v).srl32(19);}
	static Vector4i x00003fff(const Vector4i& v) {return xffffffff(v).srl32(18);}
	static Vector4i x00007fff(const Vector4i& v) {return xffffffff(v).srl32(17);}
	static Vector4i x0000ffff(const Vector4i& v) {return xffffffff(v).srl32(16);}
	static Vector4i x0001ffff(const Vector4i& v) {return xffffffff(v).srl32(15);}
	static Vector4i x0003ffff(const Vector4i& v) {return xffffffff(v).srl32(14);}
	static Vector4i x0007ffff(const Vector4i& v) {return xffffffff(v).srl32(13);}
	static Vector4i x000fffff(const Vector4i& v) {return xffffffff(v).srl32(12);}
	static Vector4i x001fffff(const Vector4i& v) {return xffffffff(v).srl32(11);}
	static Vector4i x003fffff(const Vector4i& v) {return xffffffff(v).srl32(10);}
	static Vector4i x007fffff(const Vector4i& v) {return xffffffff(v).srl32( 9);}
	static Vector4i x00ffffff(const Vector4i& v) {return xffffffff(v).srl32( 8);}
	static Vector4i x01ffffff(const Vector4i& v) {return xffffffff(v).srl32( 7);}
	static Vector4i x03ffffff(const Vector4i& v) {return xffffffff(v).srl32( 6);}
	static Vector4i x07ffffff(const Vector4i& v) {return xffffffff(v).srl32( 5);}
	static Vector4i x0fffffff(const Vector4i& v) {return xffffffff(v).srl32( 4);}
	static Vector4i x1fffffff(const Vector4i& v) {return xffffffff(v).srl32( 3);}
	static Vector4i x3fffffff(const Vector4i& v) {return xffffffff(v).srl32( 2);}
	static Vector4i x7fffffff(const Vector4i& v) {return xffffffff(v).srl32( 1);}

	static Vector4i x80000000(const Vector4i& v) {return xffffffff(v).sll32(31);}
	static Vector4i xc0000000(const Vector4i& v) {return xffffffff(v).sll32(30);}
	static Vector4i xe0000000(const Vector4i& v) {return xffffffff(v).sll32(29);}
	static Vector4i xf0000000(const Vector4i& v) {return xffffffff(v).sll32(28);}
	static Vector4i xf8000000(const Vector4i& v) {return xffffffff(v).sll32(27);}
	static Vector4i xfc000000(const Vector4i& v) {return xffffffff(v).sll32(26);}
	static Vector4i xfe000000(const Vector4i& v) {return xffffffff(v).sll32(25);}
	static Vector4i xff000000(const Vector4i& v) {return xffffffff(v).sll32(24);}
	static Vector4i xff800000(const Vector4i& v) {return xffffffff(v).sll32(23);}
	static Vector4i xffc00000(const Vector4i& v) {return xffffffff(v).sll32(22);}
	static Vector4i xffe00000(const Vector4i& v) {return xffffffff(v).sll32(21);}
	static Vector4i xfff00000(const Vector4i& v) {return xffffffff(v).sll32(20);}
	static Vector4i xfff80000(const Vector4i& v) {return xffffffff(v).sll32(19);}
	static Vector4i xfffc0000(const Vector4i& v) {return xffffffff(v).sll32(18);}
	static Vector4i xfffe0000(const Vector4i& v) {return xffffffff(v).sll32(17);}
	static Vector4i xffff0000(const Vector4i& v) {return xffffffff(v).sll32(16);}
	static Vector4i xffff8000(const Vector4i& v) {return xffffffff(v).sll32(15);}
	static Vector4i xffffc000(const Vector4i& v) {return xffffffff(v).sll32(14);}
	static Vector4i xffffe000(const Vector4i& v) {return xffffffff(v).sll32(13);}
	static Vector4i xfffff000(const Vector4i& v) {return xffffffff(v).sll32(12);}
	static Vector4i xfffff800(const Vector4i& v) {return xffffffff(v).sll32(11);}
	static Vector4i xfffffc00(const Vector4i& v) {return xffffffff(v).sll32(10);}
	static Vector4i xfffffe00(const Vector4i& v) {return xffffffff(v).sll32( 9);}
	static Vector4i xffffff00(const Vector4i& v) {return xffffffff(v).sll32( 8);}
	static Vector4i xffffff80(const Vector4i& v) {return xffffffff(v).sll32( 7);}
	static Vector4i xffffffc0(const Vector4i& v) {return xffffffff(v).sll32( 6);}
	static Vector4i xffffffe0(const Vector4i& v) {return xffffffff(v).sll32( 5);}
	static Vector4i xfffffff0(const Vector4i& v) {return xffffffff(v).sll32( 4);}
	static Vector4i xfffffff8(const Vector4i& v) {return xffffffff(v).sll32( 3);}
	static Vector4i xfffffffc(const Vector4i& v) {return xffffffff(v).sll32( 2);}
	static Vector4i xfffffffe(const Vector4i& v) {return xffffffff(v).sll32( 1);}

	static Vector4i x0001(const Vector4i& v) {return xffffffff(v).srl16(15);}
	static Vector4i x0003(const Vector4i& v) {return xffffffff(v).srl16(14);}
	static Vector4i x0007(const Vector4i& v) {return xffffffff(v).srl16(13);}
	static Vector4i x000f(const Vector4i& v) {return xffffffff(v).srl16(12);}
	static Vector4i x001f(const Vector4i& v) {return xffffffff(v).srl16(11);}
	static Vector4i x003f(const Vector4i& v) {return xffffffff(v).srl16(10);}
	static Vector4i x007f(const Vector4i& v) {return xffffffff(v).srl16( 9);}
	static Vector4i x00ff(const Vector4i& v) {return xffffffff(v).srl16( 8);}
	static Vector4i x01ff(const Vector4i& v) {return xffffffff(v).srl16( 7);}
	static Vector4i x03ff(const Vector4i& v) {return xffffffff(v).srl16( 6);}
	static Vector4i x07ff(const Vector4i& v) {return xffffffff(v).srl16( 5);}
	static Vector4i x0fff(const Vector4i& v) {return xffffffff(v).srl16( 4);}
	static Vector4i x1fff(const Vector4i& v) {return xffffffff(v).srl16( 3);}
	static Vector4i x3fff(const Vector4i& v) {return xffffffff(v).srl16( 2);}
	static Vector4i x7fff(const Vector4i& v) {return xffffffff(v).srl16( 1);}

	static Vector4i x8000(const Vector4i& v) {return xffffffff(v).sll16(15);}
	static Vector4i xc000(const Vector4i& v) {return xffffffff(v).sll16(14);}
	static Vector4i xe000(const Vector4i& v) {return xffffffff(v).sll16(13);}
	static Vector4i xf000(const Vector4i& v) {return xffffffff(v).sll16(12);}
	static Vector4i xf800(const Vector4i& v) {return xffffffff(v).sll16(11);}
	static Vector4i xfc00(const Vector4i& v) {return xffffffff(v).sll16(10);}
	static Vector4i xfe00(const Vector4i& v) {return xffffffff(v).sll16( 9);}
	static Vector4i xff00(const Vector4i& v) {return xffffffff(v).sll16( 8);}
	static Vector4i xff80(const Vector4i& v) {return xffffffff(v).sll16( 7);}
	static Vector4i xffc0(const Vector4i& v) {return xffffffff(v).sll16( 6);}
	static Vector4i xffe0(const Vector4i& v) {return xffffffff(v).sll16( 5);}
	static Vector4i xfff0(const Vector4i& v) {return xffffffff(v).sll16( 4);}
	static Vector4i xfff8(const Vector4i& v) {return xffffffff(v).sll16( 3);}
	static Vector4i xfffc(const Vector4i& v) {return xffffffff(v).sll16( 2);}
	static Vector4i xfffe(const Vector4i& v) {return xffffffff(v).sll16( 1);}
};

__declspec(align(16)) class Vector4 : public AlignedClass<16>
{
public:
	union 
	{
		struct {float x, y, z, w;}; 
		struct {float r, g, b, a;}; 
		struct {float left, top, right, bottom;}; 
		struct {Vector2 tl, br;};
		float v[4];
		float f32[4];
		char i8[16];
		short i16[8];
		int i32[4];
		__int64 i64[2];		
		unsigned char u8[16];
		unsigned short u16[8];
		unsigned int u32[4];
		unsigned __int64 u64[2];
		__m128 m;
	};

	static const Vector4 m_ps0123;
	static const Vector4 m_ps4567;

	static const Vector4 m_x3f800000;
	static const Vector4 m_x4b000000;

	Vector4()
	{
	}

	Vector4(float x, float y, float z, float w)
	{
		m = _mm_set_ps(w, z, y, x);
	}

	Vector4(float x, float y)
	{
		m = _mm_unpacklo_ps(_mm_load_ss(&x), _mm_load_ss(&y));
	}

	Vector4(int x, int y, int z, int w)
	{
		Vector4i v(x, y, z, w);

		m = _mm_cvtepi32_ps(v.m);
	}

	Vector4(int x, int y)
	{
		m = _mm_cvtepi32_ps(_mm_unpacklo_epi32(_mm_cvtsi32_si128(x), _mm_cvtsi32_si128(y)));
	}

	Vector4(const Vector4& v) 
	{
		m = v.m;
	}

	explicit Vector4(const Vector2& v)
	{
		m = _mm_castsi128_ps(_mm_loadl_epi64((__m128i*)&v));
	}

	Vector4(const Vector2& v0, const Vector2& v1)
	{
		*this = Vector4(v0).xyxy(Vector4(v1));
	}

	explicit Vector4(const Vector2i& v)
	{
		m = _mm_cvtepi32_ps(_mm_loadl_epi64((__m128i*)&v));
	}

	explicit Vector4(float f)
	{
		m = _mm_set1_ps(f);
	}

	explicit Vector4(__m128 m)
	{
		this->m = m;
	}

	explicit Vector4(DWORD u32)
	{
		*this = Vector4(Vector4i::load((int)u32).u8to32());
	}

	explicit Vector4(const Vector4i& v);

	Vector4& operator = (const Vector4& v)
	{
		m = v.m;

		return *this;
	}

	Vector4& operator = (float f)
	{
		m = _mm_set1_ps(f);

		return *this;
	}

	Vector4& operator = (__m128 m)
	{
		this->m = m;

		return *this;
	}

	Vector4& operator = (DWORD u32)
	{
		*this = Vector4(Vector4i::load((int)u32).u8to32());

		return *this;
	}

	operator __m128() const 
	{
		return m;
	}

	unsigned int rgba32() const
	{
		return Vector4i(*this).rgba32();
	}

	static Vector4 cast(const Vector4i& v);

	Vector4 abs() const 
	{
		return *this & cast(Vector4i::x7fffffff());
	}

	Vector4 neg() const 
	{
		return *this ^ cast(Vector4i::x80000000());
	}

	Vector4 rcp() const 
	{
		return Vector4(_mm_rcp_ps(m));
	}

	Vector4 rcpnr() const 
	{
		Vector4 v = rcp();

		return (v + v) - (v * v) * *this;
	}

	Vector4 sqrt() const
	{
		// return Vector4(_mm_sqrt_ps(m));
		return Vector4(_mm_mul_ps(m, _mm_rsqrt_ps(m)));
	}

	Vector4 rsqrt() const
	{
		return Vector4(_mm_rsqrt_ps(m));
	}

	enum RoundMode {NearestInt = 8, NegInf = 9, PosInf = 10};

	template<int mode> Vector4 round() const 
	{
		#if _M_SSE >= 0x401

		return Vector4(_mm_round_ps(m, mode));

		#else

		Vector4 a = *this;

		Vector4 b = (a & cast(Vector4i::x80000000())) | m_x4b000000;

		b = a + b - b;

		if((mode & 7) == (NegInf & 7))
		{
			return b - ((a < b) & m_x3f800000);
		}

		if((mode & 7) == (PosInf & 7))
		{
			return b + ((a > b) & m_x3f800000);
		}

		ASSERT((mode & 7) == (NearestInt & 7)); // other modes aren't implemented

		return b;

		#endif
	}
	
	Vector4 fit(int arx, int ary) const;

	Vector4 floor() const 
	{
		return round<NegInf>();
	}

	Vector4 ceil() const 
	{
		return round<PosInf>();
	}

	Vector4 mod2x(const Vector4& f, const int scale = 256) const 
	{
		return *this * (f * (2.0f / scale));
	}

	Vector4 mod2x(float f, const int scale = 256) const 
	{
		return mod2x(Vector4(f), scale);
	}

	Vector4 madd(const Vector4& a, const Vector4& b) const
	{
		return *this * a + b; // TODO: _mm_fmadd_ps
	}

	Vector4 msub(const Vector4& a, const Vector4& b) const
	{
		return *this * a + b; // TODO: _mm_fmsub_ps
	}

	Vector4 nmadd(const Vector4& a, const Vector4& b) const
	{
		return b - *this * a; // TODO: _mm_fnmadd_ps
	}

	Vector4 nmsub(const Vector4& a, const Vector4& b) const
	{
		return -b - *this * a; // TODO: _mm_fmnsub_ps
	}

	Vector4 lerp(const Vector4& v, const Vector4& f) const 
	{
		return *this + (v - *this) * f;
	}

	Vector4 lerp(const Vector4& v, float f) const 
	{
		return lerp(v, Vector4(f));
	}

	Vector4 hadd() const
	{
		#if _M_SSE >= 0x300
		return Vector4(_mm_hadd_ps(m, m));
		#else
		return xzxz() + ywyw();
		#endif
	}

	Vector4 hadd(const Vector4& v) const
	{
		#if _M_SSE >= 0x300
		return Vector4(_mm_hadd_ps(m, v.m));
		#else
		return xzxz(v) + ywyw(v);
		#endif
	}

	#if _M_SSE >= 0x401
	template<int i> Vector4 dp(const Vector4& v) const 
	{
		return Vector4(_mm_dp_ps(m, v.m, i));
	}
	#endif

	Vector4 sat(const Vector4& a, const Vector4& b) const 
	{
		return Vector4(_mm_min_ps(_mm_max_ps(m, a), b));
	}

	Vector4 sat(const Vector4& a) const 
	{
		return Vector4(_mm_min_ps(_mm_max_ps(m, a.xyxy()), a.zwzw()));
	}

	Vector4 sat(const float scale = 255) const 
	{
		return sat(zero(), Vector4(scale));
	}

	Vector4 clamp(const float scale = 255) const 
	{
		return minf(Vector4(scale));
	}

	Vector4 minf(const Vector4& a) const
	{
		return Vector4(_mm_min_ps(m, a));
	}

	Vector4 maxf(const Vector4& a) const
	{
		return Vector4(_mm_max_ps(m, a));
	}

	Vector4 blend8(const Vector4& a, const Vector4& mask)  const
	{
		#if _M_SSE >= 0x401

		return Vector4(_mm_blendv_ps(m, a, mask));

		#else

		return Vector4(_mm_or_ps(_mm_andnot_ps(mask, m), _mm_and_ps(mask, a)));

		#endif
	}

	Vector4 upl(const Vector4& a) const
	{
		return Vector4(_mm_unpacklo_ps(m, a));
	}

	Vector4 uph(const Vector4& a) const
	{
		return Vector4(_mm_unpackhi_ps(m, a));
	}

	Vector4 l2h(const Vector4& a) const
	{
		return Vector4(_mm_movelh_ps(m, a));
	}	

	Vector4 h2l(const Vector4& a) const
	{
		return Vector4(_mm_movehl_ps(m, a));
	}	

	Vector4 andnot(const Vector4& v) const
	{
		return Vector4(_mm_andnot_ps(v.m, m));
	}

	int mask() const
	{
		return _mm_movemask_ps(m);
	}

	bool alltrue() const
	{
		return _mm_movemask_ps(m) == 0xf;
	}

	bool anytrue() const
	{
		return !allfalse();
	}

	bool allfalse() const
	{
		#if _M_SSE >= 0x401
		__m128i a = _mm_castps_si128(m);
		return _mm_testz_si128(a, a) != 0;
		#else
		return _mm_movemask_ps(m) == 0;
		#endif
	}

	// TODO: insert

	template<int i> int extract() const
	{
		#if _M_SSE >= 0x401
		return _mm_extract_ps(m, i);
		#else
		return i32[i];
		#endif
	}

	static Vector4 zero() 
	{
		return Vector4(_mm_setzero_ps());
	}

	static Vector4 xffffffff() 
	{
		return zero() == zero();
	}

	static Vector4 ps0123()
	{
		return Vector4(m_ps0123);
	}

	static Vector4 ps4567()
	{
		return Vector4(m_ps4567);
	}

	static Vector4 loadl(const void* p)
	{
		return Vector4(_mm_castpd_ps(_mm_load_sd((double*)p)));
	}

	static Vector4 load(float f)
	{
		return Vector4(_mm_load_ss(&f));
	}

	template<bool aligned> static Vector4 load(const void* p)
	{
		return Vector4(aligned ? _mm_load_ps((float*)p) : _mm_loadu_ps((float*)p));
	}

	static void storel(void* p, const Vector4& v)
	{
		_mm_store_sd((double*)p, _mm_castps_pd(v.m));
	}

	template<bool aligned> static void store(void* p, const Vector4& v)
	{
		if(aligned) _mm_store_ps((float*)p, v.m);
		else _mm_storeu_ps((float*)p, v.m);
	}

	__forceinline static void expand(const Vector4i& v, Vector4& a, Vector4& b, Vector4& c, Vector4& d)
	{
		Vector4i mask = Vector4i::x000000ff();

		a = Vector4(v & mask);
		b = Vector4((v >> 8) & mask);
		c = Vector4((v >> 16) & mask);
		d = Vector4((v >> 24));
	}

	__forceinline static void transpose(Vector4& a, Vector4& b, Vector4& c, Vector4& d)
	{
		Vector4 v0 = a.xyxy(b);
		Vector4 v1 = c.xyxy(d);

		Vector4 e = v0.xzxz(v1);
		Vector4 f = v0.ywyw(v1);

		Vector4 v2 = a.zwzw(b);
		Vector4 v3 = c.zwzw(d);

		Vector4 g = v2.xzxz(v3);
		Vector4 h = v2.ywyw(v3);

		a = e;
		b = f;
		c = g;
		d = h;
/*
		Vector4 v0 = a.xyxy(b);
		Vector4 v1 = c.xyxy(d);
		Vector4 v2 = a.zwzw(b);
		Vector4 v3 = c.zwzw(d);

		a = v0.xzxz(v1);
		b = v0.ywyw(v1);
		c = v2.xzxz(v3);
		d = v2.ywyw(v3);
*/
/*
		Vector4 v0 = a.upl(b);
		Vector4 v1 = a.uph(b);
		Vector4 v2 = c.upl(d);
		Vector4 v3 = c.uph(d);

		a = v0.l2h(v2);
		b = v2.h2l(v0);
		c = v1.l2h(v3);
		d = v3.h2l(v1);
*/	}

	Vector4 operator - () const
	{
		return neg();
	}

	void operator += (const Vector4& v) 
	{
		m = _mm_add_ps(m, v);
	}

	void operator -= (const Vector4& v) 
	{
		m = _mm_sub_ps(m, v);
	}

	void operator *= (const Vector4& v) 
	{
		m = _mm_mul_ps(m, v);
	}

	void operator /= (const Vector4& v) 
	{
		m = _mm_div_ps(m, v);
	}

	void operator += (float f) 
	{
		*this += Vector4(f);
	}

	void operator -= (float f) 
	{
		*this -= Vector4(f);
	}

	void operator *= (float f) 
	{
		*this *= Vector4(f);
	}

	void operator /= (float f) 
	{
		*this /= Vector4(f);
	}

	void operator &= (const Vector4& v)
	{
		m = _mm_and_ps(m, v);
	}

	void operator |= (const Vector4& v) 
	{
		m = _mm_or_ps(m, v);
	}

	void operator ^= (const Vector4& v) 
	{
		m = _mm_xor_ps(m, v);
	}

	friend Vector4 operator + (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_add_ps(v1, v2));
	}

	friend Vector4 operator - (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_sub_ps(v1, v2));
	}

	friend Vector4 operator * (const Vector4& v1, const Vector4& v2)
	{
		return Vector4(_mm_mul_ps(v1, v2));
	}

	friend Vector4 operator / (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_div_ps(v1, v2));
	}

	friend Vector4 operator + (const Vector4& v, float f) 
	{
		return v + Vector4(f);
	}

	friend Vector4 operator - (const Vector4& v, float f) 
	{
		return v - Vector4(f);
	}

	friend Vector4 operator * (const Vector4& v, float f) 
	{
		return v * Vector4(f);
	}

	friend Vector4 operator / (const Vector4& v, float f) 
	{
		return v / Vector4(f);
	}

	friend Vector4 operator & (const Vector4& v1, const Vector4& v2)
	{
		return Vector4(_mm_and_ps(v1, v2));
	}
	
	friend Vector4 operator | (const Vector4& v1, const Vector4& v2)
	{
		return Vector4(_mm_or_ps(v1, v2));
	}
	
	friend Vector4 operator ^ (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_xor_ps(v1, v2));
	}

	friend Vector4 operator == (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_cmpeq_ps(v1, v2));
	}

	friend Vector4 operator != (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_cmpneq_ps(v1, v2));
	}

	friend Vector4 operator > (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_cmpgt_ps(v1, v2));
	}

	friend Vector4 operator < (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_cmplt_ps(v1, v2));
	}

	friend Vector4 operator >= (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_cmpge_ps(v1, v2));
	}

	friend Vector4 operator <= (const Vector4& v1, const Vector4& v2) 
	{
		return Vector4(_mm_cmple_ps(v1, v2));
	}

	template<int i> Vector4 shuffle() const
	{
		return Vector4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(i, i, i, i)));
	}

	#define VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, ws, wn) \
		Vector4 xs##ys##zs##ws() const {return Vector4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(wn, zn, yn, xn)));} \
		Vector4 xs##ys##zs##ws(const Vector4& v) const {return Vector4(_mm_shuffle_ps(m, v.m, _MM_SHUFFLE(wn, zn, yn, xn)));} \

	#define VECTOR4_SHUFFLE_3(xs, xn, ys, yn, zs, zn) \
		VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, x, 0) \
		VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, y, 1) \
		VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, z, 2) \
		VECTOR4_SHUFFLE_4(xs, xn, ys, yn, zs, zn, w, 3) \

	#define VECTOR4_SHUFFLE_2(xs, xn, ys, yn) \
		VECTOR4_SHUFFLE_3(xs, xn, ys, yn, x, 0) \
		VECTOR4_SHUFFLE_3(xs, xn, ys, yn, y, 1) \
		VECTOR4_SHUFFLE_3(xs, xn, ys, yn, z, 2) \
		VECTOR4_SHUFFLE_3(xs, xn, ys, yn, w, 3) \

	#define VECTOR4_SHUFFLE_1(xs, xn) \
		Vector4 xs##4##() const {return Vector4(_mm_shuffle_ps(m, m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		Vector4 xs##4##(const Vector4& v) const {return Vector4(_mm_shuffle_ps(m, v.m, _MM_SHUFFLE(xn, xn, xn, xn)));} \
		VECTOR4_SHUFFLE_2(xs, xn, x, 0) \
		VECTOR4_SHUFFLE_2(xs, xn, y, 1) \
		VECTOR4_SHUFFLE_2(xs, xn, z, 2) \
		VECTOR4_SHUFFLE_2(xs, xn, w, 3) \

	VECTOR4_SHUFFLE_1(x, 0)
	VECTOR4_SHUFFLE_1(y, 1)
	VECTOR4_SHUFFLE_1(z, 2)
	VECTOR4_SHUFFLE_1(w, 3)
};

inline Vector4i::Vector4i(const Vector4& v)
{
	m = _mm_cvttps_epi32(v);
}

inline Vector4::Vector4(const Vector4i& v)
{
	m = _mm_cvtepi32_ps(v);
}

#pragma pack(pop)
