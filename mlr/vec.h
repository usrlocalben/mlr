
#ifndef __VEC_H
#define __VEC_H

#include <iostream>
#include <array>
#include <algorithm>
#include <boost/format.hpp>

#include <emmintrin.h>

static const double PI = 3.14159265358979323846F;
static const float PI_F = 3.14159265358979f;


template<typename T>
T lerp_fast(const T a, const T b, const T f) {
	return a + (f*(b - a));
}

template<typename T>
T lerp(const T a, const T b, const T f) {
	return (1 - f)*a + f*b;
}

/*
template<typename T>
T clamp(const T a, const T x, const T y) {
	return a < x ? x : (a > y ? y : a);
}


template<typename T>
T saturate(const T& a) {
	return clamp<T>(a, 0, 1);
}
*/


template<typename T>
T smoothstep(const T a, const T b, const T t) {
	const T x = saturate<T>((t - a) / (b - a));
	return x*x * (3 - 2 * x);
}

template<typename T>
T smootherstep(const T a, const T b, const T t) {
	const T x = saturate<T>((t - a) / (b - a));
	return x*x*x * (x * (x * 6 - 15) + 10);
}


// constant needed for abs & operator-, from the fastcpp code
static const __m128 SIGNMASK = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));

#define realtofixed(x) ((x)*65536l)


/*
* https://github.com/LiraNuna/glsl-sse2/blob/master/source/mat4.h
* LiraNuna...
*/
#define _mm_shufd(xmm,mask) _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(xmm),mask))


__forceinline double fract(const double x) {
	double b;
	return modf(x, &b);
//	auto ux = static_cast<int>(x);
//	return x - ux;
}

__forceinline float fastsqrt(const float x) {
	union {
		int i;
		float x;
	} u;
	u.x = x;
	u.i = (1 << 29) + (u.i >> 1) - (1 << 22);
	return u.x;
}


__forceinline double fastpow(double a, double b) {
	union {
		double d;
		int x[2];
	} u = { a };
	u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
	u.x[0] = 0;
	return u.d;
}



struct irect {
	int y0, y1, x0, x1;
	irect::irect(const int y0, const int y1, const int x0, const int x1) :y0(y0), y1(y1), x0(x0), x1(x1) {}
	irect::irect(){}
};




__declspec(align(16)) struct vec2 {

	__forceinline  vec2() :x(0), y(0) {}
	__forceinline  vec2(float a) : x(a), y(a) {}
	__forceinline  vec2(float a, float b) : x(a), y(b) {}

	__forceinline vec2  operator+ (const vec2& b) const { return vec2(x + b.x, y + b.y); }
	__forceinline vec2  operator- (const vec2& b) const { return vec2(x - b.x, y - b.y); }
	__forceinline vec2  operator* (const vec2& b) const { return vec2(x*b.x, y*b.y); }
	__forceinline vec2  operator/ (const vec2& b) const { return vec2(x / b.x, y / b.y); }

	__forceinline vec2  operator+ (const float b) const { return vec2(x + b, y + b); }
	__forceinline vec2  operator- (const float b) const { return vec2(x - b, y - b); }
	__forceinline vec2  operator* (const float b) const { return vec2(x*b, y*b); }
	__forceinline vec2  operator/ (const float b) const { return vec2(x / b, y / b); }

	__forceinline vec2& operator+=(const vec2& b) { x += b.x; y += b.y; return *this; }
	__forceinline vec2& operator-=(const vec2& b) { x -= b.x; y -= b.y; return *this; }
	__forceinline vec2& operator*=(const vec2& b) { x *= b.x; y *= b.y; return *this; }
	__forceinline vec2& operator/=(const vec2& b) { x /= b.x; y /= b.y; return *this; }

	__forceinline vec2& operator+=(const float b) { x += b;   y += b;   return *this; }
	__forceinline vec2& operator-=(const float b) { x -= b;   y -= b;   return *this; }
	__forceinline vec2& operator*=(const float b) { x *= b;   y *= b;   return *this; }
	__forceinline vec2& operator/=(const float b) { x /= b;   y /= b;   return *this; }

	__forceinline friend vec2  operator- (const vec2& a) { return vec2(-a.x, -a.y); }

	__forceinline friend vec2  operator* (const float b, const vec2& a) { return vec2(a.x*b, a.y*b); }

	__forceinline friend float dot(const vec2& a, const vec2& b) { return a.x*b.x + a.y*b.y; }

	/*
	__forceinline float length(){
		return sqrt(dot(*this, *this));
	}
	*/

	__forceinline friend vec2 normalized(const vec2& a) {
		return (1.0f / sqrt(dot(a, a)))*a;
	}

	__forceinline friend vec2 lerp(const vec2& a, const vec2& b, const float t) {
		return vec2( lerp(a.x,b.x,t), lerp(a.y,b.y,t) );
	}
	__forceinline friend  vec2  abs(const vec2& a) {
		return vec2(fabs(a.x), fabs(a.y));
	}

	float x, y;

};//vec2



