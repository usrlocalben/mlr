
#include "stdafx.h"

#include <vector>

#include "tri.h"
#include "vec.h"
#include "clip.h"
#include "mesh.h"
#include "render.h"
#include "texture.h"
#include "viewport.h"
#include "fragment.h"

using namespace std;

const int tile_width_in_subtiles = 6;
const int tile_height_in_subtiles = 6;

__forceinline unsigned add_one_and_wrap(const unsigned val, const unsigned max)
{
	const auto next = val + 1;
	return next == max ? 0 : next;
}

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

	device_max = ivec4(device_width, device_height, 0, 0);

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

void Binner::insert(const vec4& p1, const vec4& p2, const vec4& p3, const int idx)
{
	/*
	auto pmin = vmax(vmin(p1, vmin(p2, p3)), vec4::zero());
	auto pmax = vmin(vmax(p1, vmax(p2, p3)), itof(device_max));

	const auto x0 = int(pmin.x);
	const auto y0 = int(pmin.y);
	const auto x1 = int(pmax.x);
	const auto y1 = int(pmax.y);
	*/

	const int y0 = max((int)min(p1.y, min(p2.y, p3.y)), 0);
	const int y1 = min((int)max(p1.y, max(p2.y, p3.y)), device_height);

	const int x0 = max((int)min(p1.x, min(p2.x, p3.x)), 0);
	const int x1 = min((int)max(p1.x, max(p2.x, p3.x)), device_width);

	// compiler doesn't seem smart enough to optimize this out of the for expression??
	const int ylim = min(y1 / tileheight, device_height_in_tiles - 1);
	const int xlim = min(x1 / tilewidth, device_width_in_tiles - 1);
	const int tx0 = x0 / tilewidth;

	for (int ty = y0 / tileheight; ty <= ylim; ty++) {
		for (int tx = tx0; tx <= xlim; tx++) {
			auto& bin = bins[ty * device_width_in_tiles + tx];
			bin.faces.push_back(idx);
		}
	}
}

void Binner::sort() {
	std::sort(bins.begin(), bins.end(),
		[](Tilebin const& a, Tilebin const& b){return a.faces.size() > b.faces.size(); });
}
void Binner::unsort() {
	std::sort(bins.begin(), bins.end(),
		[](Tilebin const& a, Tilebin const& b){return a.id < b.id; });
}



Pipeline::Pipeline(const int threads)
	:threads(threads)
{
	pipes.clear();
	for (int i = 0; i < threads; i++) {
		pipes.push_back(Pipedata(i, threads));
	}
}

Pipedata::Pipedata(const int thread_number, const int thread_count)
	:thread_number(thread_number), thread_count(thread_count)
{
//	reset(0,0);
}



void Pipedata::addFace(const Face& fsrc)
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
		flst.push_back(fsrc.make_rebased(vbase, tbase, nbase));
		process_face(flst.size() - 1);
		return;
	}

	

	unsigned a_vidx[32], b_vidx[32];
	unsigned a_tidx[32], b_tidx[32];
	unsigned a_nidx[32], b_nidx[32];
	unsigned a_cnt = 3;
	for (unsigned i = 0; i < 3; i++) {
		a_vidx[i] = fsrc.ivp[i] + vbase;
		a_tidx[i] = fsrc.iuv[i] + tbase;
		a_nidx[i] = fsrc.ipn[i] + nbase;
	}


	for (int cnum = 0; cnum < 5; cnum++) { // XXX set to 6 to enable the far plane

		const int planebit = 1 << cnum;
		if (!(required_clipping & planebit)) continue; // skip plane that is ok

		bool we_are_inside;
		unsigned pi = 0;
		unsigned b_cnt = 0;

		while (1) {
			// select the next point, wrap if needed
			const auto next_pi = add_one_and_wrap(pi, a_cnt);

			const auto& cv = vlst[a_vidx[pi]];
			const auto& ct = tlst[a_tidx[pi]];
			const auto& cn = nlst[a_nidx[pi]];

			const auto& nv = vlst[a_vidx[next_pi]];
			const auto& nt = tlst[a_tidx[next_pi]];
			const auto& nn = nlst[a_nidx[next_pi]];

			if (pi == 0) {
				we_are_inside = Guardband::is_inside(planebit, cv.c);
			}
			const bool next_is_inside = Guardband::is_inside(planebit, nv.c);

			if (we_are_inside) {
				b_vidx[b_cnt] = a_vidx[pi];
				b_tidx[b_cnt] = a_tidx[pi];
				b_nidx[b_cnt] = a_nidx[pi];
				b_cnt++;
			}

			if (we_are_inside != next_is_inside) {
				we_are_inside = !we_are_inside;

				const float t = Guardband::clipLine(planebit, cv.c, nv.c);
				vlst.push_back(lerp(cv, nv, t));    b_vidx[b_cnt] = vlst.size() - 1;
				tlst.push_back(lerp(ct, nt, t));    b_tidx[b_cnt] = tlst.size() - 1;
				nlst.push_back(nlerp(cn, nn, t));    b_nidx[b_cnt] = nlst.size() - 1;
				b_cnt++;
			}

			pi = next_pi;
			if (pi == 0) break;
		}

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
		flst.push_back(make_tri(
			fsrc.mf,
			a_vidx[0], a_vidx[a], a_vidx[a + 1],
			a_tidx[0], a_tidx[a], a_tidx[a + 1],
			a_nidx[0], a_nidx[a], a_nidx[a + 1]));
		process_face(flst.size() - 1);
	}

}


