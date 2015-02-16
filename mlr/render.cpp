
#include "stdafx.h"

#include <vector>

#include <Windows.h>

#include "tri.h"
#include "vec.h"
#include "clip.h"
#include "mesh.h"
#include "stats.h"
#include "utils.h"
#include "render.h"
#include "texture.h"
#include "viewport.h"
#include "fragment.h"
#include "distort.h"

using namespace std;

const int tile_width_in_subtiles = 16;
const int tile_height_in_subtiles = 8;

//#define SLEEP_METHOD
#define SLEEP_METHOD SleepEx(0,true)
//#define SLEEP_METHOD Sleep(0)
//#define SLEEP_METHOD Sleep(1)

__forceinline vec4 extrude_to_infinity(const vec4& p, const vec4& l)
{
	return vec4(
		p.x*l.w - l.x*p.w,
		p.y*l.w - l.y*p.w,
		p.z*l.w - l.z*p.w,
		0);
}

void Binner::reset(const int width, const int height)
{
	if (device_width != width  || device_height != height) {
		device_width = width;
		device_height = height;
		onResize();
	}

	for (auto& bin : bins) {
		bin.clear();
	}
}


void Binner::onResize()
{
	tilewidth = tile_width_in_subtiles * 8;
	tileheight = tile_height_in_subtiles * 8;

	device_width_in_tiles = (device_width + tilewidth - 1) / tilewidth;
	device_height_in_tiles = (device_height + tileheight - 1) / tileheight;

	device_max = vec4{float(device_width), float(device_height), 0, 0};

	bins.clear();
	int i = 0;
	for (int ty = 0; ty<device_height_in_tiles; ty++) {
		for (int tx = 0; tx<device_width_in_tiles; tx++) {
			irect bbox;
			bbox.x0 = tx * tilewidth;
			bbox.y0 = ty * tileheight;
			bbox.x1 = std::min((tx+1) * tilewidth, device_width); // clip to device edges
			bbox.y1 = std::min((ty+1) * tileheight, device_height);
			bins.push_back({ bbox, i++ });
		}
	}
}


void Binner::insert(const vec4& p1, const vec4& p2, const vec4& p3, const PFace& face)
{
	auto pmin = vmax(vmin(p1, vmin(p2, p3)), vec4::zero());
	auto pmax = vmin(vmax(p1, vmax(p2, p3)), device_max);

	auto x0 = int(pmin._x());
	auto y0 = int(pmin._y());
	auto x1 = int(pmax._x()); // trick: set this to pmin._x()
	auto y1 = int(pmax._y());

	const int ylim = min(y1 / tileheight, device_height_in_tiles - 1);
	const int xlim = min(x1 / tilewidth, device_width_in_tiles - 1);

	const int tx0 = x0 / tilewidth;
	for (int ty = y0 / tileheight; ty <= ylim; ty++) {
		int bin_row_offset = ty * device_width_in_tiles;
		for (int tx = tx0; tx <= xlim; tx++) {
			auto& bin = bins[bin_row_offset + tx];
			bin.faces.push_back(face);
		}
	}
}


void Binner::insert_shadow(const vec4& p1, const vec4& p2, const vec4& p3)
{
	auto pmin = vmax(vmin(p1, vmin(p2, p3)), vec4::zero());
	auto pmax = vmin(vmax(p1, vmax(p2, p3)), device_max);

	auto x0 = int(pmin._x());
	auto y0 = int(pmin._y());
	auto x1 = int(pmax._x()); // trick: set this to pmin._x()
	auto y1 = int(pmax._y());

	const int ylim = min(y1 / tileheight, device_height_in_tiles - 1);
	const int xlim = min(x1 / tilewidth, device_width_in_tiles - 1);

	const int tx0 = x0 / tilewidth;
	for (int ty = y0 / tileheight; ty <= ylim; ty++) {
		int bin_row_offset = ty * device_width_in_tiles;
		for (int tx = tx0; tx <= xlim; tx++) {
			auto& bin = bins[bin_row_offset + tx];
			bin.sv.push_back(p1);
			bin.sv.push_back(p2);
			bin.sv.push_back(p3);
		}
	}
}


