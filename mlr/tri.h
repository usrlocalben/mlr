
#ifndef __TRI_H
#define __TRI_H

#include "stdafx.h"

#include <cmath>

#include "ryg_srgb.h"
#include "vec_soa.h"
#include "canvas.h"
#include "vec.h"

using namespace PixelToaster;

const ivec4 iqx(0,1,0,1);
const ivec4 iqy(0,0,1,1);
const vec4 fqx(0,1,0,1);
const vec4 fqy(0,0,1,1);

__forceinline int iround(const float x)
{
	int t;
	__asm {
		fld x
		fistp t
	}
	return t;
}


struct Edge {
	int c;
	ivec4 b, block_left_start;
	ivec4 bdx, bdy;

	void setup(const int x1, const int y1, const int x2, const int y2, const int startx, const int starty) {
		const int dx = x1 - x2;
		const int dy = y2 - y1;
		
		c = dy*((startx << 4) - x1) + dx*((starty << 4) - y1);

		// correct for top/left fill convention
		if (dy > 0 || (dy == 0 && dx > 0)) c++;

		c = (c - 1) >> 4;

		block_left_start = ivec4(c) + iqx*dy + iqy*dx;
		b = block_left_start;
		bdx = ivec4(dy * 2);
		bdy = ivec4(dx * 2);
	}

	__forceinline void inc_y() {
		block_left_start += bdy;
		b = block_left_start;
	}

	__forceinline void inc_x() {
		b += bdx;
	}
	__forceinline const ivec4 val() {
		return b;
	}

	/*
	__forceinline void inc_y() {
		b += bdy;
		bdx *= ivec4(-1);
	}
	__forceinline void inc_x() {
		b += bdx;
	}
	__forceinline const ivec4 val() {
		return b;
	}
	*/
};


template <typename FRAGMENT_PROCESSOR>
void draw_triangle(const irect& r, const vec4& s1, const vec4& s2, const vec4& s3, FRAGMENT_PROCESSOR& fp)
{
	const int x1 = iround(16.0f * s1.x);
	const int x2 = iround(16.0f * s2.x);
	const int x3 = iround(16.0f * s3.x);

	const int y1 = iround(16.0f * s1.y);
	const int y2 = iround(16.0f * s2.y);
	const int y3 = iround(16.0f * s3.y);

	int minx = max((min(min(x1,x2),x3) + 0xf) >> 4, r.x0);
	int maxx = min((max(max(x1,x2),x3) + 0xf) >> 4, r.x1);
	int miny = max((min(min(y1,y2),y3) + 0xf) >> 4, r.y0);
	int maxy = min((max(max(y1,y2),y3) + 0xf) >> 4, r.y1);

	const int q = 2; // block size is 2x2
	minx &= ~(q - 1); // align to 2x2 block
	miny &= ~(q - 1);

	Edge e[3];
	e[0].setup(x1, y1, x2, y2, minx, miny);
	e[1].setup(x2, y2, x3, y3, minx, miny);
	e[2].setup(x3, y3, x1, y1, minx, miny);

	const float _s = 1.0f / (e[0].c + e[1].c + e[2].c);
	const vec4 scale(_s);

	fp.goto_xy(minx, miny);

	for (int y = miny; y < maxy; y += 2, e[0].inc_y(), e[1].inc_y(), e[2].inc_y(), fp.inc_y()) {
		for (int x = minx; x < maxx; x += 2, e[0].inc_x(), e[1].inc_x(), e[2].inc_x(), fp.inc_x()) {

			const ivec4 edges(e[0].val() | e[1].val() | e[2].val());
			if (movemask(bits2float(edges)) == 0xf) continue;
			const ivec4 trimask(sar<31>(edges));

			qfloat2 frag_coord = { vec4(x+0.5f)+fqx, vec4(y+0.5f)+fqy };

			vertex_float bary;
			bary.x[0] = itof(e[1].val()) * scale;
			bary.x[2] = itof(e[0].val()) * scale;
			bary.x[1] = vec4(1.0f) - (bary.x[0] + bary.x[2]);

			fp.render(frag_coord, trimask, bary);
		}
	}
}


template <typename FRAGMENT_PROCESSOR>
void draw_rectangle(const irect& r, FRAGMENT_PROCESSOR& fp)
{
	int minx = r.x0;
	int maxx = r.x1;
	int miny = r.y0;
	int maxy = r.y1;

	const int q = 2; // block size is 2x2
//	minx &= ~(q - 1); // align to 2x2 block
//	miny &= ~(q - 1);

	fp.goto_xy(minx, miny);
	for (int y = miny; y < maxy; y += 2, fp.inc_y())
	for (int x = minx; x < maxx; x += 2, fp.inc_x()) {
		qfloat2 frag_coord = { vec4(x+0.5f)+fqx, vec4(y+0.5f)+fqy };
		fp.render(frag_coord);
	}
}

#endif //__TRI_H