__declspec(align(16)) struct vec3 {

	__forceinline vec3(const float a, const float b, const float c) :v(_mm_set_ps(0, c, b, a)){}
	__forceinline vec3(const float a) : v(_mm_set_ps(0, a, a, a)){}
	__forceinline vec3() : v(_mm_setzero_ps()){}
	__forceinline vec3(__m128 m) : v(m){}

//	__forceinline vec4 xyz1() { return vec4(x, y, z, 1); }
//	__forceinline vec4 xyz0() { return vec4(x, y, z, 0); }

	__forceinline vec3  operator+ (const vec3& b) const { return _mm_add_ps(v, b.v); }
	__forceinline vec3  operator- (const vec3& b) const { return _mm_sub_ps(v, b.v); }
	__forceinline vec3  operator* (const vec3& b) const { return _mm_mul_ps(v, b.v); }
	__forceinline vec3  operator/ (const vec3& b) const { return _mm_div_ps(v, b.v); }

	__forceinline vec3  operator+ (const float b) const { return _mm_add_ps(v, _mm_set1_ps(b)); }
	__forceinline vec3  operator- (const float b) const { return _mm_sub_ps(v, _mm_set1_ps(b)); }
	__forceinline vec3  operator* (const float b) const { return _mm_mul_ps(v, _mm_set1_ps(b)); }
	__forceinline vec3  operator/ (const float b) const { return _mm_div_ps(v, _mm_set1_ps(b)); }

	__forceinline vec3& operator+=(const vec3& b) { v = _mm_add_ps(v, b.v); return *this; }
	__forceinline vec3& operator-=(const vec3& b) { v = _mm_sub_ps(v, b.v); return *this; }
	__forceinline vec3& operator*=(const vec3& b) { v = _mm_mul_ps(v, b.v); return *this; }
	__forceinline vec3& operator/=(const vec3& b) { v = _mm_div_ps(v, b.v); return *this; }

	__forceinline vec3& operator+=(const float b) { v = _mm_add_ps(v, _mm_set1_ps(b)); return *this; }
	__forceinline vec3& operator-=(const float b) { v = _mm_sub_ps(v, _mm_set1_ps(b)); return *this; }
	__forceinline vec3& operator*=(const float b) { v = _mm_mul_ps(v, _mm_set1_ps(b)); return *this; }
	__forceinline vec3& operator/=(const float b) { v = _mm_div_ps(v, _mm_set1_ps(b)); return *this; }

	__forceinline friend vec3 operator-(const vec3& a) { return vec3(_mm_xor_ps(a.v, SIGNMASK)); }
	__forceinline friend vec3 abs(const vec3& a) { return vec3(_mm_andnot_ps(SIGNMASK, a.v)); }

	__forceinline friend vec3 clamp(const vec3& a, float minval, float maxval)
	{
		return vec3(
			_mm_min_ps( _mm_max_ps(_mm_set1_ps(minval), a.v), _mm_set1_ps(maxval))
			);
	}
	/*
	__forceinline friend vec3 clamp255(const vec3& a)
	{
		return vec3(
			_mm_min_ps(	_mm_max_ps(_mm_setzero_ps(), a.v), _mm_set1_ps(255.0f))
		);
	}
	*/

	/*
	inline vec3 cross(const vec3& b) const
	{
	return _mm_sub_ps(
	_mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(3, 1, 0, 2))),
	_mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(3, 0, 2, 1)))
	);
	}
	*/
	//  inline float dot(const vec3& b) const { return _mm_cvtss_f32(_mm_dp_ps(v, b.v, 0x71)); } //XXX SSE4 only!

	__forceinline friend vec3 operator+(float a, const vec3& b) { return b + a; }
	__forceinline friend vec3 operator-(float a, const vec3& b) { return vec3(_mm_set1_ps(a)) - b; }
	__forceinline friend vec3 operator*(float a, const vec3& b) { return b * a; }
	__forceinline friend vec3 operator/(float a, const vec3& b) { return vec3(_mm_set1_ps(a)) / b; }
	/*
	friend vec3 vec3max(const vec3& a, const float b) { return vec3(a.x>b ? a.x : b, a.y>b ? a.y : b, a.z>b ? a.z : b); }
	friend vec3 vec3min(const vec3& a, const float b) { return vec3(a.x<b ? a.x : b, a.y<b ? a.y : b, a.z<b ? a.z : b); }
	*/

	//__forceinline friend float dot(const vec3 &a, const vec3 &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
	__forceinline friend float dot(const vec3 &a, const vec3 &b) {
		float r;
		const __m128 r1 = _mm_mul_ps(a.v, b.v);
		const __m128 r2 = _mm_hadd_ps(r1, r1);
		const __m128 r3 = _mm_hadd_ps(r2, r2);
		_mm_store_ss(&r, r3);
		return r;
	}

	__forceinline friend	float length(const vec3 &a) { return sqrt(dot(a, a)); }

	//__forceinline friend	vec3	normalized	(const vec3 &a)					{ return (1.0F/sqrt(dot(a,a)))*a; }

	__forceinline vec3 from_rgb(const int rgb) {
		return vec3(
			(float)((rgb >> 16) & 0xff) / 256.0f
			, (float)((rgb >> 8) & 0xff) / 256.0f
			, (float)((rgb >> 0) & 0xff) / 256.0f);
	}

	union {
		__m128 v;
		struct { float x, y, z; };
	};

	__forceinline friend vec3 lerp(const vec3 &a, const vec3 &b, const float pct)
	{
		return (1.0f - pct)*a + pct*b;
	}

};//vec3




__declspec(align(16)) struct ivec2 {

	__forceinline ivec2(const int a, const int b) : x(a), y(b){}
	__forceinline ivec2(const int a) : x(a), y(a) {}
	__forceinline ivec2() : x(0), y(0) {}

	__forceinline ivec2 operator+(const ivec2& b) const { return ivec2(x + b.x, y + b.y); }
	__forceinline ivec2 operator-(const ivec2& b) const { return ivec2(x - b.x, y - b.y); }
	__forceinline ivec2 operator*(const ivec2& b) const { return ivec2(x * b.x, y * b.y); }
	__forceinline ivec2 operator/(const ivec2& b) const { return ivec2(x / b.x, y / b.y); }

	__forceinline ivec2& operator+=(const ivec2& b) { x += b.x; y += b.y; return *this; }
	__forceinline ivec2& operator-=(const ivec2& b) { x -= b.x; y -= b.y; return *this; }

	__forceinline ivec2 operator+(int b) const { return ivec2(x + b, y + b); }
	__forceinline ivec2 operator-(int b) const { return ivec2(x - b, y - b); }

	int x, y;
};

__declspec(align(16)) struct ivec3 {

	__forceinline ivec3(const int a, const int b, const int c) : v(_mm_set_epi32(0, c, b, a)){}
	__forceinline ivec3(const int a) : v(_mm_set_epi32(0, a, a, a)){}
	__forceinline ivec3() : v(_mm_setzero_si128()){}
	__forceinline ivec3(__m128i m) : v(m){}

	__forceinline ivec3 operator+(const ivec3& b) const { return _mm_add_epi32(v, b.v); }
	__forceinline ivec3 operator-(const ivec3& b) const { return _mm_sub_epi32(v, b.v); }

	__forceinline ivec3& operator+=(const ivec3& b) { v = _mm_add_epi32(v, b.v); return *this; }
	__forceinline ivec3& operator-=(const ivec3& b) { v = _mm_sub_epi32(v, b.v); return *this; }

	__forceinline ivec3 operator+(int b) const { return _mm_add_epi32(v, _mm_set1_epi32(b)); }
	__forceinline ivec3 operator-(int b) const { return _mm_sub_epi32(v, _mm_set1_epi32(b)); }

	union {
		__m128i v;
		struct { int x, y, z; };
	};
};// ivec3