void Binner::insert_gltri(
	const Viewport& vp,
	const vec4 * const pv,
	const vec4 * const pn,
	const vec4 * const pc,
	const vec4 * const pt,
	int i0, int i1, int i2,
	const int material_id
)
{
	auto p1 = vp.eye_to_device(pv[i0]);
	auto p2 = vp.eye_to_device(pv[i1]);
	auto p3 = vp.eye_to_device(pv[i2]);

	const auto d31 = p3 - p1;
	const auto d21 = p2 - p1;
	const float area = d31.x*d21.y - d31.y*d21.x;
	auto backfacing = area < 0;
	if (backfacing) return; // cull
	/*
	if (backfacing) {
		std::swap(p1, p3);
		std::swap(i0, i2);
	}
	*/

	auto pmin = vmax(vmin(p1, vmin(p2, p3)), vec4::zero());
	auto pmax = vmin(vmax(p1, vmax(p2, p3)), device_max);

	auto x0 = int(pmin._x());
	auto y0 = int(pmin._y());
	auto x1 = int(pmax._x()); // trick: set this to pmin._x()
	auto y1 = int(pmax._y());

	const int ylim = min(y1 / tileheight, device_height_in_tiles - 1);
	const int xlim = min(x1 / tilewidth, device_width_in_tiles - 1);

	const int tx0 = x0 / tilewidth;
	for (int ty = y0 / tileheight; ty <= ylim; ty++) {
		int bin_row_offset = ty * device_width_in_tiles;
		for (int tx = tx0; tx <= xlim; tx++) {
			auto& bin = bins[bin_row_offset + tx];

			unsigned char facedata = material_id;
			if (backfacing) facedata |= 0x80;
			bin.glface.push_back(facedata);

			bin.gldata.push_back(p1);
			bin.gldata.push_back(pv[i0]);
			bin.gldata.push_back(pn[i0]);
			bin.gldata.push_back(pc[i0]);
			bin.gldata.push_back(pt[i0]);

			bin.gldata.push_back(p2);
			bin.gldata.push_back(pv[i1]);
			bin.gldata.push_back(pn[i1]);
			bin.gldata.push_back(pc[i1]);
			bin.gldata.push_back(pt[i1]);

			bin.gldata.push_back(p3);
			bin.gldata.push_back(pv[i2]);
			bin.gldata.push_back(pn[i2]);
			bin.gldata.push_back(pc[i2]);
			bin.gldata.push_back(pt[i2]);
		}
	}
}




Pipeline::Pipeline(const int threads, class Telemetry& telemetry)
	:threads(threads), telemetry(telemetry)
{
	for (int i = 0; i < threads; i++) {
		pipes[i].setup(i, threads);
		if (i) {
			workers.push_back(thread(&Pipeline::workerthread, this, i));
		}
	}
}

Pipeline::~Pipeline()
{
	for (int i = 1; i < threads; i++) {
		pipes[i].my_signal = -1;
	}
	for (auto & thread : workers) {
		thread.join();
	}
}


void Pipedata::setup(const int thread_number, const int thread_count)
{
	this->thread_number = thread_number;
	this->thread_count = thread_count;
	this->root_count = 0;
	this->my_signal = 0;
}


