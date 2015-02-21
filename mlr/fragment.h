
#ifndef __FRAGMENT_H
#define __FRAGMENT_H

#include "stdafx.h"

#include "vec_soa.h"
#include "canvas.h"

class FlatShader {
public:
	int offs, offs_left_start, offs_inc;

	qfloat3 face_color;
	vertex_float vert_invw;
	vertex_float vert_depth;

	SOAPixel * __restrict cb;
	SOAPixel * __restrict cbx;
	__m128 * __restrict db;
	__m128 * __restrict dbx;
	vec4 targetsize[2];

	int width, height;

	__m128 old_red, old_green, old_blue;

	void setColorBuffer(SOAPixel * buf) {
		cb = buf;
	}

	void setDepthBuffer(__m128 * buf) {
		db = buf;
	}

	void setColor(const vec4& color) {
		face_color.set(color);
	}

	void setup(const int width, const int height, const vec4& s1, const vec4& s2, const vec4& s3) {

		this->width = width;
		this->height = height;

		vert_invw.set(s1.wwww(), s2.wwww(), s3.wwww());

		vert_depth.set(
			(-s1.zzzz() + vec4(1)) * vec4(0.5f),
			(-s2.zzzz() + vec4(1)) * vec4(0.5f),
			(-s3.zzzz() + vec4(1)) * vec4(0.5f)
		);

		qfloat2 targetsize = {
			vec4(static_cast<float>(width)),
			vec4(static_cast<float>(height))
		};
	}

	__forceinline void goto_xy(int x, int y) {
		offs_left_start = (y >> 1)*(width >> 1) + (x >> 1);
		offs = offs_left_start;
//		offs_inc = 1;
	}

	__forceinline void inc_y() {
		offs_left_start += width >> 1;
		offs = offs_left_start;
//		offs += width >> 1;
//		offs_inc = -offs_inc;
	}

	__forceinline void inc_x() {
		offs++; // += offs_inc;
	}

	virtual __forceinline vec4 colorproc(const vec4& o, const vec4& n, const vec4& alpha, const ivec4& mask) const {
		return selectbits(o, n, mask);
		//return o + (n &bits2float(mask));
		//return lerp_premul(o, n, selectbits(vec4(0),alpha,mask))
	}

	virtual __forceinline void colorout(const qfloat4& n, const ivec4& mask) const {
		auto cbx = cb+offs;
		colorproc(vec4::load(&cbx->r), n.v[0], n.v[3], mask).store(&cbx->r);
		colorproc(vec4::load(&cbx->g), n.v[1], n.v[3], mask).store(&cbx->g);
		colorproc(vec4::load(&cbx->b), n.v[2], n.v[3], mask).store(&cbx->b);
	}

	__forceinline void depthwrite(const qfloat& old_depth, const qfloat& new_depth, const ivec4& mask) {
		auto dbx = db + offs;
		selectbits(old_depth, new_depth, mask).store(dbx);
	}

	virtual __forceinline void render(const qfloat2& frag_coord, const ivec4& trimask, const vertex_float& BS) {

		qfloat frag_depth = vertex_blend(BS, vert_depth);

		// read depth buffer
		qfloat old_depth(vec4::load(db + offs));
		ivec4 depthmask = float2bits(cmpge(frag_depth, old_depth));
		ivec4 frag_mask = andnot(trimask, depthmask);
//		ivec4 frag_mask = andnot(trimask, ivec4(-1,-1,-1,-1)); // depthmask);

		//if (movemask(bits2float(frag_mask)) == 0) return; // early out
		
		// restore perspective
		qfloat frag_w = vec4(1.0f) / vertex_blend(BS, vert_invw);
		vertex_float BP;
		BP.x[0] = vert_invw.x[0] * BS.x[0] * frag_w;
		BP.x[1] = vert_invw.x[1] * BS.x[1] * frag_w;
		BP.x[2] = vec4(1.0f) - (BP.x[0] + BP.x[1]);

		qfloat4 frag_color;
		fragment(frag_color, frag_mask, frag_coord, frag_depth, BS, BP);
		colorout(frag_color, frag_mask);
		depthwrite(old_depth, frag_depth, frag_mask);
	}

	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, const qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) const {
		qfloat3 color3 = face_color * frag_depth;
		frag_color.set(color3);
//		frag_color.set(face_color);
	}
};