__forceinline void Pipedata::addVertex(const Viewport * const vp, const vec4& src, const mat4& m)
{
	PVertex pv;
	pv.p = mat4_mul(m, src);
	pv.c = vp->to_clip_space(pv.p);
	pv.s = vp->to_screen_space(pv.c);

	float one_over_w = 1 / pv.s.w;
	pv.f = pv.s / pv.s.wwww();// ->to_device(pv.s);
	pv.f.w = one_over_w;

//	cout << "src:" << src << ", p:" << pv.p << ", c:" << pv.c << ", s:" << pv.s << ", f:" << pv.f << endl;

	pv.cf = Guardband::clipPoint(pv.c);
	vlst.push_back(pv);
	new_vcnt++;
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


void Pipedata::process_face(const int face_id)
{
	Face& f = flst[face_id];

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

	binner.insert(p1, p2, p3, face_id);
}



void Pipedata::addMeshy(Meshy& mi, const mat4& camera_inverse, const Viewport * const vp)
{
	const Mesh& mesh = *mi.mesh;

	int next_idx = thread_number + root_count;
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
		if (!vp->is_visible(tbb)) continue;

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
			addFace(face);
		}
//		for (auto& face : mesh.faces)
//			addFace(face);
		end_batch();

	}
}


void Pipeline::render(struct SOADepth& db, struct SOACanvas& cb, class MaterialStore& materialstore, class TextureStore& texturestore, std::function<void(bool)>& mark, TrueColorPixel * const __restrict target, const int target_width)
{
	auto my_thread_id = 0;
	auto thread_count = 1;

	auto& pipe = this->pipes[my_thread_id];
	for (auto& mesh : this->meshlist) {
		pipe.addMeshy(*mesh, camera_inverse, vp);
	}
	mark(true);

	index_bins();
	mark(true);

	for (auto& idx : bin_index) {

		const irect& tilerect = pipes[0].binner.bins[idx.first].rect;

		db.clear(tilerect);
		cb.clear(tilerect);
		for (int ti = 0; ti < threads; ti++) {
			pipes[ti].render(db.rawptr(), cb.rawptr(), materialstore, texturestore, vp, idx.first);
			mark(false);
		}
		convertCanvas(tilerect, target_width, target, cb.rawptr(), PostprocessNoop());
	}
}



void Pipedata::render(__m128 * __restrict db, SOAPixel * __restrict cb, MaterialStore& materialstore, TextureStore& texturestore, const Viewport * const vp, const int bin_idx)
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
	for (auto& idx : bin.faces) {
		auto& face = flst[idx];

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
			tex_shader.setup(vp->width, vp->height, v0.f, v1.f, v2.f);
			drawTri(bin.rect, v0.f, v1.f, v2.f, tex_shader);
		}
		else {
			if (1) { //face.mf<26) {
				my_shader.setColor(vec4(mat.kd.x, mat.kd.y, mat.kd.z, 0));
				my_shader.setup(vp->width, vp->height, v0.f, v1.f, v2.f);
				drawTri(bin.rect, v0.f, v1.f, v2.f, my_shader);
			} else {
				wire_shader.setColor(vec4(mat.kd.x, mat.kd.y, mat.kd.z, 0));
				wire_shader.setup(vp->width, vp->height, v0.f, v1.f, v2.f);
				drawTri(bin.rect, v0.f, v1.f, v2.f, wire_shader);
			}
		}

		//		cout << "v1:" << v0.f << ", v2:" << v1.f << ", v3:" << v2.f << endl;

	}//faces
}

