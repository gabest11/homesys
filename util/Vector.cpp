#include "StdAfx.h"
#include "Vector.h"

const Vector4 Vector4::m_ps0123(0.0f, 1.0f, 2.0f, 3.0f);
const Vector4 Vector4::m_ps4567(4.0f, 5.0f, 6.0f, 7.0f);
const Vector4 Vector4::m_x3f800000(_mm_castsi128_ps(_mm_set1_epi32(0x3f800000)));
const Vector4 Vector4::m_x4b000000(_mm_castsi128_ps(_mm_set1_epi32(0x4b000000)));

Vector4i Vector4i::cast(const Vector4& v)
{
	return Vector4i(_mm_castps_si128(v.m));
}

Vector4 Vector4::cast(const Vector4i& v)
{
	return Vector4(_mm_castsi128_ps(v.m));
}

Vector4i Vector4i::fit(const Vector2i& ar) const
{
	return fit(ar.x, ar.y);
}

Vector4i Vector4i::fit(int arx, int ary) const
{
	Vector4i r = *this;

	if(arx > 0 && ary > 0)
	{
		int w = width();
		int h = height();

		if(w * ary > h * arx)
		{
			w = h * arx / ary;
			r.left = (r.left + r.right - w) >> 1;
			if(r.left & 1) r.left++;
			r.right = r.left + w;
		}
		else
		{
			h = w * ary / arx;
			r.top = (r.top + r.bottom - h) >> 1;
			if(r.top & 1) r.top++;
			r.bottom = r.top + h;
		}

		r = r.rintersect(*this);
	}
	else
	{
		r = *this;
	}

	return r;
}

Vector4i Vector4i::fit(int preset) const
{
	Vector4i r;

	static const int ar[][2] = {{0, 0}, {4, 3}, {16, 9}};

	if(preset > 0 && preset < sizeof(ar) / sizeof(ar[0]))
	{
		r = fit(ar[preset][0], ar[preset][1]);
	}
	else
	{
		r = *this;
	}

	return r;
}

Vector4 Vector4::fit(int arx, int ary) const
{
	Vector4 r = *this;

	if(arx > 0 && ary > 0)
	{
		float w = right - left;
		float h = bottom - top;

		if(w * ary > h * arx)
		{
			w = h * arx / ary;
			r.left = (r.left + r.right - w) / 2;
			r.right = r.left + w;
		}
		else
		{
			h = w * ary / arx;
			r.top = (r.top + r.bottom - h) / 2;
			r.bottom = r.top + h;
		}

		r = r.sat(*this);
	}
	else
	{
		r = *this;
	}

	return r;
}