class DepthOnly : public FlatShader {
public:
	virtual __forceinline void render(const int x, const int y, const ivec4& trimask, const vertex_float& BS) const {
		qfloat frag_depth = vertex_blend(BS, vert_depth);
		qfloat old_depth(vec4::load(db + offs));
		qfloat new_depth = vmax(frag_depth, old_depth);
		new_depth.store(db + offs);
	}
};

class WireShader : public FlatShader {
public:
	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, const qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) const {
		static const qfloat grey(0.5f);
		const qfloat e = edgefactor(BS);
		frag_color.v[0] = frag_color.v[1] = frag_color.v[2] = mix(grey, vec4::zero(), e) * frag_depth * vec4(4.0f);

	}
private:
	__forceinline qfloat edgefactor(const vertex_float& BS) const {
		static const qfloat thickfactor(1.5f);
		qfloat3 d = fwidth(BS);
		qfloat3 a3 = smoothstep_zero(d*thickfactor, BS);
		return vmin(a3.v[0], vmin(a3.v[1], a3.v[2]));
	}
};


class ShadedShader : public FlatShader {
public:
	void setColor(const vec4& c1, const vec4& c2, const vec4& c3) {
		color.fill(c1, c2, c3);
	}

	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, const qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) const {
		qfloat3 color3 = vertex_blend(BS, color);
		frag_color.set(color3);
	}
private:
	vertex_float3 color;
};


__forceinline void fpp_gather(const FloatingPointPixel * const __restrict src, const ivec4& offsets, vec4 * const __restrict px)
{
	px[0].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.x]));
	px[1].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.y]));
	px[2].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.z]));
	px[3].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.w]));
	_MM_TRANSPOSE4_PS(px[0].v, px[1].v, px[2].v, px[3].v);
}


template<int pow>
//__forceinline void mipcalc(const float u0, const float u1, int& offset, int& mip_size, int& stride)
//__forceinline void mipcalc(const float dux, const float dvx, int* offset, int* mip_size, int* stride)
__forceinline void mipcalc(const float dux, const float dvx, const float duy, const float dvy, int* offset, int* mip_size, int* stride)
{
	const int image_xy = 1 << pow; // dimension of root image
	const int mipmap_height = 2 << pow; // total rows in mipmap
	const int max_lod = pow;

	const float v = fabs(dux*dvy - duy*dvx);
	const int fast_lod = ( ((int &)v) - (127 << 23)) >> 24;
	const int lod = std::min(pow, std::max(fast_lod, 0)); // must clamp to 0...max_lod

	const int inverted_lod = max_lod - lod;

	// calculate offset for the first row by subtracting from the end
	const int row = mipmap_height - (2 << inverted_lod);

	*stride = image_xy;
	*offset = row * (*stride);
	*mip_size = 1 << (pow - lod);
}


template<int power>
struct ts_pow2_mipmap {

	const FloatingPointPixel * __restrict texdata;
	const float fstride;

	ts_pow2_mipmap(const FloatingPointPixel * const __restrict ptr) :texdata(ptr), fstride(float(1<<power)) {}

	__forceinline void fetch_texel(const int rowoffset, const int mipsize, const ivec4& x, const ivec4& y, vec4 * const __restrict px) const
	{
		const auto texmod = ivec4(mipsize - 1);

		auto tx = ivec4(x & texmod);
		auto ty = ivec4(texmod - (y & texmod));

		auto offset = ivec4(shl<power>(ty) | tx) + ivec4(rowoffset);

		px[0].v = _mm_load_ps((float*)&texdata[offset.x]);
		px[1].v = _mm_load_ps((float*)&texdata[offset.y]);
		px[2].v = _mm_load_ps((float*)&texdata[offset.z]);
		px[3].v = _mm_load_ps((float*)&texdata[offset.w]);
		_MM_TRANSPOSE4_PS(px[0].v, px[1].v, px[2].v, px[3].v);
	}