__declspec(align(16)) struct vec4 {

	__forceinline vec4(const float a, const float b, const float c, const float d) : v(_mm_set_ps(d, c, b, a)){}
	//__forceinline vec4( const float a, const float b, const float c )                : v(_mm_set_ps(0,c,b,a)){}
	__forceinline vec4(const float a) : v(_mm_set1_ps(a)){}
	//__forceinline vec4():v(_mm_setzero_ps()){}
	__forceinline vec4(){}
	__forceinline vec4(__m128 m) : v(m){}

	static __forceinline vec4 load(__m128 *a) {
		return vec4(_mm_load_ps(reinterpret_cast<float*>(a)));
	}

	__forceinline void store(__m128 *out) {
		_mm_store_ps(reinterpret_cast<float*>(out), this->v);
	}
	//static __forceinline vec4 load(vec4 *a) {
//		return _mm_load_ps(reinterpret_cast<float*>(&a->v));
//	}


	static __forceinline vec4  zero() { return vec4(_mm_setzero_ps()); }

	__forceinline vec2 xy() { return vec2(x, y); }

	__forceinline vec4  operator+ (const vec4& b) const { return _mm_add_ps(v, b.v); }
	__forceinline vec4  operator- (const vec4& b) const { return _mm_sub_ps(v, b.v); }
	__forceinline vec4  operator* (const vec4& b) const { return _mm_mul_ps(v, b.v); }
	__forceinline vec4  operator/ (const vec4& b) const { return _mm_div_ps(v, b.v); }

	__forceinline vec4& operator+=(const vec4& b)       { v = _mm_add_ps(v, b.v); return *this; }
	__forceinline vec4& operator-=(const vec4& b)       { v = _mm_sub_ps(v, b.v); return *this; }
	__forceinline vec4& operator*=(const vec4& b)       { v = _mm_mul_ps(v, b.v); return *this; }
	__forceinline vec4& operator/=(const vec4& b)       { v = _mm_div_ps(v, b.v); return *this; }

	__forceinline vec4  operator+ (const float b) const { return _mm_add_ps(v, _mm_set1_ps(b)); }
	__forceinline vec4  operator- (const float b) const { return _mm_sub_ps(v, _mm_set1_ps(b)); }
	__forceinline vec4  operator* (const float b) const { return _mm_mul_ps(v, _mm_set1_ps(b)); }
	__forceinline vec4  operator/ (const float b) const { return _mm_div_ps(v, _mm_set1_ps(b)); }

	__forceinline friend vec4 operator* (const float b, const vec4& a) { return _mm_mul_ps(a.v, _mm_set1_ps(b)); }

	__forceinline vec4& operator+=(const float b)      { v = _mm_add_ps(v, _mm_set1_ps(b)); return *this; }
	__forceinline vec4& operator-=(const float b)      { v = _mm_sub_ps(v, _mm_set1_ps(b)); return *this; }
	__forceinline vec4& operator*=(const float b)      { v = _mm_mul_ps(v, _mm_set1_ps(b)); return *this; }
	__forceinline vec4& operator/=(const float b)      { v = _mm_div_ps(v, _mm_set1_ps(b)); return *this; }

	__forceinline friend vec4 operator-(const vec4& a) { return vec4(_mm_xor_ps(a.v, SIGNMASK)); }
	__forceinline friend vec4 abs(const vec4& a) { return vec4(_mm_andnot_ps(SIGNMASK, a.v)); }

	__forceinline friend float dot(const vec4 &a, const vec4 &b) {
		float r;
		__m128 r1 = _mm_mul_ps(a.v, b.v);
		__m128 r2 = _mm_hadd_ps(r1, r1);
		__m128 r3 = _mm_hadd_ps(r2, r2);
		_mm_store_ss(&r, r3);
		return r;
	}

	__forceinline friend float length(const vec4 &a) {
		float r;
		__m128 r1 = _mm_mul_ps(a.v, a.v);
		__m128 r2 = _mm_hadd_ps(r1, r1);
		__m128 r3 = _mm_hadd_ps(r2, r2);
		_mm_store_ss(&r, _mm_sqrt_ss(r3));
		return r;
	}

	__forceinline friend vec4 normalized(const vec4 &a)	{
		__m128 l = _mm_mul_ps(a.v, a.v);
		l = _mm_add_ps(l, _mm_shuffle_ps(l, l, 0x4E));
		return _mm_div_ps(a.v, _mm_sqrt_ps(_mm_add_ps(l, _mm_shuffle_ps(l, l, 0x11))));
	}

	__forceinline vec4 xxxx() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0)); }
	__forceinline vec4 yyyy() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1)); }
	__forceinline vec4 zzzz() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2)); }
	__forceinline vec4 wwww() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3)); }

	__forceinline vec4 xyxy() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 1, 0, 1)); }
	__forceinline vec4 zwzw() const { return _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 3, 2, 3)); }

	__forceinline vec4 yyww() const { return _mm_movehdup_ps(v); }
	__forceinline vec4 xxzz() const { return _mm_moveldup_ps(v); }

	__forceinline float _x() const { float a; _mm_store_ss(&a,        v); return a; }
	__forceinline float _y() const { float a; _mm_store_ss(&a, yyyy().v); return a; }
	__forceinline float _z() const { float a; _mm_store_ss(&a, zzzz().v); return a; }
	__forceinline float _w() const { float a; _mm_store_ss(&a, wwww().v); return a; }

	__forceinline unsigned mask() const { return _mm_movemask_ps(v); }

	//__forceinline friend vec4 divw(const vec4& a) {
	//	return _mm_div_ps(a.v, a.wwww().v);
	//}

	/*
	__forceinline friend vec4 blender(const vec4& b, const vec4& a1, const vec4& a2, const vec4& a3)
	{
		const __m128 xxxx = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(0, 0, 0, 0));
		const __m128 yyyy = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(1, 1, 1, 1));
		const __m128 zzzz = _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(2, 2, 2, 2));
		return _mm_add_ps(_mm_add_ps(_mm_mul_ps(a1.v, xxxx), _mm_mul_ps(a2.v, yyyy)), _mm_mul_ps(a3.v, zzzz));
		//	return b.x*a1 + b.y*a2 + b.z*a3;
	}
	*/

	//__forceinline friend	float length    (const vec4 &a) { return sqrt(dot(a,a)); }

	__forceinline friend  vec4  cross(const vec4 &a, const vec4 &b) {
		//MY_ASSERT(a.w < 0.0001);
		//MY_ASSERT(b.w < 0.0001);
		//return vec4(a.y*b.z-a.z*b.y ,a.z*b.x-a.x*b.z ,a.x*b.y-a.y*b.x, 0); 

		return _mm_sub_ps(
			_mm_mul_ps(_mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(3, 1, 0, 2))),
			_mm_mul_ps(_mm_shuffle_ps(a.v, a.v, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(b.v, b.v, _MM_SHUFFLE(3, 0, 2, 1)))
			);
	}

	__forceinline friend vec3 xyz(const vec4& a) { return vec3(a.x, a.y, a.z); }
	__forceinline friend vec3 zyx(const vec4& a) { return vec3(a.z, a.y, a.x); }

	__forceinline friend vec4 lerp(const vec4 &a, const vec4 &b, const float t)	{
		return (1.0f-t)*a + t*b;
	}

	__forceinline friend vec4 nlerp(const vec4 &a, const vec4 &b, const float t) {
		return normalized(lerp(a, b, t));
	}

	/*
	friend	vec4 reflect(const vec4& i, const vec4& n)
	{
		// http://www.opengl.org/sdk/docs/manglsl/xhtml/reflect.xml
		//	vec4 a;
		//	a = i - 2.0 * dot(n,i) * n;
		//	return a;
		return i - 2.0 * dot(n, i) * n;
	}
	*/

	/*
	friend	vec4  refract( const vec4& i, const vec4& n, float eta )
	{
	vec4 r;
	const float k = 1.0 - eta * eta * (1.0 - dot(n,i)*dot(n,i));
	if ( k < 0.0 ) {
	r.x=r.y=r.z=r.w = 0.0f;
	} else {
	r = eta * i - (eta * dot(n,i) + sqrt(k)) * n;
	}
	return r;
	}
	*/
	/*
	__forceinline friend  vec4  abs(const vec4& a) {
	return vec4( fabs(a.x), fabs(a.y), fabs(a.z), fabs(a.w) );
	}
	*/

	static __forceinline vec4 from_rgb(const int rgb) {
		return vec4(
			(float)((rgb >> 16) & 0xff) / 256.0f,
			(float)((rgb >> 8) & 0xff) / 256.0f,
			(float)((rgb >> 0) & 0xff) / 256.0f,
			0
			);
	}

	static __forceinline vec4 from_argb(const int argb) {
		return vec4(
			(float)((argb >> 16) & 0xff) / 256.0f,
			(float)((argb >> 8) & 0xff) / 256.0f,
			(float)((argb >> 0) & 0xff) / 256.0f,
			(float)((argb >> 24) & 0xff) / 256.0f
			);
	}

	union {
		__m128 v;
		struct { float x, y, z, w; }; // registers 0,1,2,3
		struct { float r, g, b, a; };
	};




};// vec4

