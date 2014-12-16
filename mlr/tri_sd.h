
#ifndef __TRI_SD_H
#define __TRI_SD_H

#include "stdafx.h"

#include <array>

#include "ryg_srgb.h"
#include "vec_soa.h"
#include "canvas.h"
#include "vec.h"


/*
__forceinline int iround(const float x)
{
	int t;
	__asm {
		fld x
		fistp t
	}
	return t;
}
*/

struct sdEdge {
	int c;
	int cb;
	int ex, ey;
	int nmin, nmax;
	int tls, val;
	int dx, dy;

	void setup(const int x1, const int y1, const int x2, const int y2, const int startx, const int starty, const int q) {
		dx = x1 - x2;
		dy = y2 - y1;

		c = dy*((startx << 4) - x1) + dx*((starty << 4) - y1);

		// correct for top/left fill convention
		if (dy > 0 || (dy == 0 && dx > 0)) c++;

		c = (c - 1) >> 4;

		nmin = 0;
		nmax = 0;
		if (dx >= 0) nmax -= q*dx; else nmin -= q*dx;
		if (dy >= 0) nmax -= q*dy; else nmin -= q*dy;

		cb = c;
		ex = dy * q;
		ey = dx * q;
	}
	__forceinline void inc_y() {
		cb += ey;
	}
	__forceinline void inc_x() {
		cb += ex;
	}
	__forceinline void reverse_x() {
		ex = -ex;
	}
	__forceinline bool good() {
		return cb >= nmax;
	}
	__forceinline bool outside() {
		return cb < nmax;
	}
	__forceinline bool allgood() {
		return cb >= nmin;
	}
	__forceinline bool heading_outward() {
		return ex < 0;
	}
	__forceinline void start_block() {
		tls = cb;
		val = tls;
	}
	__forceinline void inc_by() {
		tls += dx;
		val = tls;
	}
	__forceinline void inc_bx() {
		val += dy;
	}
	__forceinline int bval() {
		return val;
	}
};


template <typename FRAGMENT_PROCESSOR>
void hdrawTri(const irect r, const vec4& s1, const vec4& s2, const vec4& s3, FRAGMENT_PROCESSOR& fp)
{
	const int x1 = iround(16.0f * s1.x);
	const int x2 = iround(16.0f * s2.x);
	const int x3 = iround(16.0f * s3.x);

	const int y1 = iround(16.0f * s1.y);
	const int y2 = iround(16.0f * s2.y);
	const int y3 = iround(16.0f * s3.y);

	int minx = max((min(min(x1, x2), x3) + 0xf) >> 4, r.x0);
	int miny = max((min(min(y1, y2), y3) + 0xf) >> 4, r.y0);
	int maxx = min((max(max(x1, x2), x3) + 0xf) >> 4, r.x1);
	int maxy = min((max(max(y1, y2), y3) + 0xf) >> 4, r.y1);

	const int q = 8;
	minx &= ~(q - 1);
	miny &= ~(q - 1);

	std::array<sdEdge, 3> edges;
	edges[0].setup(x1, y1, x2, y2, minx, miny, q);
	edges[1].setup(x2, y2, x3, y3, minx, miny, q);
	edges[2].setup(x3, y3, x1, y1, minx, miny, q);

	const float scale = 1.0f / (edges[0].c + edges[1].c + edges[2].c);

	fp.goto_xy(minx, miny);

	int qstep = q;

#define REVERSE_DIRECTION	qstep = -qstep;for (auto& edge : edges) edge.reverse_x();fp.reverse_x();
#define ADVANCE_TILE x0+=qstep;for(auto& edge:edges)edge.inc_x();fp.inc_x();

	REVERSE_DIRECTION
	int x0 = minx;
	for (int y0 = miny; y0 < maxy; y0 += q) {

		// new block line -- keep hunting for tri outer edge in old block line dir
		while (x0 >= minx && x0 <maxx && edges[0].good() && edges[1].good() && edges[2].good()) {
			ADVANCE_TILE
		}

		// Okay, we are now in a block we know is outside. Reverse direction and go into main loop.
		REVERSE_DIRECTION

		while (1) {

			ADVANCE_TILE

			if (x0 < minx || x0 >= maxx) break;
			if (edges[0].outside()) if (edges[0].heading_outward()) break; else continue;
			if (edges[1].outside()) if (edges[1].heading_outward()) break; else continue;
			if (edges[2].outside()) if (edges[2].heading_outward()) break; else continue;

			// tile test here
			const bool fully_covered = (edges[0].allgood() && edges[1].allgood() && edges[2].allgood());

			if (fully_covered) {
				fp.start_block(x0, y0);
				for (auto& edge : edges) edge.start_block();
				for (int iy = 0; iy < q; iy++) {
					for (int ix = 0; ix < q; ix++) {

						float BS[3];
						BS[0] = edges[1].bval() * scale;
						BS[2] = edges[0].bval() * scale;
						BS[1] = 1 - BS[2] - BS[0];
						fp.render(x0 + ix, y0 + iy, BS);

						for (auto& edge : edges) edge.inc_bx();
						fp.inc_bx();
					}
					for (auto& edge : edges) edge.inc_by();
					fp.inc_by();
				}
			} else {

				fp.start_block(x0, y0);
				for (auto& edge : edges) edge.start_block();
				for (int iy = 0; iy < q; iy++) {
					for (int ix = 0; ix < q; ix++) {
						if ((edges[0].bval() | edges[1].bval() | edges[2].bval()) >= 0) {

							float BS[3];
							BS[0] = edges[1].bval() * scale;
							BS[2] = edges[0].bval() * scale;
							BS[1] = 1 - BS[2] - BS[0];
							fp.render(x0 + ix, y0 + iy, BS);

						}
						for (auto& edge : edges) edge.inc_bx();
						fp.inc_bx();
					}
					for (auto& edge : edges) edge.inc_by();
					fp.inc_by();
				}
			}
		}

		// advance to next row of blocks
		for (auto& edge : edges) edge.inc_y();
		fp.inc_y();
	}
}




