
#ifndef __RENDER_H
#define __RENDER_H

#include "stdafx.h"

#include <functional>
#include <atomic>
#include <thread>
#include <vector>

#include "aligned_allocator.h"

#include "vec.h"
#include "clip.h"
#include "mesh.h"
#include "canvas.h"
#include "meshops.h"
#include "viewport.h"


struct PVertex {
	unsigned cf;
	vec4 f; // fixed 2d x,y,"z",1overw
	vec4 p; // eyespace
	vec4 c; // clipspace

	vec4 n; // vertex normal

	void process(const Viewport& vp);
};



struct Tilebin {
	irect rect;
	int id;
	std::vector<PFace> faces;
	void reset() {
		faces.clear();
	}
};

class Binner {
public:
	void reset(const int cur_width, const int cur_height);
	void insert(const vec4& p1, const vec4& p2, const vec4& p3, const PFace& face);
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
	vec4 device_max;
};

struct ShadowMesh {
	const Mesh* mesh;
	int vbase;
	mat4 c2o; // cameraspace-to-objectspace
};


class Pipedata {
public:
	void setup(const int thread_number, const int thread_count);

	void addMeshy(Meshy& mi, const mat4& camera_inverse, const Viewport& vp);

	void reset(const int width, const int height) {
		vlst.clear();
		tlst.clear();
		nlst.clear();
		llst.clear();
		batch_in_progress = 0;
		root_count = 0;
		binner.reset(width, height);
		shadowqueue.clear();
	}
	void addFace(const Viewport& vp, const Face& fsrc);
	void addNormal(const vec4& src, const mat4& m);
	void addUV(const vec4& src);
	Binner binner;
	void render(__m128 * __restrict db, SOAPixel * __restrict cb, class MaterialStore& materialstore, class TextureStore& texturestore, const Viewport& vp, const int bin_idx);

	void addVertex(const Viewport& vp, const vec4& src, const mat4& m);
	PVertex clipcalc(const Viewport& vp, const PVertex& a, const PVertex& b, const float t);

	std::atomic<int> my_signal;
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
	void process_face(PFace& f);

	vectorsse<PVertex> vlst;  unsigned vbase;    int new_vcnt;    int clipbase;
	vectorsse<vec4> nlst;     unsigned nbase;    int new_ncnt;
	vectorsse<vec4> tlst;     unsigned tbase;    int new_tcnt;
	vectorsse<Light> llst;    unsigned lbase;
	int batch_in_progress;
	int thread_number;
	int thread_count;
	int root_count;

	vectorsse<ShadowMesh> shadowqueue;
};


typedef std::pair<int, int> binstat;


class Pipeline {
public:
	Pipeline(const int threads, class Telemetry& telemetry);
	~Pipeline();

	void addMeshy(Meshy& mi) {
		meshlist.push_back(&mi);
	}

	void addLight(const Light& li);
	void setViewport(const Viewport * const vp) { this->vp = vp; }

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
	void render();
	void render_thread(const int thread_number);
	void process_thread(const int thread_number);

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

	void setDepthbuffer(struct SOADepth& db) {
		this->db = &db;
	}
	void setColorbuffer(struct SOACanvas& cb) {
		this->cb = &cb;
	}
	void setMaterialStore(class MaterialStore& materialstore) {
		this->materialstore = &materialstore;
	}
	void setTextureStore(class TextureStore& texturestore) {
		this->texturestore = &texturestore;
	}
	void setTarget(TrueColorPixel * const __restrict target, const int target_width) {
		this->target = target;
		this->target_width = target_width;
	}


private:
	const int threads;
	Pipedata pipes[16];
	std::vector<Meshy*> meshlist;

	mat4 camera;
	mat4 camera_inverse;

	int framecounter;
	const Viewport * vp;
	std::vector<binstat> bin_index;

	struct SOADepth * db;
	struct SOACanvas * cb;
	class MaterialStore * materialstore;
	class TextureStore * texturestore;
	TrueColorPixel * __restrict target;
	int target_width;

	std::atomic<int> current_bin;
	std::vector<std::thread> workers;
	void workerthread(const int thread_number);

	class Telemetry& telemetry;
};

#endif //__RENDER_H
