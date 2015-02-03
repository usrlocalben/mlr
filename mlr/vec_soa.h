
#ifndef __VEC_SOA_H
#define __VEC_SOA_H

#include "stdafx.h"

#include "vec.h"

/*
 * SoA helpers for fragments (quads)
 */
typedef vec4 qfloat;
struct qfloat2 {
	vec4 v[2];
};
struct qfloat3 {
	vec4 v[3];
	__forceinline void set(const vec4& a) {
		v[0] = a.xxxx();
		v[1] = a.yyyy();
		v[2] = a.zzzz();
	}
};
struct qfloat4 {
	vec4 v[4];
	__forceinline void set(const qfloat3& a){
		v[0] = a.v[0];
		v[1] = a.v[1];
		v[2] = a.v[2];
	}
};
__forceinline qfloat3 qmul(const qfloat3& a, const qfloat& b) {
	qfloat3 r;
	r.v[0] = a.v[0] * b;
	r.v[1] = a.v[1] * b;
	r.v[2] = a.v[2] * b;
	return r;
}
__forceinline vec4 dFdx(const vec4& a) {
	return a.yyww() - a.xxzz();
}
__forceinline vec4 dFdy(const vec4& a) {
	return a.xyxy() - a.zwzw();
}
__forceinline qfloat fwidth(const qfloat& a) {
	qfloat r;
	r = abs(dFdx(a)) + abs(dFdy(a));
	return r;
}
__forceinline qfloat2 fwidth(const qfloat2& a) {
	qfloat2 r;
	r.v[0] = abs(dFdx(a.v[0])) + abs(dFdy(a.v[0]));
	r.v[1] = abs(dFdx(a.v[1])) + abs(dFdy(a.v[1]));
	return r;
}
__forceinline qfloat3 fwidth(const qfloat3& a) {
	qfloat3 r;
	r.v[0] = abs(dFdx(a.v[0])) + abs(dFdy(a.v[0]));
	r.v[1] = abs(dFdx(a.v[1])) + abs(dFdy(a.v[1]));
	r.v[2] = abs(dFdx(a.v[2])) + abs(dFdy(a.v[2]));
	return r;
}


/*
 * SoA helpers for vertex interpolants
 */
struct vertex_float {
	vec4 x[3];
	__forceinline void set(const vec4& a, const vec4& b, const vec4& c) {
		x[0] = a;
		x[1] = b;
		x[2] = c;
	}
};
struct vertex_float2 {
	vec4 x[3];
	vec4 y[3];
	__forceinline void fill(const vec4& v0, const vec4& v1, const vec4& v2) {
		x[0] = v0.xxxx();  y[0] = v0.yyyy();
		x[1] = v1.xxxx();  y[1] = v1.yyyy();
		x[2] = v2.xxxx();  y[2] = v2.yyyy();
	}
};
struct vertex_float3 {
	vec4 x[3];
	vec4 y[3];
	vec4 z[3];
	void fill(const vec4& v0, const vec4& v1, const vec4& v2) {
		x[0] = v0.xxxx();  y[0] = v0.yyyy();  z[0] = v0.zzzz();
		x[1] = v1.xxxx();  y[1] = v1.yyyy();  z[1] = v1.zzzz();
		x[2] = v2.xxxx();  y[2] = v2.yyyy();  z[2] = v2.zzzz();
	}
};
__forceinline qfloat vertex_blend(const vertex_float& bary, const vertex_float& val) {
	qfloat r;
	r = bary.x[0] * val.x[0] + bary.x[1] * val.x[1] + bary.x[2] * val.x[2];
	return r;
}
__forceinline qfloat2 vertex_blend(const vertex_float& bary, const vertex_float2& val) {
	qfloat2 r;
	r.v[0] = bary.x[0] * val.x[0] + bary.x[1] * val.x[1] + bary.x[2] * val.x[2];
	r.v[1] = bary.x[0] * val.y[0] + bary.x[1] * val.y[1] + bary.x[2] * val.y[2];
	return r;
}
__forceinline qfloat3 vertex_blend(const vertex_float& bary, const vertex_float3& val) {
	qfloat3 r;
	r.v[0] = bary.x[0] * val.x[0] + bary.x[1] * val.x[1] + bary.x[2] * val.x[2];
	r.v[1] = bary.x[0] * val.y[0] + bary.x[1] * val.y[1] + bary.x[2] * val.y[2];
	r.v[2] = bary.x[0] * val.z[0] + bary.x[1] * val.z[1] + bary.x[2] * val.z[2];
	return r;
}
__forceinline qfloat3 fwidth(const vertex_float& a) {
	qfloat3 r;
	r.v[0] = abs(dFdx(a.x[0])) + abs(dFdy(a.x[0]));
	r.v[1] = abs(dFdx(a.x[1])) + abs(dFdy(a.x[1]));
	r.v[2] = abs(dFdx(a.x[2])) + abs(dFdy(a.x[2]));
	return r;
}
__forceinline qfloat3 smoothstep_zero(const qfloat3& b, const vertex_float& x) {
	qfloat3 r;
	r.v[0] = smoothstep_zero(b.v[0], x.x[0]);
	r.v[1] = smoothstep_zero(b.v[1], x.x[1]);
	r.v[2] = smoothstep_zero(b.v[2], x.x[2]);
	return r;
}
__forceinline vec4 mix(const vec4& a, const vec4& b, const vec4& t) {
	return a*(vec4(1.0f) - t) + b*t;
}



__forceinline vec4 ddx(const vec4& a)
{
	return vec4(_mm_sub_ps(_mm_movehdup_ps(a.v), _mm_moveldup_ps(a.v)));
}

__forceinline vec4 ddy(const vec4& a)
{
	// r0 = a2 - a0
	// r1 = a3 - a1
	// r2 = a2 - a0
	// r3 = a3 - a1

	// SSE3 self-shuffles
	__m128 quadtop = _mm_shufd(a.v, _MM_SHUFFLE(0, 1, 0, 1));
	__m128 quadbot = _mm_shufd(a.v, _MM_SHUFFLE(2, 3, 2, 3));
	//__m128 quadtop = _mm_shuffle_ps( a, a, _MM_SHUFFLE(0,1,0,1) );
	//__m128 quadbot = _mm_shuffle_ps( a, a, _MM_SHUFFLE(2,3,2,3) );

	return vec4(_mm_sub_ps(quadbot, quadtop));
}


template<int e>
__forceinline void fwidth(const vec4 * const __restrict src, vec4 * const __restrict out)
{
	for (int i=0; i<e; i++)
		dst[i] = abs(ddx(s[i])) + abs(ddy(s[i]));
}

#endif //__VEC_SOA_H