void Pipedata::addFace(const Viewport& vp, const Face& fsrc)
{
	const int vidx1 = fsrc.ivp[0] + vbase;
	const int vidx2 = fsrc.ivp[1] + vbase;
	const int vidx3 = fsrc.ivp[2] + vbase;

	const auto& pv1_p = vlst_p[vidx1];
	const auto& pv2_p = vlst_p[vidx2];
	const auto& pv3_p = vlst_p[vidx3];
	const auto& pv1_cf = vlst_cf[vidx1];
	const auto& pv2_cf = vlst_cf[vidx2];
	const auto& pv3_cf = vlst_cf[vidx3];

	// early out if all points are outside of any plane
	if (pv1_cf & pv2_cf & pv3_cf) return;

	const unsigned required_clipping = pv1_cf | pv2_cf | pv3_cf;
	if (required_clipping == 0) {
		// all inside
		process_face(fsrc.make_rebased(vbase, tbase, nbase));
		return;
	}

	// 	needs clipping
	unsigned a_vidx[32], b_vidx[32];
	unsigned a_tidx[32], b_tidx[32];
	unsigned a_nidx[32], b_nidx[32];
	unsigned a_cnt = 3;
	for (unsigned i = 0; i < 3; i++) {
		a_vidx[i] = fsrc.ivp[i] + vbase;
		a_tidx[i] = fsrc.iuv[i] + tbase;
		a_nidx[i] = fsrc.ipn[i] + nbase;
	}


	for (int clip_plane = 0; clip_plane < 5; clip_plane++) { // XXX set to 6 to enable the far plane

		const int planebit = 1 << clip_plane;
		if (!(required_clipping & planebit)) continue; // skip plane that is ok

		bool we_are_inside;
		unsigned this_pi = 0;
		unsigned b_cnt = 0;

		do {
			const auto next_pi = (this_pi + 1) % a_cnt; // wrap

			const auto& this_v_p = vlst_p[a_vidx[this_pi]];
			const auto  this_v_c = vp.eye_to_clip(this_v_p);
			const auto& this_t = tlst[a_tidx[this_pi]];
			const auto& this_n = nlst[a_nidx[this_pi]];

			const auto& next_v_p = vlst_p[a_vidx[next_pi]];
			const auto  next_v_c = vp.eye_to_clip(next_v_p);
			const auto& next_t = tlst[a_tidx[next_pi]];
			const auto& next_n = nlst[a_nidx[next_pi]];

			if (this_pi == 0) {
				we_are_inside = Guardband::is_inside(planebit, this_v_c);
			}
			const bool next_is_inside = Guardband::is_inside(planebit, next_v_c);

			if (we_are_inside) {
				b_vidx[b_cnt] = a_vidx[this_pi];
				b_tidx[b_cnt] = a_tidx[this_pi];
				b_nidx[b_cnt] = a_nidx[this_pi];
				b_cnt++;
			}

			if (we_are_inside != next_is_inside) {
				we_are_inside = !we_are_inside;

				const float t = Guardband::clipLine(planebit, this_v_c, next_v_c);

				auto new_p = lerp(this_v_p, next_v_p, t);
				auto new_c = vp.eye_to_clip(new_p);
				auto new_f = vp.clip_to_device(new_c);
				vlst_p.push_back(new_p);
				vlst_f.push_back(new_f);
				vlst_cf.push_back(0);
				b_vidx[b_cnt] = vlst_p.size() - 1;

				tlst.push_back(lerp(this_t, next_t, t));  b_tidx[b_cnt] = tlst.size() - 1;
				nlst.push_back(lerp(this_n, next_n, t));  b_nidx[b_cnt] = nlst.size() - 1;
				b_cnt++;
			}

			this_pi = next_pi;
		} while (this_pi != 0);

		for (unsigned i = 0; i < b_cnt; i++) {
			a_vidx[i] = b_vidx[i];
			a_tidx[i] = b_tidx[i];
			a_nidx[i] = b_nidx[i];
		}
		a_cnt = b_cnt;
		
		if (a_cnt == 0) break;
	}
	if (a_cnt == 0) return;

	for (unsigned a = 1; a < a_cnt - 1; a++) {
		PFace f = make_tri(
			fsrc.mf,
			a_vidx[0], a_vidx[a], a_vidx[a + 1],
			a_tidx[0], a_tidx[a], a_tidx[a + 1],
			a_nidx[0], a_nidx[a], a_nidx[a + 1]);
		process_face(f);
	}

}




void Pipedata::addNormal(const vec4& src, const mat4& m)
{
	nlst.push_back(mat4_mul(m, src));
	new_ncnt++;
}


void Pipedata::addUV(const vec4& src)
{
	tlst.push_back(src);
	new_tcnt++;
}

void Pipedata::addLight(const mat4& camera_inverse, const Light& light)
{
	Light l2 = light;
	l2.position = mat4_mul(camera_inverse, light.position);
	llst.push_back(l2);
}



