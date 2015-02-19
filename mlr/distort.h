
#ifndef __DISTORT_H
#define __DISTORT_H

template <typename TEXTURE_UNIT>
class DistortShader {
public:
	int offs, offs_left_start, offs_inc;
	int width;
	int height;
	const TEXTURE_UNIT &texunit;

	void setColorBuffer(SOAPixel * buf) {
		cb = buf;
	}
	SOAPixel * __restrict cb;

	void setup(const int width, const int height) {
		this->width = width;
		this->height = height;
	}

	void setParam(const int i, const float v) {
		if (i==0) {
			t = vec4(v);
		} else if (i==1) {
			center_coord.v[0] = vec4(v);
		} else if (i==2) {
			center_coord.v[1] = vec4(v);
		} else if (i==3) {
			deg2rad = vec4(PI*2/v);
		} else if (i == 4) {
			alpha = vec4(v);
		} else if (i == 5) {
			scale = vec4(v);
		} else if (i == 6) {
			pos.v[0] = vec4(v);
		} else if (i == 7) {
			pos.v[1] = vec4(v);
		}
	}

	DistortShader(const TEXTURE_UNIT& tu) :texunit(tu) {}

	__forceinline void goto_xy(int x, int y) {
		offs_left_start = (y >> 1)*(width >> 1) + (x >> 1);
		offs = offs_left_start;
	}
	__forceinline void inc_y() {
		offs_left_start += width >> 1;
		offs = offs_left_start;
	}
	__forceinline void inc_x() {
		offs++; // += offs_inc;
	}

	__forceinline qfloat colorproc(const qfloat& o, const qfloat& n, const qfloat& alpha) {
		return lerp_fast(o,n,alpha);
		//return n;
	}
	__forceinline void colorout(const qfloat4& n) {
		auto cbx = cb+offs;
		colorproc(vec4::load(&cbx->r), n.v[0], alpha).store(&cbx->r);
		colorproc(vec4::load(&cbx->g), n.v[1], alpha).store(&cbx->g);
		colorproc(vec4::load(&cbx->b), n.v[2], alpha).store(&cbx->b);
	}

	virtual __forceinline void render(const qfloat2& frag_coord) {
		qfloat2 screensize = { vec4(width), -vec4(width) };
		qfloat2 adj_coord = frag_coord / (screensize * scale) + pos;
		qfloat2 centervector = adj_coord - center_coord;
		qfloat distance = length(centervector)*256.0;
		qfloat2 dir = centervector / distance;
		qfloat factor = sin(distance*deg2rad+t) * (distance/1.0) + (distance/2);
		qfloat2 newpos = center_coord + dir * (abs(factor) + distance);

		qfloat4 frag_color;
		texunit.sample(newpos, frag_color);
		colorout(frag_color);
	}

private:
	qfloat t;
	qfloat2 center_coord;
	qfloat deg2rad;
	qfloat alpha;
	qfloat scale;
	qfloat2 pos;
};



template <typename TEXTURE_UNIT>
class OverlayShader {
public:
	int offs, offs_left_start, offs_inc;
	int width;
	int height;
	const TEXTURE_UNIT &texunit;

	void setColorBuffer(SOAPixel * buf) {
		cb = buf;
	}
	SOAPixel * __restrict cb;

	void setup(const int width, const int height, const int imgwidth, const int imgheight) {
		this->width = width;
		this->height = height;
		float scaled_width( (height*imgwidth)/float(imgheight) );
		float scaled_height(height); // (width*imgheight) / float(imgwidth) );
		auto wscale = vec4( float(width) / scaled_width );
		auto hscale = vec4( float(height) / scaled_height );
		imgscale = { wscale, hscale };
	}

	void setParam(const int i, const float v) {
/*		if (i==0) {
			t = vec4(v);
		} else if (i==1) {
			center_coord.v[0] = vec4(v);
		}*/
	}

	OverlayShader(const TEXTURE_UNIT& tu) :texunit(tu) {}

	__forceinline void goto_xy(int x, int y) {
		offs_left_start = (y >> 1)*(width >> 1) + (x >> 1);
		offs = offs_left_start;
	}
	__forceinline void inc_y() {
		offs_left_start += width >> 1;
		offs = offs_left_start;
	}
	__forceinline void inc_x() {
		offs++; // += offs_inc;
	}

	__forceinline qfloat colorproc(const qfloat& o, const qfloat& n, const qfloat& alpha) {
		return o * (vec4(1.0)-alpha) + n;
		//return lerp_fast(o,n,alpha);
		//return n;
	}
	__forceinline void colorout(const qfloat4& n) {
		auto cbx = cb+offs;
		colorproc(vec4::load(&cbx->r), n.v[0], n.v[3]).store(&cbx->r);
		colorproc(vec4::load(&cbx->g), n.v[1], n.v[3]).store(&cbx->g);
		colorproc(vec4::load(&cbx->b), n.v[2], n.v[3]).store(&cbx->b);
	}

	virtual __forceinline void render(const qfloat2& frag_coord) {
		qfloat2 screensize = { vec4(width), -vec4(height) };
		qfloat2 adj_coord = frag_coord / screensize;
		qfloat2 img_coord = adj_coord * imgscale;

		qfloat4 frag_color;
		texunit.sample(img_coord, frag_color);
		colorout(frag_color);
	}

private:
//	qfloat t;
//	qfloat2 center_coord;
	qfloat2 imgscale;
};

#endif //__DISTORT_H