__forceinline vec4 operator &(const vec4& a, const vec4& b) { return vec4(_mm_and_ps(a.v, b.v)); }
__forceinline vec4 operator |(const vec4& a, const vec4& b) { return vec4(_mm_or_ps(a.v, b.v)); }





typedef float mvec3[3];
typedef mvec3 mat3[3];

void mat3_init(
	mat3& m,
	const float a00, const float a01, const float a02,
	const float a10, const float a11, const float a12,
	const float a20, const float a21, const float a22
);

void mat3_add(mat3& r, const mat3& a, mat3& b);
void mat3_sub(mat3& r, const mat3& a, mat3& b);
void mat3_mul(mat3& r, const mat3& a, mat3& b);
//void mat3_mul_vec       ( vec3 r, mat3 a, vec3 b );
//void mat3_mul_vec_scl   ( vec3 r, mat3 a, vec3 b, float s ); // scale b by s then xform by a
void mat3_mul_scl(mat3& r, const mat3& a, const float b);
void mat3_orthonormalize(mat3& a);
void mat3_copy(mat3& dest, const mat3& src);
int  mat3_invert(mat3& m, const mat3& r);

// transforms vector i into vector j through affine transform a */
//void affine_xform(affine *a, vec3 i, vec3 j);
//int  affine_invert(affine *src,affine *dst);


//void rotatevectors( vec3 v1, vec3 v2, vec3 u1, vec3 u2, float theta );
//void rotatebase( matrix m, int axis, float theta );



/*
* jan2013... ok.  D(istance) is the distance from the origin along the normal.
* so, positive D(istance) means the normal faces the origin, and negative means it faces away.
*
* another variation is the opposite, which can be convenient since the
* traveling along the plane Normal from the origin D distance, will be a point on the plane
* and continuing to travel in that direction would be in the same direction as the plane faces.
*
* http://www.gamedev.net/topic/308189-line-segment-plane-intersection-code-issues/
* clipping was built from this.
*
* also, distance function is similar.
*
* sometimes it seems like I have it backwards, but I guess it is a common issue and only
* requires that you pick a system and stick to it.
*
* I'm going to stick with "negative-D when the plane normal faces away from the origin"
* instead of positive-D, because it is consistent with (for example) iq's signed-distance function
* for a plane (raymarching)
*
* if I ever decide to flip it, then...
*  1. update the frustum planes derivation to match
*  2. negate the D in distace() calc
*  3. negate the D in the clipLine()
*  4. maybe more?  try to keep this in plane{} class and then it won't be a problem.
*/
struct Plane {

	float distance(const vec4 &v) const;
	static Plane from_origin(const vec4&);
	
	/*
	* clip a line against a plane.
	* from P1 to P2.  returns N, the new coord, and T from 0.0-1.0 for Lerp along that line.
	* IMPORTANT !!!!  ASSUMES P1 And P2 STRADDLE THE PLANE!!!!!
	*/
	__forceinline void clipLine(const vec4& p1, const vec4& p2, vec4& n, float& t) const
	{
		const vec4 dir = p2 - p1;
		const float denom = dot(this->n, dir);
		t = (-dot(p1, this->n)) / denom;
		n = p1 + (t * dir);
	}

	vec4 n;

};//plane




typedef float mvec4[4];
//typedef mvec4 mat4[4];

__declspec(align(16)) struct mat4 {

	__forceinline mat4(const __m128& a, const __m128& b, const __m128& c, const __m128& d) : m1(a), m2(b), m3(c), m4(d) {}

	__forceinline mat4() :
		m1(_mm_setzero_ps()),
		m2(_mm_setzero_ps()),
		m3(_mm_setzero_ps()),
		m4(_mm_setzero_ps()) {}

	__forceinline mat4(const float* const n) :
		m1(_mm_setr_ps(n[ 0], n[ 1], n[ 2], n[ 3])),
		m2(_mm_setr_ps(n[ 4], n[ 5], n[ 6], n[ 7])),
		m3(_mm_setr_ps(n[ 8], n[ 9], n[10], n[11])),
		m4(_mm_setr_ps(n[12], n[13], n[14], n[15])) {}

	static __forceinline mat4 ident() {
		return mat4(vec4(1, 0, 0, 0).v,
					vec4(0, 1, 0, 0).v,
					vec4(0, 0, 1, 0).v,
					vec4(0, 0, 0, 1).v);
	}

	static __forceinline mat4 scale(const vec3& a) {
		return mat4(vec4(a.x,   0,   0,  0).v,
					vec4(  0, a.y,   0,  0).v,
					vec4(  0,   0, a.z,  0).v,
					vec4(  0,   0,   0,  1).v);
	}
	static __forceinline mat4 scale(const vec4& a) {
		return mat4(vec4(a._x(),   0,   0,  0).v,
					vec4(  0, a._y(),   0,  0).v,
					vec4(  0,   0, a._z(),  0).v,
					vec4(  0,   0,   0,  1).v);
	}

	static __forceinline mat4 position(const vec4& a) {
		return mat4(vec4(1, 0, 0, 0).v,
					vec4(0, 1, 0, 0).v,
					vec4(0, 0, 1, 0).v,
					a.v);
	}

	static __forceinline mat4 position(const vec3& a) {
		return mat4::position(vec4(a.x, a.y, a.z, 1));
	}

	static __forceinline mat4 rotate_x(const float theta) {
		const float s_t = sin(theta);
		const float c_t = cos(theta);
		return mat4(vec4(  1,  0,   0,   0).v,
		            vec4(  0, c_t, s_t,  0).v,
		            vec4(  0,-s_t, c_t,  0).v,
		            vec4(  0,  0,   0,   1).v);
	}

	static __forceinline mat4 rotate_y(const float theta) {
		const float s_t = sin(theta);
		const float c_t = cos(theta);
		return mat4(vec4(c_t, 0,-s_t, 0).v,
		            vec4( 0,  1,  0,  0).v,
		            vec4(s_t, 0, c_t, 0).v,
		            vec4( 0,  0,  0,  1).v);
	}

	static __forceinline mat4 rotate_z(const float theta) {
		const float s_t = sin(theta);
		const float c_t = cos(theta);
		return mat4(vec4( c_t, s_t,  0,  0).v,
		            vec4(-s_t, c_t,  0,  0).v,
		            vec4(  0,   0,   1,  0).v,
		            vec4(  0,   0,   0,  1).v);
	}

	union {
		__m128 v[4];
		mvec4 f[4];
		struct { __m128 m1, m2, m3, m4; };
		float ff[16];
	};

};


__forceinline mat4 mat4_init(
	const float a00, const float a01, const float a02, const float a03,
	const float a10, const float a11, const float a12, const float a13,
	const float a20, const float a21, const float a22, const float a23,
	const float a30, const float a31, const float a32, const float a33
)
{
	return mat4(vec4(a00, a10, a20, a30).v,
		        vec4(a01, a11, a21, a31).v,
				vec4(a02, a12, a22, a32).v,
				vec4(a03, a13, a23, a33).v);
}


