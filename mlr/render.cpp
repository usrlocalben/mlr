
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

using namespace std;

const int tile_width_in_subtiles = 16;
const int tile_height_in_subtiles = 8;


void Binner::reset(const int width, const int height)
{
	if (device_width != width  || device_height != height) {
		device_width = width;
		device_height = height;
		onResize();
	}

	for (auto& bin : bins) {
		bin.reset();
	}
}


void Binner::onResize()
{
	tilewidth = tile_width_in_subtiles * 8;
	tileheight = tile_height_in_subtiles * 8;

	device_width_in_tiles = (device_width + tilewidth - 1) / tilewidth;
	device_height_in_tiles = (device_height + tileheight - 1) / tileheight;

	device_max = vec4(device_width, device_height, 0, 0);

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

	const auto& pv1 = vlst[vidx1];
	const auto& pv2 = vlst[vidx2];
	const auto& pv3 = vlst[vidx3];

	// early out if all points are outside of any plane
	if (pv1.cf & pv2.cf & pv3.cf) return;

	const unsigned required_clipping = pv1.cf | pv2.cf | pv3.cf;
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

			const auto& this_v = vlst[a_vidx[this_pi]];
			const auto& this_t = tlst[a_tidx[this_pi]];
			const auto& this_n = nlst[a_nidx[this_pi]];

			const auto& next_v = vlst[a_vidx[next_pi]];
			const auto& next_t = tlst[a_tidx[next_pi]];
			const auto& next_n = nlst[a_nidx[next_pi]];

			if (this_pi == 0) {
				we_are_inside = Guardband::is_inside(planebit, this_v.c);
			}
			const bool next_is_inside = Guardband::is_inside(planebit, next_v.c);

			if (we_are_inside) {
				b_vidx[b_cnt] = a_vidx[this_pi];
				b_tidx[b_cnt] = a_tidx[this_pi];
				b_nidx[b_cnt] = a_nidx[this_pi];
				b_cnt++;
			}

			if (we_are_inside != next_is_inside) {
				we_are_inside = !we_are_inside;

				const float t = Guardband::clipLine(planebit, this_v.c, next_v.c);
				vlst.push_back(clipcalc(vp, this_v, next_v, t));  b_vidx[b_cnt] = vlst.size() - 1;
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

__forceinline void PVertex::process(const Viewport& vp)
{
	this->c = vp.to_clip_space(this->p);
	vec4 s = vp.to_screen_space(this->c);

	float one_over_w = 1 / s._w();
	this->f = s / s.wwww();
	this->f.w = one_over_w;

	this->cf = Guardband::clipPoint(this->c);
}

__forceinline void Pipedata::addVertex(const Viewport& vp, const vec4& src, const mat4& m)
{
	PVertex pv;
	pv.p = mat4_mul(m, src);
	pv.process(vp);
	vlst.push_back(pv);
	new_vcnt++;
}


PVertex Pipedata::clipcalc(const Viewport& vp, const PVertex& a, const PVertex& b, const float t)
{
	PVertex n;
	n.p = lerp(a.p, b.p, t);
	n.c = lerp(a.c, b.c, t);
	vec4 s = vp.to_screen_space(n.c);
	n.n = lerp(a.n, b.n, t);

	float one_over_w = 1 / s.w;
	n.f = s / s.wwww();
	n.f.w = one_over_w;
	return n;
}


void Pipedata::process_face(PFace& f)
{
	vec4 p1 = vlst[f.ivp[0]].f;
	vec4 p2 = vlst[f.ivp[1]].f;
	vec4 p3 = vlst[f.ivp[2]].f;

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
			sm.vbase = vlst.size();
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
		int binnumber = current_bin++;
		if (binnumber >= bin_index.size()) break;

		auto& idx = bin_index[binnumber];

		const irect& tilerect = pipes[0].binner.bins[idx.first].rect;

		db->clear(tilerect);
		cb->clear(tilerect);
		for (int ti = 0; ti < threads; ti++) {
			pipes[ti].render(db->rawptr(), cb->rawptr(), *materialstore, *texturestore, *vp, idx.first);
//			mark(false);
		}
		convertCanvas(tilerect, target_width, target, cb->rawptr(), PostprocessNoop());
		telemetry.mark(thread_number);
	}
}


void Pipeline::process_thread(const int thread_number){
	auto& pipe = this->pipes[thread_number];
	for (auto& mesh : this->meshlist) {
		pipe.addMeshy(*mesh, camera_inverse, *vp);
	}
	telemetry.mark(thread_number);
}


void Pipeline::workerthread(const int thread_number)
{
	bind_to_cpu(thread_number);

	auto& signal_start = pipes[thread_number].my_signal;

	bool done = false;
	while (!done) {
		while (signal_start == 0) {
			Sleep(0);
		}

		int job_to_do = signal_start;

		sse_speedup();

		if (job_to_do == -1) {
			done = true;
		} else if (job_to_do == 1) {
			process_thread(thread_number);
		} else if (job_to_do == 2) {
			render_thread(thread_number);
		}
		signal_start = 0;
	}
}

void Pipeline::render()
{
	for (int i = 1; i < this->threads; i++) pipes[i].my_signal = 1;
	process_thread(0);
	for (int i = 1; i < this->threads; i++) {
		while (pipes[i].my_signal != 0) {
			Sleep(0);
		}
	}
	telemetry.inc();

	index_bins();
	telemetry.mark(0);
	telemetry.inc();

	current_bin = 0;
	for (int i = 1; i < this->threads; i++) pipes[i].my_signal = 2;
	render_thread(0);
	for (int i = 1; i < this->threads; i++) {
		while (pipes[i].my_signal != 0) {
			Sleep(0);
		}
	}
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

		const PVertex& v0 = vlst[face.ivp[0]];
		const PVertex& v1 = vlst[face.ivp[1]];
		const PVertex& v2 = vlst[face.ivp[2]];

		Material& mat = materialstore.store[face.mf];

		if (mat.imagename != string("")) {
			const auto tex = texturestore.find(mat.imagename);
			//			const auto texunit = ts_512i(&tex->b[0]);
			//			auto tex_shader = TextureShader<ts_512i>(texunit);

			const auto texunit = ts_512_mipmap(&tex->b[0]);
			auto tex_shader = TextureShader<ts_512_mipmap>(texunit);
			tex_shader.setColorBuffer(cb);
			tex_shader.setDepthBuffer(db);
			tex_shader.setUV(tlst[face.iuv[0]], tlst[face.iuv[1]], tlst[face.iuv[2]]);
			tex_shader.setup(vp.width, vp.height, v0.f, v1.f, v2.f);
			draw_triangle(bin.rect, v0.f, v1.f, v2.f, tex_shader);
		}
		else {
			if (1) { //face.mf<26) {
				my_shader.setColor(vec4(mat.kd.x, mat.kd.y, mat.kd.z, 0));
				my_shader.setup(vp.width, vp.height, v0.f, v1.f, v2.f);
				draw_triangle(bin.rect, v0.f, v1.f, v2.f, my_shader);
			} else {
				wire_shader.setColor(vec4(mat.kd.x, mat.kd.y, mat.kd.z, 0));
				wire_shader.setup(vp.width, vp.height, v0.f, v1.f, v2.f);
				draw_triangle(bin.rect, v0.f, v1.f, v2.f, wire_shader);
			}
		}

		//		cout << "v1:" << v0.f << ", v2:" << v1.f << ", v3:" << v2.f << endl;

	}//faces
}