__forceinline void Pipedata::addVertex(const Viewport& vp, const vec4& src, const mat4& m)
{
	auto point_in_eyespace = mat4_mul(m, src);
	auto point_in_clipspace = vp.eye_to_clip(point_in_eyespace);
	auto point_in_devicespace = vp.clip_to_device(point_in_clipspace);
	auto clipflags = Guardband::clipPoint(point_in_clipspace);

	vlst_p.push_back(point_in_eyespace);
	vlst_f.push_back(point_in_devicespace);
	vlst_cf.push_back(clipflags);
	new_vcnt++;
}


void Pipedata::process_face(PFace& f)
{
	vec4 p1 = vlst_f[f.ivp[0]];
	vec4 p2 = vlst_f[f.ivp[1]];
	vec4 p3 = vlst_f[f.ivp[2]];

	const vec4 d31 = p3 - p1;
	const vec4 d21 = p2 - p1;
	const float area = d31.x*d21.y - d31.y*d21.x;

	f.backfacing = area < 0;
	
	if (f.backfacing) return; // cull

//	if (f.backfacing) {
//		std::swap(p1, p3);
//	}

	binner.insert(p1, p2, p3, f);
}


void Pipedata::addMeshy(Meshy& mi, const mat4& camera_inverse, const Viewport& vp)
{
	const Mesh& mesh = *mi.mesh;

	int next_idx = (thread_number + (root_count++)) % thread_count;

	int this_idx = 0;
	mat4 item;
	for (mi.begin(thread_number); mi.next(thread_number, item); this_idx++) {

		if (this_idx != next_idx) continue;
		next_idx += thread_count;

		mat4 to_camera;
		mat4_mul(camera_inverse, item, to_camera);

		vec4 tbb[8];
		for (int bi = 0; bi < 8; bi++ )
			tbb[bi] = mat4_mul(to_camera, mesh.bbox[bi]);
		if (!vp.is_visible(tbb)) continue;

		if ( mi.shadows_enabled() ) {
			ShadowMesh sm;
			sm.mesh = mi.mesh;
			sm.c2o = mat4_inverse(to_camera);
			sm.vbase = vlst_p.size();
			shadowqueue.push_back(sm);
		}

		begin_batch();
		for (auto& vert : mesh.bvp)
			addVertex(vp, vert, to_camera);
		for (auto& uv : mesh.buv)
			addUV(uv);
		for (auto& normal : mesh.bpn)
			addNormal(normal, to_camera);

		Face face;
		mi.fbegin(thread_number);
		for (; mi.fnext(thread_number, face); ) {
			addFace(vp, face);
		}
//		for (auto& face : mesh.faces)
//			addFace(face);
		end_batch();

	}
}

void Pipeline::render_thread(const int thread_number)
{
	while (1) {
		auto binnumber = current_bin++;

		int idx;
		if (0) {
			// linear order
			if (binnumber >= pipes[0].binner.bins.size()) break;
			idx = binnumber;
		} else {
			// use the sort index
			if (binnumber >= bin_index.size()) break;
			idx = bin_index[binnumber].first;
		}

		const irect& tilerect = pipes[0].binner.bins[idx].rect;

		db->clear(tilerect);
		//cb->clear(tilerect);
		if (this->clear_color_enable) {
			cb->clear(tilerect, this->clear_color_rgb);
		}
		for (int ti = 0; ti < threads; ti++) {
			pipes[ti].render(db->rawptr(), cb->rawptr(), *materialstore, *texturestore, *vp, idx);
			pipes[ti].render_gltri(db->rawptr(), cb->rawptr(), *materialstore, *texturestore, *vp, idx);
			pipes[ti].render_rect(db->rawptr(), cb->rawptr(), *materialstore, *texturestore, *vp, idx);
//			mark(false);
		}
		convertCanvas(tilerect, target_width, target, cb->rawptr(), PostprocessNoop());
		telemetry.mark(thread_number);
	}
}


void Pipeline::addLight(const Light& light)
{
	for (int i = 0; i < threads; i++) {
		this->pipes[i].addLight(camera_inverse, light);
	}
}