void mat4_init(mat4& r,
	const float a00, const float a01, const float a02, const float a03,
	const float a10, const float a11, const float a12, const float a13,
	const float a20, const float a21, const float a22, const float a23,
	const float a30, const float a31, const float a32, const float a33
);


void mat4_print(mat4& r);

//void mat4_mul(mat4& r, const mat4& a, const mat4& b);



//mat4 mat4_mul(const mat4& a, const mat4& b);

//void mat4_mul_vec4       ( vec4& r, const mat4& a, const vec4& b );
void mat4_mul_vec41(vec4& r, const mat4& a, const vec4& b);
void mat4_mul_vec40(vec4& r, const mat4& a, const vec4& b); // same but no translation
vec3 mat4_mul_vec30(const mat4& a, const vec3& b); // w=0, no translation
vec3 mat4_mul_vec31(const mat4& a, const vec3& b); // w=1, yes translation

mat4 mat4_inverse(const mat4& src);

//void mat4_copy(mat4& dst, const mat4& src);
//void mat3_to_mat4(mat4& dst, const mat3& src);
mat4 mat4_camera_rot_pos(const vec3& crot, const vec3& cpos);


void mat4_set_pos_rot(mat4& m, const vec4& p, const vec3& r);

mat4 mat4_look_from_to(const vec4& from, const vec4& to); // at is a position
mat4 mat4_look_from_towards(const vec4& from, const vec4& towards); // towards is a direction

/*
__forceinline mat4 mat4_mul(const mat4& a, const mat4& b)
{
	return mat4(
		vec4(a.f[0][0] * b.f[0][0] + a.f[1][0] * b.f[0][1] + a.f[2][0] * b.f[0][2] + a.f[3][0] * b.f[0][3]
		, a.f[0][1] * b.f[0][0] + a.f[1][1] * b.f[0][1] + a.f[2][1] * b.f[0][2] + a.f[3][1] * b.f[0][3]
		, a.f[0][2] * b.f[0][0] + a.f[1][2] * b.f[0][1] + a.f[2][2] * b.f[0][2] + a.f[3][2] * b.f[0][3]
		, a.f[0][3] * b.f[0][0] + a.f[1][3] * b.f[0][1] + a.f[2][3] * b.f[0][2] + a.f[3][3] * b.f[0][3]).v

		, vec4(a.f[0][0] * b.f[1][0] + a.f[1][0] * b.f[1][1] + a.f[2][0] * b.f[1][2] + a.f[3][0] * b.f[1][3]
		, a.f[0][1] * b.f[1][0] + a.f[1][1] * b.f[1][1] + a.f[2][1] * b.f[1][2] + a.f[3][1] * b.f[1][3]
		, a.f[0][2] * b.f[1][0] + a.f[1][2] * b.f[1][1] + a.f[2][2] * b.f[1][2] + a.f[3][2] * b.f[1][3]
		, a.f[0][3] * b.f[1][0] + a.f[1][3] * b.f[1][1] + a.f[2][3] * b.f[1][2] + a.f[3][3] * b.f[1][3]).v

		, vec4(a.f[0][0] * b.f[2][0] + a.f[1][0] * b.f[2][1] + a.f[2][0] * b.f[2][2] + a.f[3][0] * b.f[2][3]
		, a.f[0][1] * b.f[2][0] + a.f[1][1] * b.f[2][1] + a.f[2][1] * b.f[2][2] + a.f[3][1] * b.f[2][3]
		, a.f[0][2] * b.f[2][0] + a.f[1][2] * b.f[2][1] + a.f[2][2] * b.f[2][2] + a.f[3][2] * b.f[2][3]
		, a.f[0][3] * b.f[2][0] + a.f[1][3] * b.f[2][1] + a.f[2][3] * b.f[2][2] + a.f[3][3] * b.f[2][3]).v

		, vec4(a.f[0][0] * b.f[3][0] + a.f[1][0] * b.f[3][1] + a.f[2][0] * b.f[3][2] + a.f[3][0] * b.f[3][3]
		, a.f[0][1] * b.f[3][0] + a.f[1][1] * b.f[3][1] + a.f[2][1] * b.f[3][2] + a.f[3][1] * b.f[3][3]
		, a.f[0][2] * b.f[3][0] + a.f[1][2] * b.f[3][1] + a.f[2][2] * b.f[3][2] + a.f[3][2] * b.f[3][3]
		, a.f[0][3] * b.f[3][0] + a.f[1][3] * b.f[3][1] + a.f[2][3] * b.f[3][2] + a.f[3][3] * b.f[3][3]).v
		);
}
*/



/*
* glRotate style...
*
* http://www.gamedev.net/topic/278216-source-code-for-glrotate/
* and also wikipedia page
*/
__forceinline mat4 mat4_rotate(const float theta, const float x, const float y, const float z)
{
	const float s = sin(theta);
	const float c = cos(theta);
	const float t = 1.0f - c;

	const float tx = t * x;
	const float ty = t * y;
	const float tz = t * z;

	const float sz = s * z;
	const float sy = s * y;
	const float sx = s * x;

	return mat4(vec4(tx * x + c, tx * y + sz, tx * z - sy, 0).v
		, vec4(tx * y - sz, ty * y + c, ty * z + sx, 0).v
		, vec4(tx * z + sy, ty * z - sx, tz * z + c, 0).v
		, vec4(0, 0, 0, 1).v);

}


__forceinline vec4 mat4_mul(const mat4& a, const vec4& b)
{
	/*
	return vec4(_mm_add_ps(_mm_add_ps(_mm_add_ps(
		_mm_mul_ps(a.v[0], b.xxxx().v),
		_mm_mul_ps(a.v[1], b.yyyy().v)),
		_mm_mul_ps(a.v[2], b.zzzz().v)),
		_mm_mul_ps(a.v[3], b.wwww().v))
	);
	*/
	return vec4(a.v[0])*b.xxxx() +
	       vec4(a.v[1])*b.yyyy() +
	       vec4(a.v[2])*b.zzzz() +
		   vec4(a.v[3])*b.wwww();
}


/*
__forceinline mat4 mat4_mul(const mat4& a, const mat4& b)
{
	mat4 r;
	for (int y = 0; y<4; y++) {
		for (int x = 0; x<4; x++) {
			r.f[y][x] = a.f[0][x] * b.f[y][0]
				+ a.f[1][x] * b.f[y][1]
				+ a.f[2][x] * b.f[y][2]
				+ a.f[3][x] * b.f[y][3];
		}
	}
	return r;
}
*/