	__forceinline void sample(const qfloat2& uv, qfloat4& px) const
	{
		int offset, mip_size, stride;
		float dux = (uv.v[0].y - uv.v[0].x)*fstride;
		float duy = (uv.v[0].z - uv.v[0].x)*fstride;
		float dvx = (uv.v[1].y - uv.v[1].x)*fstride;
		float dvy = (uv.v[1].z - uv.v[1].x)*fstride;
		mipcalc<power>(dux, dvx, duy, dvy, &offset, &mip_size, &stride);

		const vec4 texsize(itof(ivec4(mip_size)));

		// scale normalized coords to texture coords
		vec4 up(uv.v[0] * texsize);
		vec4 vp(uv.v[1] * texsize);

		// floor()
		ivec4 tx0(ftoi(up));
		ivec4 ty0(ftoi(vp));
		ivec4 tx1(tx0 + ivec4(1));
		ivec4 ty1(ty0 + ivec4(1));

		// get fractional parts
		const vec4 fx(up - itof(tx0));
		const vec4 fy(vp - itof(ty0));
		const vec4 fx1(vec4(1.0f) - fx);
		const vec4 fy1(vec4(1.0f) - fy);

		// calculate weights
		const vec4 w1(fx1 * fy1);
		const vec4 w2(fx  * fy1);
		const vec4 w3(fx1 * fy);
		const vec4 w4(fx  * fy);

		// temporary load p1 AoS data by lane, then transpose to make SoA rrrr/gggg/bbbb/aaaa
		vec4 p1[4]; fetch_texel(offset, mip_size, tx0, ty0, p1);
		vec4 p2[4]; fetch_texel(offset, mip_size, tx1, ty0, p2);
		vec4 p3[4]; fetch_texel(offset, mip_size, tx0, ty1, p3);
		vec4 p4[4]; fetch_texel(offset, mip_size, tx1, ty1, p4);

		px.v[0] = p1[0] * w1 + p2[0] * w2 + p3[0] * w3 + p4[0] * w4;
		px.v[1] = p1[1] * w1 + p2[1] * w2 + p3[1] * w3 + p4[1] * w4;
		px.v[2] = p1[2] * w1 + p2[2] * w2 + p3[2] * w3 + p4[2] * w4;
		px.v[3] = p1[3] * w1 + p2[3] * w2 + p3[3] * w3 + p4[3] * w4;
	}

};


template<int power>
struct ts_pow2_mipmap_nearest {

	const FloatingPointPixel * __restrict texdata;
	const float fstride;

	ts_pow2_mipmap_nearest(const FloatingPointPixel * const __restrict ptr) :texdata(ptr), fstride(float(1<<power)) {}

	__forceinline void fetch_texel(const int rowoffset, const int mipsize, const ivec4& x, const ivec4& y, vec4 * const __restrict px) const
	{
		const auto texmod = ivec4(mipsize - 1);

		auto tx = ivec4(x & texmod);
		auto ty = ivec4(texmod - (y & texmod));

		auto offset = ivec4(shl<power>(ty) | tx) + ivec4(rowoffset);

		px[0].v = _mm_load_ps((float*)&texdata[offset.x]);
		px[1].v = _mm_load_ps((float*)&texdata[offset.y]);
		px[2].v = _mm_load_ps((float*)&texdata[offset.z]);
		px[3].v = _mm_load_ps((float*)&texdata[offset.w]);
		_MM_TRANSPOSE4_PS(px[0].v, px[1].v, px[2].v, px[3].v);
	}