void Pipeline::process_thread(const int thread_number){
	auto& pipe = this->pipes[thread_number];
	for (auto& mesh : this->meshlist) {
		pipe.addMeshy(*mesh, camera_inverse, *vp);
	}
	telemetry.mark(thread_number);
}

void Pipeline::shadow_thread(const int thread_number)
{
	auto& pipe = this->pipes[thread_number];

	pipe.build_shadows(*vp, 0);
	telemetry.mark(thread_number);
}


void Pipeline::workerthread(const int thread_number)
{
	bind_to_cpu(thread_number);
	auto& pipe = pipes[thread_number];
	auto& signal_start = pipe.my_signal;

	bool done = false;
	while (!done) {
		while (signal_start == 0) SLEEP_METHOD;

		int job_to_do = signal_start;

		sse_configure();

		if (job_to_do == -1) {
			done = true;
		} else if (job_to_do == 1) {
			process_thread(thread_number);
		} else if (job_to_do == 2) {
			render_thread(thread_number);
		} else if (job_to_do == 3) {
			shadow_thread(thread_number);
		}
		signal_start = 0;
	}
}

#define START_WORKERS(a) for (int _i=1; _i<this->threads; _i++) pipes[_i].my_signal = (a)
#define JOIN_WORKERS for(int _i=1; _i<this->threads; _i++) while(pipes[_i].my_signal!=0) SLEEP_METHOD

void Pipeline::render()
{
	START_WORKERS(1); process_thread(0); JOIN_WORKERS;
	telemetry.inc();

//	START_WORKERS(3); shadow_thread(0); JOIN_WORKERS;
//	telemetry.inc();

	index_bins();
	telemetry.mark(0);
	telemetry.inc();

	current_bin = 0;
	START_WORKERS(2); render_thread(0); JOIN_WORKERS;
	telemetry.inc();
}



void Pipedata::render(__m128 * __restrict db, SOAPixel * __restrict cb, MaterialStore& materialstore, TextureStore& texturestore, const Viewport& vp, const int bin_idx)
{
	FlatShader my_shader;
	my_shader.setColorBuffer(cb);
	my_shader.setDepthBuffer(db);
	my_shader.setColor(vec4(1, 0.66, 0.33, 0));

	WireShader wire_shader;
	wire_shader.setColorBuffer(cb);
	wire_shader.setDepthBuffer(db);
	wire_shader.setColor(vec4(1, 0.66, 0.33, 0));

	auto& bin = binner.bins[bin_idx];
	for (auto& face : bin.faces) {

		if (face.backfacing) continue;

		const auto& v0_f = vlst_f[face.ivp[0]];
		const auto& v1_f = vlst_f[face.ivp[1]];
		const auto& v2_f = vlst_f[face.ivp[2]];

		Material& mat = materialstore.store[face.mf];

		if (mat.imagename != string("")) {
			const auto tex = texturestore.find(mat.imagename);
			const auto texunit = ts_pow2_mipmap<9>(&tex->b[0]);
			auto tex_shader = TextureShader<ts_pow2_mipmap<9>>(texunit);
			tex_shader.setColorBuffer(cb);
			tex_shader.setDepthBuffer(db);
			tex_shader.setUV(tlst[face.iuv[0]], tlst[face.iuv[1]], tlst[face.iuv[2]]);
			tex_shader.setup(vp.width, vp.height, v0_f, v1_f, v2_f);
			draw_triangle(bin.rect, v0_f, v1_f, v2_f, tex_shader);
		}
		else {
			if (1) {
				my_shader.setColor(vec4(mat.kd.x, mat.kd.y, mat.kd.z, 0));
				my_shader.setup(vp.width, vp.height, v0_f, v1_f, v2_f);
				draw_triangle(bin.rect, v0_f, v1_f, v2_f, my_shader);
			} else {
				wire_shader.setColor(vec4(mat.kd.x, mat.kd.y, mat.kd.z, 0));
				wire_shader.setup(vp.width, vp.height, v0_f, v1_f, v2_f);
				draw_triangle(bin.rect, v0_f, v1_f, v2_f, wire_shader);
			}
		}

	}//faces
}