__forceinline void mat4_mul(const mat4& a, const mat4& b, mat4& r)
{
	r.v[0] = (vec4(a.v[0]) * vec4(b.ff[0 + 0]) +
	          vec4(a.v[1]) * vec4(b.ff[0 + 1]) +
	          vec4(a.v[2]) * vec4(b.ff[0 + 2]) +
	          vec4(a.v[3]) * vec4(b.ff[0 + 3])).v;

	r.v[1] = (vec4(a.v[0]) * vec4(b.ff[4 + 0]) +
	          vec4(a.v[1]) * vec4(b.ff[4 + 1]) +
	          vec4(a.v[2]) * vec4(b.ff[4 + 2]) +
	          vec4(a.v[3]) * vec4(b.ff[4 + 3])).v;

	r.v[2] = (vec4(a.v[0]) * vec4(b.ff[8 + 0]) +
	          vec4(a.v[1]) * vec4(b.ff[8 + 1]) +
	          vec4(a.v[2]) * vec4(b.ff[8 + 2]) +
	          vec4(a.v[3]) * vec4(b.ff[8 + 3])).v;

	r.v[3] = (vec4(a.v[0]) * vec4(b.ff[12+ 0]) +
	          vec4(a.v[1]) * vec4(b.ff[12+ 1]) +
	          vec4(a.v[2]) * vec4(b.ff[12+ 2]) +
	          vec4(a.v[3]) * vec4(b.ff[12+ 3])).v;
}
__forceinline mat4 mat4_mul(const mat4& a, const mat4& b)
{
	mat4 r;
	mat4_mul(a, b, r);
	return r;
}

/*
* signed 32x32 bit integer mul with low 32bit result
* http://stackoverflow.com/questions/10500766/sse-multiplication-of-4-32-bit-integers
* this works in SSE2.
* for sse4.1, can just use _mm_mullo_epi32()
*/
static inline __m128i sse2_mul32(const __m128i& a, const __m128i& b)
{
	// mul 2,0
	const __m128i tmp1 = _mm_mul_epu32(a, b);

	// mul 3,1
	const __m128i tmp2 = _mm_mul_epu32(
		_mm_srli_si128(a, 4)
		, _mm_srli_si128(b, 4));

	// shuffle results to [63...0] and pack
	return _mm_unpacklo_epi32(
		_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0))
		, _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0))
		);
}





__declspec(align(16)) struct ivec4 {

	__forceinline ivec4() {}
	//__forceinline ivec4():v(_mm_setzero_si128()){}

	__forceinline ivec4(const int a, const int b, const int c, const int d) : v(_mm_set_epi32(d, c, b, a)){}
	__forceinline ivec4(const int a) : v(_mm_set_epi32(a, a, a, a)){}
	__forceinline ivec4(__m128i m) : v(m){}
	__forceinline ivec4(const ivec4& x) : v(x.v) {}

	static __forceinline ivec4 zero() { return ivec4(_mm_setzero_si128()); }
	static const ivec4 two;

	__forceinline ivec4& operator  =(const ivec4& b) { v = b.v;  return *this; }
	__forceinline ivec4& operator +=(const ivec4& b) { v = _mm_add_epi32(v, b.v); return *this; }
	__forceinline ivec4& operator -=(const ivec4& b) { v = _mm_sub_epi32(v, b.v); return *this; }
	__forceinline ivec4& operator *=(const ivec4& b) { v = sse2_mul32(v, b.v); return *this; }
	__forceinline ivec4& operator |=(const ivec4& b) { v = _mm_or_si128(v, b.v); return *this; }
	__forceinline ivec4& operator &=(const ivec4& b) { v = _mm_and_si128(v, b.v); return *this; }

public:
	union {
		__m128i v;
		__m128 f;
		struct { int x, y, z, w; };
	};
};

__forceinline ivec4 operator +(const ivec4& a, const ivec4& b) { return ivec4(_mm_add_epi32(a.v, b.v)); }
__forceinline ivec4 operator -(const ivec4& a, const ivec4& b) { return ivec4(_mm_sub_epi32(a.v, b.v)); }
__forceinline ivec4 operator *(const ivec4& a, const ivec4& b) { return ivec4(sse2_mul32(a.v, b.v)); }
__forceinline ivec4 operator |(const ivec4& a, const ivec4& b) { return ivec4(_mm_or_si128(a.v, b.v)); }
__forceinline ivec4 operator &(const ivec4& a, const ivec4& b) { return ivec4(_mm_and_si128(a.v, b.v)); }

__forceinline ivec4 andnot(const ivec4& a, const ivec4& b) { return ivec4(_mm_andnot_si128(a.v, b.v)); }

//__forceinline ivec4 vmin( const ivec4& a, const ivec4& b ) { return ivec4(_mm_min_epi32(a.v,b.v)); }
//__forceinline ivec4 vmax( const ivec4& a, const ivec4& b ) { return ivec4(_mm_max_epi32(a.v,b.v)); }
__forceinline  vec4 vmin(const  vec4& a, const  vec4& b) { return  vec4(_mm_min_ps(a.v, b.v)); }
__forceinline  vec4 vmax(const  vec4& a, const  vec4& b) { return  vec4(_mm_max_ps(a.v, b.v)); }

__forceinline vec4 itof(const ivec4& a) { return vec4(_mm_cvtepi32_ps(a.v)); }
__forceinline ivec4 ftoi_round(const vec4& a) { return ivec4(_mm_cvtps_epi32(a.v)); }
__forceinline ivec4 ftoi(const vec4& a) { return ivec4(_mm_cvttps_epi32(a.v)); }

__forceinline ivec4 cmplt(const ivec4&a, const ivec4& b) { return ivec4(_mm_cmplt_epi32(a.v, b.v)); }
//__forceinline ivec4 cmple(const ivec4&a, const ivec4& b) { return ivec4(_mm_cmple_epi32(a.v,b.v)); }
//__forceinline ivec4 cmpge(const ivec4&a, const ivec4& b) { return ivec4(_mm_cmpge_epi32(a.v,b.v)); }
__forceinline ivec4 cmpeq(const ivec4&a, const ivec4& b) { return ivec4(_mm_cmpeq_epi32(a.v, b.v)); }
__forceinline ivec4 cmpgt(const ivec4&a, const ivec4& b) { return ivec4(_mm_cmpgt_epi32(a.v, b.v)); }

__forceinline  vec4 cmplt(const  vec4&a, const  vec4& b) { return  vec4(_mm_cmplt_ps(a.v, b.v)); }
__forceinline  vec4 cmple(const  vec4&a, const  vec4& b) { return  vec4(_mm_cmple_ps(a.v, b.v)); }
__forceinline  vec4 cmpge(const  vec4&a, const  vec4& b) { return  vec4(_mm_cmpge_ps(a.v, b.v)); }
__forceinline  vec4 cmpgt(const  vec4&a, const  vec4& b) { return  vec4(_mm_cmpgt_ps(a.v, b.v)); }

__forceinline ivec4 float2bits(const  vec4 &a) { return ivec4(_mm_castps_si128(a.v)); }
__forceinline  vec4 bits2float(const ivec4 &a) { return  vec4(_mm_castsi128_ps(a.v)); }

__forceinline int movemask(const vec4& a) { return _mm_movemask_ps(a.v); }


/*
* sum three vectors using three weights (e.g., 0.0 ... 1.0)
*/
//__forceinline vec4 muladd3(const vec4 * const w, const vec4 * const v)
__forceinline vec4 muladd3(const vec4 * __restrict const w, const vec4 * __restrict const v)
{
	return (w[0] * v[0]) + (w[1] * v[1]) + (w[2] * v[2]);
	//	_mm_add_ps( _mm_add_ps(
	//		_mm_mul_ps(w[0],v[0]), _mm_mul_ps(w[1],v[1]) ), _mm_mul_ps(w[2],v[2]) );
}

