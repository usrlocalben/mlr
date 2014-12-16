
#ifndef __TRI_H
#define __TRI_H

#include "stdafx.h"

#include <cmath>

#include "ryg_srgb.h"
#include "shadertools.h"
#include "canvas.h"
#include "vec.h"

using namespace PixelToaster;


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
	__forceinline void set(const vec4& v0, const vec4& v1, const vec4& v2) {
		x[0] = v0.xxxx();  y[0] = v0.yyyy();
		x[1] = v1.xxxx();  y[1] = v1.yyyy();
		x[2] = v2.xxxx();  y[2] = v2.yyyy();
	}
};
struct vertex_float3 {
	vec4 x[3];
	vec4 y[3];
	vec4 z[3];
	void set(const vec4& v0, const vec4& v1, const vec4& v2) {
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

		block_left_start = ivec4(c) + (ivec4(0, 1, 0, 1)*dy) + (ivec4(0, 0, 1, 1)*dx);
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
void drawTri( const irect r, const vec4& s1, const vec4& s2, const vec4& s3, FRAGMENT_PROCESSOR& fp )
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

			vertex_float bary;
			bary.x[0] = itof(e[1].val()) * scale;
			bary.x[2] = itof(e[0].val()) * scale;
			bary.x[1] = vec4(1.0f) - (bary.x[0] + bary.x[2]);

			fp.render(x, y, trimask, bary);
		}
	}
}




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

	__forceinline vec4 colorproc(const vec4& o, const vec4& n, const ivec4& mask) {
		return selectbits(o, n, mask);
		//return o + (n &bits2float(mask));
	}

	__forceinline void colorout(const qfloat4& n, const ivec4& mask) {
		auto cbx = cb+offs;
		colorproc(vec4::load(&cbx->r), n.v[0], mask).store(&cbx->r);
		colorproc(vec4::load(&cbx->g), n.v[1], mask).store(&cbx->g);
		colorproc(vec4::load(&cbx->b), n.v[2], mask).store(&cbx->b);
	}

	__forceinline void depthwrite(const qfloat& old_depth, const qfloat& new_depth, const ivec4& mask) {
		auto dbx = db + offs;
		selectbits(old_depth, new_depth, mask).store(dbx);
	}

	virtual __forceinline void render(const int x, const int y, const ivec4& trimask, const vertex_float& BS) {

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

		qfloat2 frag_coord = {
			vec4(x + 0.5f) + vec4(0, 1, 0, 1),
			vec4(height - y - 0.5f) + vec4(0, 0, -1, -1)
		};

		qfloat4 frag_color;
		fragment(frag_color, frag_mask, frag_coord, frag_depth, BS, BP);
		colorout(frag_color, frag_mask);
		depthwrite(old_depth, frag_depth, frag_mask);
	}

	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) {
		qfloat3 color3;
		color3 = qmul(face_color, frag_depth);
		frag_color.set(color3);
	}
};

class DepthOnly : public FlatShader {
public:
	virtual __forceinline void render(const int x, const int y, const ivec4& trimask, const vertex_float& BS) {
		qfloat frag_depth = vertex_blend(BS, vert_depth);
		qfloat old_depth(vec4::load(db + offs));
		qfloat new_depth = vmax(frag_depth, old_depth);
		new_depth.store(db + offs);
	}
};

class WireShader : public FlatShader {
public:
	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) {
		static const qfloat grey(0.5f);
		const qfloat e = edgefactor(BS);
		frag_color.v[0] = frag_color.v[1] = frag_color.v[2] = mix(grey, vec4::zero(), e) * frag_depth * vec4(4.0f);

	}
private:
	__forceinline qfloat edgefactor(const vertex_float& BS) {
		static const qfloat thickfactor(1.5f);
		qfloat3 d = fwidth(BS);
		qfloat3 a3 = smoothstep_zero(qmul(d, thickfactor), BS);
		return vmin(a3.v[0], vmin(a3.v[1], a3.v[2]));
	}
};


class ShadedShader : public FlatShader {
public:
	vertex_float3 color;

