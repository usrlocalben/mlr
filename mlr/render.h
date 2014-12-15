
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
	Binner(){
		device_width = 0;
		device_height = 0;
	}
	std::vector<irect> binrects;
	std::vector<Tilebin> bins;
//	std::vector<int> binidx;
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


class Pipeline {
public:
//	Pipeline()struct Viewport& vp) :vp(vp){}

//	void addMesh(const MeshInstance& mi);
	void addMeshy(Meshy& mi);
	void addLight(const Light& li);
	void setCamera(const mat4& camera);
	//	void setViewport(const class Viewport& viewport);

	void reset(const int width, const int height) {
		vlst.clear();
		tlst.clear();
		nlst.clear();
		llst.clear();
		flst.clear();
		batch_in_progress = 0;
		binner.reset(width, height);
		framecounter++;
		// projected = true; // ???
		// rasterstackidx = 0 ??
	}
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

	/*
	void allocate_verticies(const unsigned cnt) {
	for (unsigned i = 0; i < cnt; i++) {
	PVertex&
	}
	if
	}
	*/


	void addFace(const Face& fsrc);
	void addVertex(const vec4& src, const mat4& m);
	void addNormal(const vec4& src, const mat4& m);
	void addUV(const vec4& src);
	void setViewport(struct Viewport* new_viewport) { this->vp = new_viewport; }
	void addCamera(const mat4& cam) {
		camera = cam;
		camera_inverse = mat4_inverse(camera);
	}
	void render_tile(const int tile_id);
	void render(__m128 * __restrict db, SOAPixel * __restrict cb, class MaterialStore& materialstore, class TextureStore& texturestore);
	void render2(__m128 * __restrict db, SOAPixel * __restrict cb, class MaterialStore& materialstore, class TextureStore& texturestore, std::function<void()>& f);

private:
	vectorsse<PVertex> vlst;  unsigned vbase;    int new_vcnt;    int clipbase;
	vectorsse<vec4> nlst;     unsigned nbase;    int new_ncnt;
	vectorsse<vec4> tlst;     unsigned tbase;    int new_tcnt;
	vectorsse<Face> flst;     unsigned fbase;
	vectorsse<Light> llst;    unsigned lbase;

	mat4 camera;
	mat4 camera_inverse;

	int batch_in_progress;
	int framecounter;

//	struct Viewport& vp;
	struct Viewport* vp;
	Binner binner;


	void process_face(const int face_id);

};

#endif //__RENDER_H