/*
template <int n>
__forceinline vec4 mulsum(const vec4 * __restrict const w, const vec4 * __restrict const v)
{
	auto ax = w[0] * v[0];
	for (int i = 1; i < n; i++) {
		ax += w[i] * v[i];
	}
	return ax;
}
*/



//__forceinline vec4 dot_xyz(const vec4* const a, const vec4* const b) { return vec4(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]); }
//__forceinline vec4 length_xyz(const vec4 * const xyz) { return vec4(_mm_sqrt_ps(dot_xyz(xyz, xyz).v)); }
//__forceinline vec4 rlength_xyz(const vec4 * const xyz) { return vec4(_mm_rsqrt_ps(dot_xyz(xyz, xyz).v)); }

/*
__forceinline void normalize_xyz(vec4* const xyz)
{
	const vec4 rlen = rlength_xyz(xyz);
	xyz[0] *= rlen;
	xyz[1] *= rlen;
	xyz[2] *= rlen;
}
*/

/*
__forceinline void normalize_xyz(vec4* const dst, const vec4* const src)
{
	const vec4 rlen = rlength_xyz(src);
	dst[0] = src[0] * rlen;
	dst[1] = src[1] * rlen;
	dst[2] = src[2] * rlen;
}
*/


__forceinline vec4 saturate(const vec4& a) {
	return vmax(vmin(a, vec4(1.0f)), vec4::zero());
}


/*
__forceinline vec4 smoothstep(const vec4& a, const vec4& b, const vec4& x)
{
	// t = saturate( (x-a)/(b-a) );
	const vec4 t(saturate((x - a) / (b - a)));

	// return t * t * (3.0 - (2.0*t));
	return t * t * (vec4(3.0f) - (t + t));
}
*/


/*
* assume a is zero
*/
__forceinline vec4 smoothstep_zero(const vec4& b, const vec4& x) {
	const vec4 t(saturate(x / b));
	return t * t * (vec4(3.0f) - (t + t));
}

/*
* source form liranuna's glsl sse stuff
*/
__forceinline vec4 fract(const vec4& a)
{
	return  a - itof(ftoi_round(a - vec4(0.5f)));

	//	const __m128 wholepart = _mm_cvtepi32_ps(_mm_cvtps_epi32(a-vec4(0.5f))
	//	return  _mm_sub_ps(a, itof(ftoi_round(a-vec4(0.5f));
	//	_mm_cvtps_epi32(_mm_sub_ps(a,_mm_set1_ps(0.5f)))));
}


/*
* select*() based on ryg's helpersse
* sse2 equivalents first found in "sseplus" sourcecode
* http://sseplus.sourceforge.net/group__emulated___s_s_e2.html#g3065fcafc03eed79c9d7539435c3257c
*
* i split them into bits and sign so i can save a step when I know that I have
* a complete bitmask or not
*/
__forceinline vec4 selectbits(const vec4& a, const vec4& b, const ivec4& mask)
{
	const ivec4 a2 = andnot(mask, float2bits(a));	// keep bits in a where mask is 0000's
	const ivec4 b2 = float2bits(b) & mask;		// keep bits in b where mask is 1111's
	return bits2float(a2 | b2);
}
__forceinline vec4 selectbits(const vec4& a, const vec4& b, const vec4& mask)
{
	return selectbits(a, b, float2bits(mask));
}
/* sse 4.1
__forceinline vec4 blendv( const vec4& a, const vec4& b, const ivec4& mask )
{
return _mm_blendv_ps( a.v, b.v, bits2float(mask).v );
}
*/
/* this is _mm_blendv_ps() for SSE2 */
__forceinline vec4 select_by_sign(const vec4& a, const vec4& b, const ivec4& mask)
{
	const ivec4 newmask(_mm_srai_epi32(mask.v, 31));
	return selectbits(a, b, newmask);
}
__forceinline vec4 select_by_sign(const vec4& a, const vec4& b, const vec4& mask)
{
	const ivec4 newmask(_mm_srai_epi32(float2bits(mask).v, 31));
	return selectbits(a, b, newmask);
}

__forceinline vec4 oneover(const vec4& a) { return vec4(_mm_rcp_ps(a.v)); }

//__forceinline vec4 abs( const vec4& a ) { return andnot(signmask,a); }

/*
#include "fast_sse_pow.h"
__forceinline vec4 pow(const vec4& x, const vec4& y)
{
	return vec4(exp2f4(_mm_mul_ps(log2f4(x.v), y.v)));
}
*/

/*
* this is found in the intel intrinsics database (java thing) as "_mm__MM_TRANSPOSE4_PS"
*/
__forceinline void transpose4x4(__m128& r0, __m128& r1, __m128& r2, __m128& r3)
{
	__m128 tmp3, tmp2, tmp1, tmp0;
	tmp0 = _mm_unpacklo_ps(r0, r1);
	tmp2 = _mm_unpacklo_ps(r2, r3);
	tmp1 = _mm_unpackhi_ps(r0, r1);
	tmp3 = _mm_unpackhi_ps(r2, r3);
	r0 = _mm_movelh_ps(tmp0, tmp2);
	r1 = _mm_movehl_ps(tmp2, tmp0);
	r2 = _mm_movelh_ps(tmp1, tmp3);
	r3 = _mm_movehl_ps(tmp3, tmp1);
}

__forceinline void transpout3x4(const __m128& r0, const __m128& r1, const __m128& r2, const __m128& r3, __m128& o0, __m128& o1, __m128& o2)
{
	__m128 tmp3, tmp2, tmp1, tmp0;
	tmp0 = _mm_unpacklo_ps(r0, r1);
	tmp2 = _mm_unpacklo_ps(r2, r3);
	tmp1 = _mm_unpackhi_ps(r0, r1);
	tmp3 = _mm_unpackhi_ps(r2, r3);
	o0 = _mm_movelh_ps(tmp0, tmp2);
	o1 = _mm_movehl_ps(tmp2, tmp0);
	o2 = _mm_movelh_ps(tmp1, tmp3);
	//	o3 = _mm_movehl_ps(tmp3,tmp1);
}

__forceinline void transpout4x4(const __m128& r0, const __m128& r1, const __m128& r2, const __m128& r3, __m128& o0, __m128& o1, __m128& o2, __m128& o3)
{
	__m128 tmp3, tmp2, tmp1, tmp0;
	tmp0 = _mm_unpacklo_ps(r0, r1);
	tmp2 = _mm_unpacklo_ps(r2, r3);
	tmp1 = _mm_unpackhi_ps(r0, r1);
	tmp3 = _mm_unpackhi_ps(r2, r3);
	o0 = _mm_movelh_ps(tmp0, tmp2);
	o1 = _mm_movehl_ps(tmp2, tmp0);
	o2 = _mm_movelh_ps(tmp1, tmp3);
	o3 = _mm_movehl_ps(tmp3, tmp1);
}


__forceinline void transpose4(vec4& r0, vec4& r1, vec4& r2, vec4& r3)
{
	transpose4x4(r0.v, r1.v, r2.v, r3.v);
}

__forceinline void transpose4(vec4 * const __restrict r)
{
	transpose4x4(r[0].v, r[1].v, r[2].v, r[3].v);
}