void Pipedata::add_shadow_triangle(const Viewport& vp, const vec4& p1, const vec4& p2, const vec4& p3)
{
	unsigned char cf[3];

	vec4 pv[16];
	vec4 pb[16];
	pv[0] = p1; pv[1] = p2; pv[2] = p3;
	for (int i=0; i<3; i++) {
		auto point_in_clipspace = vp.eye_to_clip(pv[i]);
		cf[i] = Guardband::clipPoint(point_in_clipspace);
	}
	int pvcnt = 3;


	if (cf[0] & cf[1] & cf[2]) return;

	const unsigned required_clipping = cf[0] | cf[1] | cf[2];
	if (required_clipping == 0) {
		// all inside
		binner.insert_shadow(
			vp.eye_to_device(pv[0]),
			vp.eye_to_device(pv[1]),
			vp.eye_to_device(pv[2])
			);
		return;
	}

	for (int clip_plane=0; clip_plane<5; clip_plane++) {
		const int planebit = 1 << clip_plane;
		if (!(required_clipping & planebit)) continue;

		bool we_are_inside;
		unsigned this_pi = 0;
		auto this_v = pv[this_pi];
		auto this_c = vp.eye_to_clip(this_v);
		unsigned pbcnt = 0;

		do {
			const auto next_pi = (this_pi + 1) % pvcnt;

			const auto next_v = pv[next_pi];
			const auto next_c = vp.eye_to_clip(next_v);

			if (this_pi == 0) {
				we_are_inside = Guardband::is_inside(planebit, this_c);
			}
			bool next_is_inside = Guardband::is_inside(planebit, next_c);

			if (we_are_inside) {
				pb[pbcnt] = this_v;
				pbcnt++;
			}

			if (we_are_inside != next_is_inside) {
				we_are_inside = !we_are_inside;
				auto t = Guardband::clipLine(planebit, this_c, next_c);
				pb[pbcnt] = lerp(this_v, next_v, t);
				pbcnt++;
			}

			this_pi = next_pi;
			this_v = next_v;
			this_c = next_c;
		} while (this_pi != 0);

		for (unsigned i=0; i<pbcnt; i++) {
			pv[i] = pb[i];
		}
		pvcnt = pbcnt;

		if (pvcnt == 0) break;
	}
	if (pvcnt == 0) return;

	for (int a=1; a<pvcnt-1; a++) {
		binner.insert_shadow(
			vp.eye_to_device(pv[0]),
			vp.eye_to_device(pv[a]),
			vp.eye_to_device(pv[a+1])
			);
	}
}

void Pipedata::build_shadows(const Viewport& vp, const int light_id)
{
	auto& light = this->llst[light_id];
	for (auto& svmesh : shadowqueue) {
		const vec4 * const vb = &vlst_p[svmesh.vbase];

		auto inv_light_position = mat4_mul(svmesh.c2o, light.position);
		auto inv_light_position_x = inv_light_position.xxxx();
		auto inv_light_position_y = inv_light_position.yyyy();
		auto inv_light_position_z = inv_light_position.zzzz();

		for (auto& face : svmesh.mesh->faces) {

			auto light_dir_x = face.px - inv_light_position_x;
			auto light_dir_y = face.py - inv_light_position_y;
			auto light_dir_z = face.pz - inv_light_position_z;

			auto dots = (face.nx*light_dir_x + face.ny*light_dir_y + face.nz*light_dir_z);
			if ( dots._x() > 0 ) {    // facing away from light
				auto p1 = extrude_to_infinity(vb[face.ivp[0]], light.position);
				auto p2 = extrude_to_infinity(vb[face.ivp[1]], light.position);
				auto p3 = extrude_to_infinity(vb[face.ivp[2]], light.position);
				add_shadow_triangle(vp, p1, p2, p3);
			} else {               // facing towards lightA
				vec4 edge_a[3];
				vec4 edge_b[3];
				int paircnt = 0;
				if (dots._y() > 0) {
					edge_a[paircnt] = vb[face.ivp[1]];
					edge_b[paircnt] = vb[face.ivp[0]];
					paircnt++;
				}
				if (dots._z() > 0) {
					edge_a[paircnt] = vb[face.ivp[2]];
					edge_b[paircnt] = vb[face.ivp[1]];
					paircnt++;
				}
				if (dots._w() > 0) {
					edge_a[paircnt] = vb[face.ivp[0]];
					edge_b[paircnt] = vb[face.ivp[2]];
					paircnt++;
				}
				for (int i=0; i<paircnt; i++) {
					auto& na = edge_a[i];
					auto& nb = edge_b[i];
					auto fa = extrude_to_infinity(na, light.position);
					auto fb = extrude_to_infinity(nb, light.position);
					add_shadow_triangle(vp, na, nb, fb); // quad na->nb->fb->fa
					add_shadow_triangle(vp, na, fb, fa);
				}
				//front cap
				auto& c1 = vb[face.ivp[0]];
				auto& c2 = vb[face.ivp[1]];
				auto& c3 = vb[face.ivp[2]];
				add_shadow_triangle(vp, c1, c2, c3);
			}
		}
	}
}