struct sdFlatShader {

	int offs, offs_left_start, offs_inc;

	vec4 face_color;
	float vert_invw[3];
	float vert_depth[3];

	FloatingPointPixel *cb, *cbx;
	float *db, *dbx;

	vec4 targetsize[2];

	int width, height;

	__m128 old_red, old_green, old_blue;

	void setColorBuffer(FloatingPointPixel * buf) {
		cb = buf;
	}

	void setDepthBuffer(float * buf) {
		db = buf;
	}

	void setColor(const vec4& color) {
		face_color = color;
	}

	void setup(const int width, const int height, const vec4& s1, const vec4& s2, const vec4& s3) {

		this->width = width;
		this->height = height;

		vert_invw[0] = s1.w;
		vert_invw[1] = s2.w;
		vert_invw[2] = s3.w;

		vert_depth[0] = -s1.z;
		vert_depth[1] = -s2.z;
		vert_depth[2] = -s3.z;

		//one_over_half_width = vec4(1.0f) / vec4(width >> 1);
		//one_over_half_height = vec4(1.0f) / vec4(height >> 1);
		targetsize[0] = vec4(static_cast<float>(width));
		targetsize[1] = vec4(static_cast<float>(height));
	}

	__forceinline void goto_xy(int x, int y) { }
	__forceinline void inc_y() { }
	__forceinline void inc_x() { }
	__forceinline void reverse_x() { }

	__forceinline void start_block(int x, int y) {
		offs_left_start = y * width + x;
		offs = offs_left_start;
		cbx = &cb[offs];
		dbx = &db[offs];
	}
	__forceinline void inc_by() {
		offs_left_start += width;
		offs = offs_left_start;
		cbx = &cb[offs];
		dbx = &db[offs];
	}
	__forceinline void inc_bx() {
		cbx++;
		dbx++;
	}

	__forceinline vec4 colorproc(const vec4& o, const vec4& n) {
		return n;
		//return o+n;
	}

	__forceinline void colorout(const vec4& n) {
		//auto cbx = cb + offs;
		cbx->v = colorproc(cbx->v,n).v;
	}

	/*
	__forceinline void depthwrite(const vec4& old_depth, const vec4& new_depth, const ivec4& mask) {
		auto dbx = db + offs;
		selectbits(old_depth, new_depth, mask).store(dbx);
	}
	*/

	__forceinline void render(const int x, const int y, const float * __restrict const BS) {

		float frag_depth = BS[0]*vert_depth[0] + BS[1]*vert_depth[1] + BS[2]*vert_depth[2];

		//if (frag_depth >= *dbx) return;
		//if (frag_depth < *dbx) return;

		// restore perspective
		float frag_w = 1.0f / (BS[0]*vert_invw[0] + BS[1]*vert_invw[1] + BS[2]*vert_invw[2]);
		float BP[3];
		BP[0] = vert_invw[0] * BS[0] * frag_w;
		BP[1] = vert_invw[1] * BS[1] * frag_w;
		BP[2] = 1.0f - (BP[0] + BP[1]);

		float frag_coord[2] = { x + 0.5f, height - y - 0.5f };

		vec4 frag_color;
		fragment(frag_color, frag_coord, frag_depth, BS, BP);
		colorout(frag_color);
		//depthwrite(frag_depth);
	}

	virtual __forceinline void fragment(vec4& frag_color, const float * __restrict const frag_coord, float& frag_depth, const float * __restrict const BS, const float * __restrict const BP) {
		frag_color = face_color;
	}
};


struct sdShadedShader : sdFlatShader {

	vec4 vc1, vc2, vc3;

	void setColor(const vec4& c1, const vec4& c2, const vec4& c3) {
		vc1 = c1;
		vc2 = c2;
		vc3 = c3;
	}

	virtual __forceinline void fragment(vec4& frag_color, const float * __restrict const frag_coord, float& frag_depth, const float * __restrict const BS, const float * __restrict const BP) {
		frag_color = BS[0] * vc1 + BS[1] * vc2 + BS[2] * vc3;
	}
};
#endif //__TRI_SD_H
