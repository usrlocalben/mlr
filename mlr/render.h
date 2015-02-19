
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


struct Tilebin {
	irect rect;
	int id;
	vectorsse<vec4> vf;
	vectorsse<char> backfacing;
	vectorsse<PFace> faces;
	vectorsse<vec4> sv;

	// glVertex api
	vectorsse<vec4> gldata;
	std::vector<unsigned char> glface;

	void clear() {
		vf.clear();
		backfacing.clear();
		faces.clear();
		sv.clear();

		gldata.clear();
	}
};

class Binner {
public:
	void reset(const int cur_width, const int cur_height);
	void insert(const vec4& p1, const vec4& p2, const vec4& p3, const bool backfacing, const PFace& face);
	void insert_shadow(const vec4& p1, const vec4& p2, const vec4& p3);
	void insert_gltri(
		const Viewport& vp,
		const Viewdevice& vpd,
		const vec4 * const pv,
		const vec4 * const pn,
		const vec4 * const pc,
		const vec4 * const pt,
		int i0, int i1, int i2,
		const int material_id);
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



class Pipedata {
public:
	void setup(const int thread_number, const int thread_count);

	void addMeshy(Meshy& mi, const mat4& camera_inverse, const Viewport& vp, const Viewdevice& vpd);
	void add_shadow_triangle(const Viewport& vp, const Viewdevice& vpd, const vec4& p1, const vec4& p2, const vec4& p3);
	void build_shadows(const Viewport& vp, const Viewdevice& vpd, const int light_id, const struct ShadowMesh& svmesh);

	void reset(const int width, const int height) {
		vlst_p.clear(); vlst_cf.clear();
		tlst.clear();
		nlst.clear();
		llst.clear();
		batch_in_progress = 0;
		root_count = 0;
		binner.reset(width, height);
		rectdata.clear();
		rectbyte.clear();
	}
	void addFace(const Viewport& vp, const Viewdevice& vpd, const Face& fsrc);
	void addNormal(const vec4& src, const mat4& m);
	void addUV(const vec4& src);
	void addLight(const mat4& camera_inverse, const Light& light);
	Binner binner;
	void render(__m128 * __restrict db, SOAPixel * __restrict cb, class MaterialStore& materialstore, class TextureStore& texturestore, const Viewdevice& vpd, const int bin_idx);
	void render_gltri(__m128 * __restrict db, SOAPixel * __restrict cb, class MaterialStore& materialstore, class TextureStore& texturestore, const Viewdevice& vpd, const int bin_idx);
	void render_rect(__m128 * __restrict db, SOAPixel * __restrict cb, class MaterialStore& materialstore, class TextureStore& texturestore, const Viewdevice& vpd, const int bin_idx);

	void addVertex(const Viewport& vp, const vec4& src, const mat4& m);

	std::atomic<int> my_signal;
private:
	void begin_batch() {
		_ASSERT(batch_in_progress == 0);
		vbase = vlst_p.size();    new_vcnt = 0;
		tbase = tlst.size();    new_tcnt = 0;
		nbase = nlst.size();    new_ncnt = 0;
		batch_in_progress = 1;
	}
	//	void mark() {
	//		clipbase = vbase + new_vcnt;
	//	}
	void end_batch() {
		vbase = vlst_p.size();
		tbase = tlst.size();
		nbase = nlst.size();
		batch_in_progress = 0;
	}
	void process_face(PFace& f, const Viewport& vp, const Viewdevice& vpd);

	// indexed buffers api
	vectorsse<vec4> vlst_p;
//	vectorsse<vec4> vlst_f;
	vectorsse<unsigned char> vlst_cf;
	unsigned vbase;    int new_vcnt;    int clipbase;
	vectorsse<vec4> nlst;     unsigned nbase;    int new_ncnt;
	vectorsse<vec4> tlst;     unsigned tbase;    int new_tcnt;
	vectorsse<Light> llst;    unsigned lbase;
	int batch_in_progress;
	int thread_number;
	int thread_count;
	int root_count;


