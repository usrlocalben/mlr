
#include "stdafx.h"

#include "canvas.h"

using namespace PixelToaster;

void convertCanvas2(
	const irect r,
	const int width,
	const FloatingPointPixel* const __restrict sb,
	TrueColorPixel* const __restrict tb
)
{
	for (int y = r.y0; y < r.y1; y++) {

		int offs = y * width;

		for (int x = r.x0; x < r.x1; x++) {

			const FloatingPointPixel& src = sb[offs + x];
			TrueColorPixel& dst = tb[offs + x];

			ivec4 ready = float_to_srgb8_var2_SSE2(src.v);
			dst.r = ready.x;
			dst.g = ready.y;
			dst.b = ready.z;
		}
	}
}


void downsampleCanvas(
	const int width,
	const int height,
	const SOAPixel * const __restrict src,
	FloatingPointPixel* const __restrict dst
)
{
	FloatingPointPixel * __restrict dx = dst;
	const SOAPixel * __restrict sx = src;

	for (int y = 0; y < height >> 1; y++) {
		for (int x = 0; x < width >> 1; x++) {
			__m128 rg = _mm_hadd_ps(sx->r, sx->g);
			__m128 ba = _mm_hadd_ps(sx->b, sx->a);
			__m128 rgba = _mm_hadd_ps(rg, ba);
			dx->v = _mm_mul_ps(rgba, _mm_set1_ps(0.25f));
			sx++;
			dx++;
		}
	}
}