void Pipedata::process_gltri(const Viewport& vp, const int material_id)
{
	unsigned char cf[3];

	vec4 pb_v[16];
	vec4 pb_n[16];
	vec4 pb_c[16];
	vec4 pb_t[16];
	for (int i=0; i<3; i++) {
		auto point_in_clipspace = vp.eye_to_clip(tri_eye[i]);
		cf[i] = Guardband::clipPoint(point_in_clipspace);
	}
	int pvcnt = 3;


	if (cf[0] & cf[1] & cf[2]) return;

	const unsigned required_clipping = cf[0] | cf[1] | cf[2];
	if (required_clipping == 0) {
		// all inside
		binner.insert_gltri(vp, tri_eye, tri_nor, tri_col, tri_tex, 0, 1, 2, material_id);
		return;
	}

	for (int clip_plane=0; clip_plane<5; clip_plane++) {
		const int planebit = 1 << clip_plane;
		if (!(required_clipping & planebit)) continue;

		bool we_are_inside;
		unsigned this_pi = 0;
		auto this_v = tri_eye[this_pi];
		auto this_n = tri_nor[this_pi];
		auto this_c = tri_col[this_pi];
		auto this_t = tri_tex[this_pi];
		auto this_clip = vp.eye_to_clip(this_v);
		unsigned pbcnt = 0;

		do {
			const auto next_pi = (this_pi + 1) % pvcnt;

			const auto next_v = tri_eye[next_pi];
			const auto next_n = tri_nor[next_pi];
			const auto next_c = tri_col[next_pi];
			const auto next_t = tri_tex[next_pi];
			const auto next_clip = vp.eye_to_clip(next_v);

			if (this_pi == 0) {
				we_are_inside = Guardband::is_inside(planebit, this_clip);
			}
			bool next_is_inside = Guardband::is_inside(planebit, next_clip);

			if (we_are_inside) {
				pb_v[pbcnt] = this_v;
				pb_n[pbcnt] = this_n;
				pb_c[pbcnt] = this_c;
				pb_t[pbcnt] = this_t;
				pbcnt++;
			}

			if (we_are_inside != next_is_inside) {
				we_are_inside = !we_are_inside;
				auto t = Guardband::clipLine(planebit, this_clip, next_clip);
				pb_v[pbcnt] = lerp(this_v, next_v, t);
				pb_n[pbcnt] = lerp(this_n, next_n, t);
				pb_c[pbcnt] = lerp(this_c, next_c, t);
				pb_t[pbcnt] = lerp(this_t, next_t, t);
				pbcnt++;
			}

			this_pi = next_pi;
			this_v = next_v;
			this_n = next_n;
			this_c = next_c;
			this_t = next_t;
			this_clip = next_clip;
		} while (this_pi != 0);

		for (unsigned i=0; i<pbcnt; i++) {
			tri_eye[i] = pb_v[i];
			tri_nor[i] = pb_n[i];
			tri_col[i] = pb_c[i];
			tri_tex[i] = pb_t[i];
		}
		pvcnt = pbcnt;

		if (pvcnt == 0) break;
	}
	if (pvcnt == 0) return;

	for (int a=1; a<pvcnt-1; a++) {
		binner.insert_gltri(vp, tri_eye, tri_nor, tri_col, tri_tex, 0, a, a+1, material_id);
	}
}

