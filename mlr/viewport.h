
#ifndef __VIEWPORT_H
#define __VIEWPORT_H

#include "stdafx.h"

#include "vec.h"

struct Viewport
{
	Viewport(const float aspect, const float fov);
	void test();

	__forceinline vec4 eye_to_clip(const vec4& src) const {
		return mat4_mul(this->mp, src);
	}

	Plane frust[6];

	mat4 mp;	// projection matrix

	float fovy;
	float aspect;
	float znear;
	float zfar;

	__forceinline bool is_visible(const vec4 * const __restrict b) const {
		int res = 0;
		for (int fi = 0; fi < 6; fi++) {
			int inside = 0;
			for (int i = 0; i < 8; i++) {
				const float dist = frust[fi].distance(b[i]);
				if (dist >= 0) inside++;
			}
			if (inside == 0)
				return false;
		}
		return true;
	}
};



class Viewdevice {

public:
	Viewdevice(const int sx, const int sy);

	__forceinline vec4 clip_to_screen(const vec4& src) const {
		return mat4_mul(this->md, src);
	}
	/*
	__forceinline vec4 clip_to_device(const vec4& point_in_clipspace) const {
		auto point_in_screenspace = clip_to_screen(point_in_clipspace);
		float one_over_w = 1.0f / point_in_screenspace._w();
		auto point_in_devicespace = point_in_screenspace / point_in_screenspace.wwww();
		point_in_devicespace.w = one_over_w;
		return point_in_devicespace;
	}
	*/
	__forceinline vec4 clip_to_device(const vec4& point_in_clipspace) const {
		static const __m128 one = _mm_set1_ps(1.0f);
		__m128 src = clip_to_screen(point_in_clipspace).v;
		__m128 r1 = _mm_div_ps(one, src);
		__m128 invw = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 r2 = _mm_mul_ps(src, invw);
		__m128 r3 = _mm_movehl_ps(r2, r1);
		__m128 r4 = _mm_shuffle_ps(r2, r3, _MM_SHUFFLE(1, 2, 1, 0));
		return r4;
	}

public:
	const int width;
	const int height;
private:
	mat4 md;
};

#endif //__VIEWPORT_H