	__forceinline void sample(const qfloat2& uv, qfloat4& px) const
	{
		int offset, mip_size, stride;
		float dux = (uv.v[0].y - uv.v[0].x)*fstride;
		float duy = (uv.v[0].z - uv.v[0].x)*fstride;
		float dvx = (uv.v[1].y - uv.v[1].x)*fstride;
		float dvy = (uv.v[1].z - uv.v[1].x)*fstride;
		mipcalc<power>(dux, dvx, duy, dvy, &offset, &mip_size, &stride);

		const vec4 texsize(itof(ivec4(mip_size)));

		// scale normalized coords to texture coords
		vec4 up(uv.v[0] * texsize);
		vec4 vp(uv.v[1] * texsize);

		// temporary load p1 AoS data by lane, then transpose to make SoA rrrr/gggg/bbbb/aaaa
		fetch_texel(offset, mip_size, ftoi(up), ftoi(vp), px.v);
	}

};


template<int power>
struct ts_pow2_direct_nearest {

	const FloatingPointPixel * __restrict texdata;
	const float fstride;
	const ivec4 texmod;

	ts_pow2_direct_nearest(const FloatingPointPixel * const __restrict ptr) :texdata(ptr), fstride(float(1<<power)), texmod((2<<power)-1) {}

	__forceinline void fetch_texel(const ivec4& x, const ivec4& y, vec4 * const __restrict px) const
	{
		auto tx = ivec4(x & texmod);
		auto ty = ivec4(texmod - (y & texmod));

		auto offset = ivec4(shl<power>(ty) | tx);

		px[0].v = _mm_load_ps((float*)&texdata[offset.x]);
		px[1].v = _mm_load_ps((float*)&texdata[offset.y]);
		px[2].v = _mm_load_ps((float*)&texdata[offset.z]);
		px[3].v = _mm_load_ps((float*)&texdata[offset.w]);
		_MM_TRANSPOSE4_PS(px[0].v, px[1].v, px[2].v, px[3].v);
	}

	__forceinline void sample(const qfloat2& uv, qfloat4& px) const
	{
		vec4 up(uv.v[0] * fstride);
		vec4 vp(uv.v[1] * fstride);
		fetch_texel(ftoi(up), ftoi(vp), px.v);
	}

};



struct ts_any_direct_nearest {

	const FloatingPointPixel * __restrict texdata;
	const float fw, fh;
	const int width;
	const int height;

	ts_any_direct_nearest(const FloatingPointPixel * const __restrict ptr, const int width, const int height)
		:texdata(ptr),
		fw(float(width)),
		fh(float(height)),
		height(height),
		width(width)
	{}

	__forceinline void fetch_texel(const ivec4& x, const ivec4& y, vec4 * const __restrict px) const
	{
		for (int i = 0; i < 4; i++) {
			auto tx = x.v.m128i_i32[i];
			auto ty = y.v.m128i_i32[i];

			if (tx>=0 && tx<width && ty>=0 && ty<height) {
				const int offset = ty*width + tx;
				px[i] = vec4::load(reinterpret_cast<const __m128*>(&texdata[offset]));
			} else {
				px[i] = vec4::zero();
			}
		}
		_MM_TRANSPOSE4_PS(px[0].v, px[1].v, px[2].v, px[3].v);
	}

	__forceinline void sample(const qfloat2& uv, qfloat4& px) const
	{
		vec4 up(uv.v[0] * fw);
		vec4 vp(uv.v[1] * -fh);
		fetch_texel(ftoi(up), ftoi(vp), px.v);
	}

};


template <typename TEXTURE_UNIT>
class TextureShader : public FlatShader {
public:
	const TEXTURE_UNIT & texunit;
	vertex_float2 vert_uv;
	inline TextureShader(const TEXTURE_UNIT& tu) :texunit(tu){}

	void setUV(const vec4& c1, const vec4& c2, const vec4& c3) {
		vert_uv.fill(c1, c2, c3);
	}

	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, const qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) const {
		qfloat2 frag_uv = vertex_blend(BP, vert_uv);

		qfloat4 texpx;
		texunit.sample(frag_uv, texpx);
		frag_color = texpx; // .set(texpx);
	}
};