	// rect-shader api
private:
	vectorsse<int> rectbyte;
	vectorsse<float> rectdata;
	int rect_param_count;
public:
	void rect_begin(const int ty) {
		rectbyte.push_back(ty);
		rect_param_count = 0;
	}
	void rect_data(const float val) {
		rectdata.push_back(val);
		rect_param_count++;
	}
	void rect_end() {
		rectbyte.push_back(rect_param_count);
	}


	// glVertex() api
private:
	vec4 cur_nor;
	vec4 cur_col;
	vec4 cur_tex;
	int cur_mat;
	int vertex_idx;
	vec4 tri_nor[16];
	vec4 tri_col[16];
	vec4 tri_tex[16];
	vec4 tri_eye[16];
	const Viewport * batch_vp;
	const Viewdevice * batch_vpd;
public:
	void glBegin(const Viewport& vp, const Viewdevice& vpd) {
		vertex_idx = 0;
		batch_vp = &vp;
		batch_vpd = &vpd;
		cur_mat = 0;
	}
	void glEnd() {
		//assert(vertex_idx==0);
	}
	__forceinline void glColor(const vec4& v) { cur_col = v; }
	__forceinline void glNormal(const vec4& v) { cur_nor = v; }
	__forceinline void glTexture(const vec4& v) { cur_tex = v; }
	__forceinline void glMaterial(const int v) { cur_mat = v; }
	__forceinline void glVertex(const vec4& v) {
		//assert that we are in a batch
		tri_nor[vertex_idx] = cur_nor;
		tri_col[vertex_idx] = cur_col;
		tri_tex[vertex_idx] = cur_tex;
		tri_eye[vertex_idx] = v;  // early clip optimization?
		vertex_idx++;
		if (vertex_idx == 3) {
		    process_gltri(*batch_vp, *batch_vpd, cur_mat);
		    vertex_idx = 0;
		}
	}
private:
	void process_gltri(const Viewport& vp, const Viewdevice& vpd, const int material_id);

};


typedef std::pair<int, int> binstat;


class Pipeline {
public:
	Pipeline(const int threads, class Telemetry& telemetry);
	~Pipeline();

	void addMeshy(Meshy& mi, const Viewport * vp) {
		meshlist.push_back(&mi);
		viewlist.push_back(vp);
	}

	void addLight(const Light& li);
//XXX	void setViewport(const Viewport * const vp) { this->vp = vp; }
	void setViewdevice(const Viewdevice * const vpd) { this->vpd = vpd; }

	void reset(const int width, const int height) {
		for (auto& item : pipes) {
			item.reset(width, height);
		}
		meshlist.clear();  viewlist.clear();
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
		for (size_t bi = 0; bi < pipes[0].binner.bins.size(); bi++) {
			int ax = 0; 
			for (int ti = 0; ti < threads; ti++) {
				ax += pipes[ti].binner.bins[bi].faces.size();
				ax += pipes[ti].binner.bins[bi].glface.size();
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
	Pipedata * getPipe() {
		return &this->pipes[0];
	}

private:
	const int threads;
	Pipedata pipes[16];
	std::vector<Meshy*> meshlist;
	std::vector<const Viewport *> viewlist;

	mat4 camera;
	mat4 camera_inverse;

	int framecounter;
	const Viewdevice * vpd;
	std::vector<binstat> bin_index;

	struct SOADepth * db;
	struct SOACanvas * cb;
	class MaterialStore * materialstore;
	class TextureStore * texturestore;
	TrueColorPixel * __restrict target;
	int target_width;

	std::atomic<unsigned> current_bin;
	std::vector<std::thread> workers;
	void workerthread(const int thread_number);

	class Telemetry& telemetry;


public:
	bool clear_color_enable;
	vec4 clear_color_rgb;
	void clear(const bool enable, const vec4& c) {
		clear_color_enable = enable;
		clear_color_rgb = c;
	}
};

#endif //__RENDER_H
