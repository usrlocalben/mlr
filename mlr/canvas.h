
#ifndef __CANVAS_H
#define __CANVAS_H

#include "stdafx.h"

#include "aligned_allocator.h"
#include "PixelToaster.h"
#include "ryg_srgb.h"

#include "vec.h"

using namespace PixelToaster;

__declspec(align(16)) struct SOAPixel {
	__m128 r, g, b, a;
};


struct SOACanvas {
	vectorsse<SOAPixel> b; // color buffer
	int width;
	int height;
	int stride; // two rows !!!

	SOAPixel* rawptr() { return &b[0]; }

	void setup(const int x, const int y) {
		b.resize((x >> 1)*(y >> 1));
		width = x;
		height = y;
		stride = x >> 1;
	}

	void clear(const irect area) {
		int x0 = area.x0 >> 1;
		int y0 = area.y0 >> 1;
		int x1 = area.x1 >> 1;
		int y1 = area.y1 >> 1;
		int start_offset = y0 * stride + x0;
		SOAPixel * __restrict dst = &b[start_offset];
		for (int i = 0; i < y1 - y0; i++) {
			memset(dst, 0, (x1-x0)*sizeof(SOAPixel));
			dst += stride;
		}
	}

	void putpixel(int x, int y, const FloatingPointPixel& px)
	{
		SOAPixel* c = &b[ (y >> 1)*(width >> 1) + (x >> 1) ];
		vec4* cr = reinterpret_cast<vec4*>(&c->r);
		vec4* cg = reinterpret_cast<vec4*>(&c->g);
		vec4* cb = reinterpret_cast<vec4*>(&c->b);
		vec4* ca = reinterpret_cast<vec4*>(&c->a);
		int sx = x & 1;
		int sy = y & 1;
		if (sy == 0) {
			if (sx == 0) {
				cr->x = px.r;
				cg->x = px.g;
				cb->x = px.b;
				ca->x = px.a;
			} else {
				cr->y = px.r;
				cg->y = px.g;
				cb->y = px.b;
				ca->y = px.a;
			}
		} else {
			if (sx == 0) {
				cr->z = px.r;
				cg->z = px.g;
				cb->z = px.b;
				ca->z = px.a;
			} else {
				cr->w = px.r;
				cg->w = px.g;
				cb->w = px.b;
				ca->w = px.a;
			}
		}
	}
};


struct FPPCanvas {

	vectorsse<FloatingPointPixel> b;
	int width;
	int height;
	int stride;

	void setup(int x, int y) {
		b.resize(x*y);
		width = x;
		height = y;
		stride = x;
	}
};


struct SOACursor {

	int offs;
	int offsrow;
	int width;

	SOACursor(int width) :width(width) {}
	void set_xy(const int x, const int y) {
		offsrow = offs = (y >> 1) * (width >> 1) + (x >> 1);
	}
	void inc_x() {
		offsrow++;
	}
	void inc_y() {
		offs += (width >> 1);
		offsrow = offs;
	}
	const int ofs() {
		return offsrow;
	}
};


struct SOADepth {
	vectorsse<__m128> b;
	int width;
	int height;
	int stride;

	__m128* rawptr() { return &b[0]; }

	void setup(const int x, const int y) {
		b.resize((x >> 1)*(y >> 1));
		width = x;
		height = y;
		stride = x >> 1;
	}

	void clear(const irect area) {
		int x0 = area.x0 >> 1;
		int y0 = area.y0 >> 1;
		int x1 = area.x1 >> 1;
		int y1 = area.y1 >> 1;
		int start_offset = y0 * stride + x0;
		__m128 * __restrict dst = &b[start_offset];
		for (int i = 0; i < y1 - y0; i++) {
			memset(dst, 0, (x1-x0)*sizeof(__m128));
			dst += stride;
		}
	}
};


struct FPDepth {
	vectorsse<float> b;
	int width;
	int height;
	void setup(int x, int y) {
		b.resize(x*y);
		width = x;
		height = y;
	}
};


template <typename POSTPROCESS>
void convertCanvas(
	const irect r,
	const int width,
	TrueColorPixel * const __restrict tb,
	SOAPixel * const __restrict sb,
	POSTPROCESS& pp
)
{
	for (int y = r.y0; y < r.y1; y += 2) {

		auto* dst1 = &tb[y*width + r.x0];
		auto* dst2 = dst1 + width;
		auto* src = &sb[(y >> 1) * (width >> 1) + (r.x0 >> 1)];

		for (int x = r.x0; x < r.x1; x += 4) {
			const ivec4 packed1 =
				shl<16>(float_to_srgb8_var2_SSE2(pp.proc(src->r).v)) |
				shl< 8>(float_to_srgb8_var2_SSE2(pp.proc(src->g).v)) |
				        float_to_srgb8_var2_SSE2(pp.proc(src->b).v);
			src++;
			const ivec4 packed2 =
				shl<16>(float_to_srgb8_var2_SSE2(pp.proc(src->r).v)) |
				shl< 8>(float_to_srgb8_var2_SSE2(pp.proc(src->g).v)) |
				        float_to_srgb8_var2_SSE2(pp.proc(src->b).v);
			src++;

			_mm_stream_si128(reinterpret_cast<__m128i*>(dst1), _mm_unpacklo_epi64(packed1.v, packed2.v));
			_mm_stream_si128(reinterpret_cast<__m128i*>(dst2), _mm_unpackhi_epi64(packed1.v, packed2.v));

			dst1 += 4;
			dst2 += 4;
		}
	}
}


struct PostprocessNoop {
	__forceinline vec4 proc(const vec4& a) { return a; }
};

#endif //__CANVAS_H