template <typename TEXTURE_UNIT>
class TextureShaderAlpha : public FlatShader {
public:
	const TEXTURE_UNIT & texunit;
	vertex_float2 vert_uv;
	inline TextureShaderAlpha(const TEXTURE_UNIT& tu) :texunit(tu){}

	void setUV(const vec4& c1, const vec4& c2, const vec4& c3) {
		vert_uv.fill(c1, c2, c3);
	}

	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, const qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) const {
		qfloat2 frag_uv = vertex_blend(BP, vert_uv);

		qfloat4 texpx;
		texunit.sample(frag_uv, texpx);
		frag_color = texpx; // .set(texpx);
	}
	virtual __forceinline vec4 colorproc(const vec4& o, const vec4& n, const vec4& alpha, const ivec4& mask) const {
		//return selectbits(o, n, mask);
		//return o + (n &bits2float(mask));
		//return lerp_premul(o, n, alpha);
		return selectbits(o, lerp_premul(o, n, alpha), mask);
	}

	virtual __forceinline void colorout(const qfloat4& n, const ivec4& mask) const {
		auto cbx = cb+offs;
		colorproc(vec4::load(&cbx->r), n.v[0], n.v[3], mask).store(&cbx->r);
		colorproc(vec4::load(&cbx->g), n.v[1], n.v[3], mask).store(&cbx->g);
		colorproc(vec4::load(&cbx->b), n.v[2], n.v[3], mask).store(&cbx->b);
	}
};


template <typename TEXTURE_UNIT>
class TextureShaderAlphaNoZ : public FlatShader {
public:
	const TEXTURE_UNIT & texunit;
	vertex_float2 vert_uv;
	inline TextureShaderAlpha(const TEXTURE_UNIT& tu) :texunit(tu){}

	void setUV(const vec4& c1, const vec4& c2, const vec4& c3) {
		vert_uv.fill(c1, c2, c3);
	}

	virtual __forceinline void render(const qfloat2& frag_coord, const ivec4& trimask, const vertex_float& BS) {

		qfloat frag_depth = vertex_blend(BS, vert_depth);
		ivec4 frag_mask = andnot(trimask, ivec4(-1,-1,-1,-1)); // depthmask);

		// restore perspective
		qfloat frag_w = vec4(1.0f) / vertex_blend(BS, vert_invw);
		vertex_float BP;
		BP.x[0] = vert_invw.x[0] * BS.x[0] * frag_w;
		BP.x[1] = vert_invw.x[1] * BS.x[1] * frag_w;
		BP.x[2] = vec4(1.0f) - (BP.x[0] + BP.x[1]);

		qfloat4 frag_color;
		fragment(frag_color, frag_mask, frag_coord, frag_depth, BS, BP);
		colorout(frag_color, frag_mask);
	}

	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, const qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) const {
		qfloat2 frag_uv = vertex_blend(BP, vert_uv);

		qfloat4 texpx;
		texunit.sample(frag_uv, texpx);
		frag_color = texpx; // .set(texpx);
	}
	virtual __forceinline vec4 colorproc(const vec4& o, const vec4& n, const vec4& alpha, const ivec4& mask) const {
		//return selectbits(o, n, mask);
		//return o + (n &bits2float(mask));
		//return lerp_premul(o, n, alpha);
		return selectbits(o, lerp_premul(o, n, alpha), mask);
	}

	virtual __forceinline void colorout(const qfloat4& n, const ivec4& mask) const {
		auto cbx = cb+offs;
		colorproc(vec4::load(&cbx->r), n.v[0], n.v[3], mask).store(&cbx->r);
		colorproc(vec4::load(&cbx->g), n.v[1], n.v[3], mask).store(&cbx->g);
		colorproc(vec4::load(&cbx->b), n.v[2], n.v[3], mask).store(&cbx->b);
	}
};

#endif //__FRAGMENT_H