#pragma pack(1)
struct VertexData {
	vec4 f;
	vec4 p;
	vec4 n;
	vec4 c;
	vec4 t;
};
#pragma pack()

void Pipedata::render_gltri(__m128 * __restrict db, SOAPixel * __restrict cb, MaterialStore& materialstore, TextureStore& texturestore, const Viewport& vp, const int bin_idx)
{
	FlatShader my_shader;
	my_shader.setColorBuffer(cb);
	my_shader.setDepthBuffer(db);
	my_shader.setColor(vec4(1, 0.66, 0.33, 0));

	WireShader wire_shader;
	wire_shader.setColorBuffer(cb);
	wire_shader.setDepthBuffer(db);
	wire_shader.setColor(vec4(1, 0.66, 0.33, 0));

	auto& bin = binner.bins[bin_idx];
	unsigned di = 0;
	unsigned fi = 0;
	while (di < bin.gldata.size()) {

		auto facedata = bin.glface[fi];
		bool backfacing = (facedata & 0x80) > 0;
		int material_id = facedata & 0x7f;

		if (backfacing) continue;

		const auto& v0 = *reinterpret_cast<VertexData*>(&bin.gldata[di+0]);
		const auto& v1 = *reinterpret_cast<VertexData*>(&bin.gldata[di+5]);
		const auto& v2 = *reinterpret_cast<VertexData*>(&bin.gldata[di+10]);
		di += 15; fi++;

		Material& mat = materialstore.store[material_id];

		if (mat.imagename != string("")) {
			const auto tex = texturestore.find(mat.imagename);
			const auto texunit = ts_pow2_mipmap<9>(&tex->b[0]);
			auto tex_shader = TextureShader<ts_pow2_mipmap<9>>(texunit);
			tex_shader.setColorBuffer(cb);
			tex_shader.setDepthBuffer(db);
			tex_shader.setUV(v0.t, v1.t, v2.t);
			tex_shader.setup(vp.width, vp.height, v0.f, v1.f, v2.f);
			draw_triangle(bin.rect, v0.f, v1.f, v2.f, tex_shader);
		}
		else {
			if (0) {
				my_shader.setColor(vec4(mat.kd.x, mat.kd.y, mat.kd.z, 0));
				my_shader.setup(vp.width, vp.height, v0.f, v1.f, v2.f);
				draw_triangle(bin.rect, v0.f, v1.f, v2.f, my_shader);
			} else {
				wire_shader.setColor(vec4(mat.kd.x, mat.kd.y, mat.kd.z, 0));
				wire_shader.setup(vp.width, vp.height, v0.f, v1.f, v2.f);
				draw_triangle(bin.rect, v0.f, v1.f, v2.f, wire_shader);
			}
		}

	}//gldata
}


void Pipedata::render_rect(__m128 * __restrict db, SOAPixel * __restrict cb, MaterialStore& materialstore, TextureStore& texturestore, const Viewport& vp, const int bin_idx)
{
	auto& bin = binner.bins[bin_idx];
	const irect& tilerect = bin.rect;
	unsigned di = 0;
	unsigned fi = 0;
	while (di < this->rectbyte.size()) {

		int rtype = this->rectbyte[di++];
		int rvals = this->rectbyte[di++];

		if (rtype == 1) {
			const auto tex = texturestore.find("girl256.png");
			const auto texunit = ts_pow2_mipmap_nearest<8>(&tex->b[0]);
			DistortShader<ts_pow2_mipmap_nearest<8>> ds(texunit);
			ds.setColorBuffer(cb);
			ds.setup(vp.width, vp.height);
			for (int pi=0; pi<rvals; pi++) {
				const float val = this->rectdata[fi++];
				ds.setParam(pi, val);
			}
			draw_rectangle(tilerect, ds);
		}

	}//rectbytes
}

