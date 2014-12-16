
#ifndef __RENDER_H
#define __RENDER_H

#include "stdafx.h"

#include <functional>
#include <vector>

#include "aligned_allocator.h"

#include "vec.h"
#include "mesh.h"
#include "canvas.h"


struct PVertex {
	vec4 p; // eyespace
	vec4 c; // clipspace
	vec4 s; // screenspace
	vec4 f; // fixed 2d x,y,"z",1overw

	vec4 n; // vertex normal
	unsigned cf;
};

__forceinline PVertex lerp(const PVertex& a, const PVertex& b, const float t) {
	PVertex n;
	n.p = lerp(a.p, b.p, t);
	n.c = lerp(a.c, b.c, t);
	n.s = lerp(a.s, b.s, t);
	n.n = normalized(lerp(a.n, b.n, t));

	float one_over_w = 1 / n.s.w;
	n.f = n.s / n.s.wwww();
	n.f.w = one_over_w;

	/*
	cout << "----- PVertex lerp:" << endl;
	cout << "t: " << t << endl;
	cout << "p" << a.p << " -> " << n.p << " <- " << b.p << endl;
	cout << "c" << a.c << " -> " << n.c << " <- " << b.c << endl;
	cout << "s" << a.s << " -> " << n.s << " <- " << b.s << endl;
	cout << "f" << a.f << " -> " << n.f << " <- " << b.f << endl;
	cout << endl;
	*/
	return n;
}


struct Tilebin {
	irect rect;
	int id;
	std::vector<int> faces;
	void reset() {
		faces.clear();
	}
};

class Binner {
public:
	void reset(const int cur_width, const int cur_height);
	void insert(const vec4& p1, const vec4& p2, const vec4& p3, const int idx);
	void sort();
	void unsort();
	Binner() :device_height(0), device_width(0) {}
	std::vector<irect> binrects;
	std::vector<Tilebin> bins;
private:
	void onResize();
	int device_width;
	int device_height;
	int tilewidth;
	int tileheight;
	int device_width_in_tiles;
	int device_height_in_tiles;
	ivec4 device_max;
};


class Pipedata {
public:
	Pipedata(const int thread_number, const int thread_count);

	void addMeshy(Meshy& mi, const mat4& camera_inverse, const struct Viewport * const vp);

	void reset(const int width, const int height) {
		vlst.clear();
		tlst.clear();
		nlst.clear();
		llst.clear();
		flst.clear();
		batch_in_progress = 0;
		root_count = 0;
		binner.reset(width, height);
	}
	void addFace(const Face& fsrc);
	void addVertex(const struct Viewport * const vp, const vec4& src, const mat4& m);
	void addNormal(const vec4& src, const mat4& m);
	void addUV(const vec4& src);
	Binner binner;
	void render(__m128 * __restrict db, SOAPixel * __restrict cb, class MaterialStore& materialstore, class TextureStore& texturestore, const struct Viewport * const vp, const int bin_idx);

private:
	void begin_batch() {
		_ASSERT(batch_in_progress == 0);
		vbase = vlst.size();    new_vcnt = 0;
		tbase = tlst.size();    new_tcnt = 0;
		nbase = nlst.size();    new_ncnt = 0;
		batch_in_progress = 1;
	}
	//	void mark() {
	//		clipbase = vbase + new_vcnt;
	//	}
	void end_batch() {
		vbase = vlst.size();
		tbase = tlst.size();
		nbase = nlst.size();
		batch_in_progress = 0;
	}
	void process_face(const int face_id);

	vectorsse<PVertex> vlst;  unsigned vbase;    int new_vcnt;    int clipbase;
	vectorsse<vec4> nlst;     unsigned nbase;    int new_ncnt;
	vectorsse<vec4> tlst;     unsigned tbase;    int new_tcnt;
	vectorsse<Face> flst;     unsigned fbase;
	vectorsse<Light> llst;    unsigned lbase;
	int batch_in_progress;
	const int thread_number;
	const int thread_count;
	int root_count;
};


typedef std::pair<int, int> binstat;


class Pipeline {
public:
	Pipeline(const int threads);

	void addMeshy(Meshy& mi) {
		meshlist.push_back(&mi);
	}

	void addLight(const Light& li);
	void setViewport(struct Viewport* new_viewport) { this->vp = new_viewport; }

	void reset(const int width, const int height) {
		for (auto& item : pipes) {
			item.reset(width, height);
		}
		meshlist.clear();
		framecounter++;
	}

	void addCamera(const mat4& cam) {
		camera = cam;
		camera_inverse = mat4_inverse(camera);
	}
	void render(__m128 * __restrict db, SOAPixel * __restrict cb, class MaterialStore& materialstore, class TextureStore& texturestore, std::function<void(bool)>& f);

	void index_bins() {
		bin_index.clear();
		for (int bi = 0; bi < pipes[0].binner.bins.size(); bi++) {
			int ax = 0; 
			for (int ti = 0; ti < threads; ti++) {
				ax += pipes[ti].binner.bins[bi].faces.size();
			}
			bin_index.push_back(binstat(bi, ax));
		}
		sort(bin_index.begin(), bin_index.end(),
			[](const binstat& a, const binstat& b){ return a.second > b.second; });
	}

private:
	const int threads;
	vectorsse<Pipedata> pipes;
	std::vector<Meshy*> meshlist;

	mat4 camera;
	mat4 camera_inverse;

	int framecounter;
	struct Viewport* vp;
	std::vector<binstat> bin_index;

};

#endif //__RENDER_H