	void setColor(const vec4& c1, const vec4& c2, const vec4& c3) {
		color.set(c1, c2, c3);
	}

	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, const qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) {
		qfloat3 color3 = vertex_blend(BS, color);
		frag_color.set(color3);
	}
};


__forceinline void fpp_gather(const FloatingPointPixel * const __restrict src, const ivec4& offsets, vec4 * const __restrict px)
{
	px[0].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.x]));
	px[1].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.y]));
	px[2].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.z]));
	px[3].v = _mm_load_ps(reinterpret_cast<const float*>(&src[offsets.w]));
	transpose4(px);
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
	for ( int i=0; i<e; i++ )
		dst[i] = abs(ddx(s[i])) + abs(ddy(s[i]));
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


struct ts_512_mipmap {

	const FloatingPointPixel * __restrict texdata;

	ts_512_mipmap(const FloatingPointPixel * const __restrict ptr) :texdata(ptr) {}

	__forceinline void fetch_texel(const int rowoffset, const int mipsize, const ivec4& x, const ivec4& y, vec4 * const __restrict px) const
	{
		const auto texmod = ivec4(mipsize - 1);

		auto tx = ivec4(x & texmod);
		auto ty = ivec4(texmod - (y & texmod));

		auto offset = ivec4(shl<9>(ty) | tx) + ivec4(rowoffset);

		px[0].v = _mm_load_ps((float*)&texdata[offset.x]);
		px[1].v = _mm_load_ps((float*)&texdata[offset.y]);
		px[2].v = _mm_load_ps((float*)&texdata[offset.z]);
		px[3].v = _mm_load_ps((float*)&texdata[offset.w]);
	}

	__forceinline void sample(const qfloat2& uv, qfloat4& px) const
	{
		int offset, mip_size, stride;
		float dux = (uv.v[0].y - uv.v[0].x)*512.0f;
		float duy = (uv.v[0].z - uv.v[0].x)*512.0f;
		float dvx = (uv.v[1].y - uv.v[1].x)*512.0f;
		float dvy = (uv.v[1].z - uv.v[1].x)*512.0f;
		mipcalc<9>(dux, dvx, duy, dvy, &offset, &mip_size, &stride);

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
		vec4 p1[4]; fetch_texel(offset, mip_size, tx0, ty0, p1); transpose4(p1);
		vec4 p2[4]; fetch_texel(offset, mip_size, tx1, ty0, p2); transpose4(p2);
		vec4 p3[4]; fetch_texel(offset, mip_size, tx0, ty1, p3); transpose4(p3);
		vec4 p4[4]; fetch_texel(offset, mip_size, tx1, ty1, p4); transpose4(p4);

		px.v[0] = p1[0] * w1 + p2[0] * w2 + p3[0] * w3 + p4[0] * w4;
		px.v[1] = p1[1] * w1 + p2[1] * w2 + p3[1] * w3 + p4[1] * w4;
		px.v[2] = p1[2] * w1 + p2[2] * w2 + p3[2] * w3 + p4[2] * w4;
		px.v[3] = p1[3] * w1 + p2[3] * w2 + p3[3] * w3 + p4[3] * w4;
	}

};


template <typename TEXTURE_UNIT>
class TextureShader : public FlatShader {
public:
	const TEXTURE_UNIT & texunit;
	vertex_float2 vert_uv;
	inline TextureShader(const TEXTURE_UNIT& tu) :texunit(tu){}

	void setUV(const vec4& c1, const vec4& c2, const vec4& c3) {
		vert_uv.set(c1, c2, c3);
	}

	virtual __forceinline void fragment(qfloat4& frag_color, ivec4& frag_mask, const qfloat2& frag_coord, qfloat& frag_depth, const vertex_float& BS, const vertex_float& BP) {
		qfloat2 frag_uv = vertex_blend(BP, vert_uv);

		qfloat4 texpx;
		texunit.sample(frag_uv, texpx);
		frag_color = texpx; // .set(texpx);
	}
};


#endif //__TRI_H