__forceinline void transpout3x4(const vec4* const r, vec4& o0, vec4& o1, vec4& o2)
{
	transpout3x4(r[0].v, r[1].v, r[2].v, r[3].v, o0.v, o1.v, o2.v);
}
__forceinline void transpout4x4(const vec4* const r, vec4& o0, vec4& o1, vec4& o2, vec4& o3)
{
	transpout4x4(r[0].v, r[1].v, r[2].v, r[3].v, o0.v, o1.v, o2.v, o3.v);
}

__forceinline void transpout4x4(const vec4* const r, __m128* o1, __m128* o2, __m128* o3, __m128 *o4)
{
	__m128 tmp3, tmp2, tmp1, tmp0;
	tmp0 = _mm_unpacklo_ps(r[0].v, r[1].v);
	tmp2 = _mm_unpacklo_ps(r[2].v, r[3].v);
	tmp1 = _mm_unpackhi_ps(r[0].v, r[1].v);
	tmp3 = _mm_unpackhi_ps(r[2].v, r[3].v);
	_mm_stream_ps((float*)o1, _mm_movelh_ps(tmp0, tmp2));
	_mm_stream_ps((float*)o2, _mm_movehl_ps(tmp2, tmp0));
	_mm_stream_ps((float*)o3, _mm_movelh_ps(tmp1, tmp3));
	_mm_stream_ps((float*)o4, _mm_movehl_ps(tmp3, tmp1));
}


template<int N> __forceinline ivec4 shl(const ivec4& x) { return ivec4(_mm_slli_epi32(x.v, N)); }
template<int N> __forceinline ivec4 sar(const ivec4& x) { return ivec4(_mm_srai_epi32(x.v, N)); }



/*
* some functions for sin/cos ...
* based mostly on Nick's thread here:
*  http://devmaster.net/forums/topic/4648-fast-and-accurate-sinecosine/page__st__80
* also referenced on pouet:
*  http://www.pouet.scene.org/topic.php?which=9132&page=1
* also interesting:
*  ISPC stdlib sincos, https://github.com/ispc/ispc/blob/master/stdlib.ispc
* and another here..
*  http://gruntthepeon.free.fr/ssemath/
*
* I'll use nick's.
* It approximates sin() from a parabola, so it requires the input be in
* the range -PI ... +PI.
*
* These functions can be combined or decomposed to optimize a few
* instructions if the inputs are known to fit, and also if they are in the range -1 ... +1
*/

/*
* wrap a float to the range -1 ... +1
*/
__forceinline vec4 wrap1(const vec4& a)
{
	const __m128 magic = _mm_set1_ps(25165824.0f); // 0x4bc00000
	const vec4 z = a + magic;
	return a - (z - magic);
}

/*
* implementation of nick's original -pi...pi version
*/
template<bool high_precision>
__forceinline vec4 nick_sin(const vec4& x)
{
	const vec4 B(4 / M_PI);
	const vec4 C(-4 / (M_PI * M_PI));

	vec4 y = B*x + C*x * abs(x);

	if (high_precision) {
		const vec4 P(0.225);
		y = P * (y*abs(y) - y) + y;
	}
	return y;
}




/*
* sin() but input must be in the range -1...1
* higher precision
*/
__forceinline vec4 sin1hp(const vec4& x)
{
	const vec4 Q(3.1f);
	const vec4 P(3.6f);
	vec4 y = x - (x * abs(x));
	return y * (Q + P * abs(y));
}

/* same, but low-precision */
__forceinline vec4 sin1lp(const vec4& x)
{
	return vec4(4.0f) * (x - x * abs(x));
}

/*
* if x is in radians, and needs to be wrapped, higher precision
* ie, drop-in-sin()-replacement
*/
__forceinline vec4 sin(const vec4& x)
{
	vec4 M = x * vec4(1.0f / PI_F);	// scale x to -1...1
	M = wrap1(M);					// wrap to -1...1
	return sin1hp(M);
}

/*
* cos(x) = sin( pi/2 + x)
* if... -1= -pi,  so...
* bring into range -1 ... 1, then add 0.5, then wrap.
*/
__forceinline vec4 cos(const vec4& x)
{
	vec4 M = x * vec4(1.0f / PI_F);	// scale x to -1...1
	M = wrap1(M + vec4(0.5));		// add 0.5 (PI/2) and wrap
	return sin1hp(M);
}

// calculate sin and cos.... but not much is gained, saves one mul.
__forceinline void sincos(const vec4& x, vec4 &os, vec4& oc)
{
	vec4 M = x * vec4(1.0f / PI_F);	// scale x to -1...1
	os = sin1hp(wrap1(M));
	oc = sin1hp(wrap1(M + vec4(0.5)));
}

__forceinline vec4 slowsin(const vec4& x)
{
	return vec4(sinf(x.x), sinf(x.y), sinf(x.z), sinf(x.w));
}

__forceinline vec4 slowcos(const vec4& x)
{
	return vec4(cosf(x.x), cosf(x.y), cosf(x.z), cosf(x.w));
}




class Subdivider {

private:
	const float margin_l_per_element;
	const float span;
	const int elements;

public:

	Subdivider(const int elements, const float ml, const float mr)
		:elements(elements),
		 span(1 - (ml + mr)),
		 margin_l_per_element(ml / elements) {}

	__forceinline float f(const int n, const float t) const	{
		const float x = t - n * margin_l_per_element;
		return std::min(std::max(x, 0.0f), span) / span;
	}

};



class MatrixStack {

	std::array<mat4, 32> stack;
	int sp;

public:
	MatrixStack() :sp(0) {
		stack[0] = mat4::ident();
	}

	__forceinline MatrixStack& push(const mat4& newxform) {
		//MY_ASSERT(m_sp<stacksize - 1);
		mat4_mul(stack[sp], newxform, stack[sp + 1]);
		//stack[sp + 1] = mat4_mul(stack[sp], newxform);
		sp++;
		return *this;
	}

	__forceinline void pop() {
		//MY_ASSERT(m_sp>0);
		sp--;
	}

	__forceinline void set(const mat4& newxform) {
		stack[sp] = newxform;
	}
	__forceinline const mat4& get() const {
		return stack[sp];
	}
//	__forceinline void ident() {
//		stack[sp] = mat4::ident();
//	}
	__forceinline MatrixStack& position(const vec4& a) { return this->push(mat4::position(a)); }
	__forceinline MatrixStack& position(const vec3& a) { return this->push(mat4::position(a)); }

	__forceinline MatrixStack& scale(const vec3& a) { return this->push(mat4::scale(a)); }

	__forceinline MatrixStack& rotate_x(const float theta) { return this->push(mat4::rotate_x(theta)); }
	__forceinline MatrixStack& rotate_y(const float theta) { return this->push(mat4::rotate_y(theta)); }
	__forceinline MatrixStack& rotate_z(const float theta) { return this->push(mat4::rotate_z(theta)); }

};

/*
void test()
{
	mat4 test = MatrixStack().translate(vec3(1, 2, 3)).rotate_z(0).rotate_x(0.23f).get();
}
*/


std::ostream& operator<<(std::ostream& os, const vec4& v);
std::ostream& operator<<(std::ostream& os, const vec3& v);
std::ostream& operator<<(std::ostream& os, const Plane& p);

#endif //__VEC_H
