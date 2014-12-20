
#ifndef __VIEWPORT_H
#define __VIEWPORT_H

#include "stdafx.h"

#include "vec.h"

struct Viewport
{
	Viewport(const unsigned sx, const unsigned sy, const float aspect, const float fov);
	void test();

	__forceinline vec4 to_clip_space(const vec4& src) const { return mat4_mul(this->mp, src); }
	__forceinline vec4 to_screen_space(const vec4& src) const { return mat4_mul(this->md, src); }

	Plane frust[6];

	unsigned width;
	unsigned height;

	int xfix, yfix;
	vec4 xyfix;

	mat4 mp;	// projection matrix
	mat4 md;	// device matrix

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

#endif //__VIEWPORT_H
