
#ifndef __MESH_H
#define __MESH_H

#include "stdafx.h"

#include <string>
#include <array>

#include "aligned_allocator.h"

#include "vec.h"

struct Light {
	vec4 position;
	vec4 color_diffuse;
	float power_diffuse;
	vec4 color_specular;
	float power_specular;
//	bool casts_shadows;
};


struct Material {
	vec3 ka;
	vec3 kd;
	vec3 ks;
	float specpow;
	float d;
	std::string name;
	std::string imagename;
	std::string shader;
	void print() const;
};

class MaterialStore {
public:
	void print() const;
	int find(const std::string& name) const;
	vectorsse<Material> store;
};

struct Face {
	std::array<int,3> ivp;
	std::array<int,3> iuv;
	std::array<int,3> ipn;
//	vec4 n;
	int mf;
//	int mb;

	bool backfacing;

	__forceinline Face make_rebased(const unsigned vbase, const unsigned tbase, const unsigned nbase) const {
		Face f;
		f.ivp = { { ivp[0] + vbase, ivp[1] + vbase, ivp[2] + vbase } };
		f.iuv = { { iuv[0] + tbase, iuv[1] + tbase, iuv[2] + tbase } };
		f.ipn = { { ipn[0] + nbase, ipn[1] + nbase, ipn[2] + nbase } };
		f.mf = mf;
		return f;
	}
};

__forceinline Face make_tri(
	const int mf,
	const int v1, const int v2, const int v3,
	const int t1, const int t2, const int t3,
	const int n1, const int n2, const int n3)
{
	Face f;
	f.mf = mf;
	f.ivp = { { v1, v2, v3 } };
	f.iuv = { { t1, t2, t3 } };
	f.ipn = { { n1, n2, n3 } };
	return f;
}

struct Mesh {
	vec4 bbox[8];

	vectorsse<vec4> bvp; // vertex points
//	vectorsse<vec4> bvn; // vertex normals

	vectorsse<vec4> bpn; // precalc'd normals
	vectorsse<vec4> buv; // texture coords
	vectorsse<Face> faces;

	std::string name;
	//bool solid;
	void print() const;
	void calcBounds();
};

class MeshStore {
public:

	int index_of(const std::string& name) const {
		for ( unsigned i=0; i<store.size(); i++ ) {
			if ( store[i].name == name ) {
				return i;
			}
		}
		return 0;
	}

	const Mesh& find(const std::string& name) const { 
		return store[index_of(name)];
	}

	void print() const;
	void loadDirectory(const std::string& prepend, MaterialStore& materialstore, class TextureStore& texturestore);

	vectorsse<Mesh> store;
};


//local rotate x,y,z
//scale x,y,z
//rotate x,y,z
//translate x,y,z
//extrude-normal float
//count int, min=1
class Meshy {
public:
	const Mesh * mesh;
	Meshy(const Mesh * const mesh) : mesh(mesh) {}
	virtual mat4 begin(const int t) = 0;
	virtual bool next(const int t, mat4& m) = 0;
	__forceinline void* operator new[]   (size_t x){ return _aligned_malloc(x, 16); }
	__forceinline void* operator new     (size_t x){ return _aligned_malloc(x, 16); }
	__forceinline void  operator delete[](void*  x) { if (x) _aligned_free(x); }
	__forceinline void  operator delete  (void*  x) { if (x) _aligned_free(x); }
};

class MeshySet : public Meshy {
public:

	MeshySet(const Mesh * const mesh, mat4& xform) :Meshy(mesh), xform(xform) {}
	virtual mat4 begin(const int t){
		int& idx = this->idx[t];
		mat4& xform = this->xform[t];
		idx = 0;
		return mat4::ident();
	}
	virtual bool next(const int t, mat4& m) {
		int& idx = this->idx[t];
		mat4& xform = this->xform[t];
		if (idx == 0) {
			m = xform;
			idx++;
			return true;
		} else {
			return false;
		}
	}
private:
	std::array<int,16> idx;
	std::array<mat4,16> xform;
};


class MeshyTranslate : public Meshy {
public:
	MeshyTranslate(Meshy& inmesh, mat4& xform) :Meshy(inmesh.mesh), in(inmesh), xform(xform) {}

	virtual mat4 begin(const int t) {
		int& idx = this->idx[t];
		mat4& xform = this->xform[t];
		idx = 0;
		in.begin();
		return mat4::ident();
	}
	virtual bool next(const int t, mat4& m) {
		int& idx = this->idx[t];
		mat4& xform = this->xform[t];

		mat4 in_mat;
		auto alive = in.next(t, in_mat);
		if (alive) {
			mat4_mul(xform, in_mat, m);
			idx++;
			return true;
		} else {
			return false;
		}
	}

private:
	Meshy& in;

	std::array<mat4,16> xform;
	std::array<int,16> idx;
};

class MeshyMultiply : public Meshy {
public:
	MeshyMultiply(Meshy& inmesh, int many, vec3& translate, vec3& scale) :Meshy(inmesh.mesh), in(inmesh), many(many), translate(translate), scale(scale) {}

	virtual mat4 begin(const int t) {
		int& idx = this->idx[t];
		mat4& xform = this->xform[t];

		idx = 0; 
		calc(idx, xform);
		in.begin();
		return mat4::ident();
	}
	virtual bool next(const int t, mat4& m) {
		int& idx = this->idx[t];
		mat4& xform = this->xform[t];

		if (idx == many) return false;

		mat4 in_mat;
		auto alive = in.next(t, in_mat);
		if (!alive) {
			idx++;
			calc(idx, xform);
			if (idx == many) return false;
			in.begin(t);
			in.next(t, in_mat);
		}

		mat4_mul(xform, in_mat, m);
		return true;
	}

	void calc(const int idx, mat4& xform) {
		mat4 tr = mat4_translate(translate*vec3(float(idx)));
		mat4 sc = mat4_scale(vec3(1.0f) + scale*vec3(float(idx)));
		mat4_mul(tr, sc, xform);
	}
private:
	Meshy& in;
	int many;
	vec3 translate;
	vec3 scale;

	std::array<mat4,16> xform;
	std::array<int,16> idx;
};

class MeshyCenter : public Meshy {
public:
	MeshyCenter(Meshy& inmesh, const bool center_x, const bool center_y, const bool center_z, const bool y_on_floor) :Meshy(inmesh.mesh), in(inmesh), center_x(center_x), center_y(center_y), center_z(center_z), y_on_floor(y_on_floor){}

	virtual mat4 begin(const int t) {
		int& idx = this->idx[t];
		mat4& xform = this->xform[t];

		float minx,miny,minz;
		float maxx,maxy,maxz;
		in.begin(t);
		mat4 in_mat;
		in.next(t, in_mat);
		vec4 xb = mat4_mul(in_mat, mesh->bbox[0]);
		minx = maxx = xb.x;
		miny = maxy = xb.y;
		minz = maxz = xb.z;

		in.begin(t);
		while ( in.next(t, in_mat) ) {
			for (unsigned i=0; i<8; i++ ){
				vec4 xb = mat4_mul(in_mat, mesh->bbox[i]);
				minx = std::min(xb.x, minx);    maxx = std::max(xb.x, maxx);
				miny = std::min(xb.y, miny);    maxy = std::max(xb.y, maxy);
				minz = std::min(xb.z, minz);    maxz = std::max(xb.z, maxz);
			}
		}

//		cout << "min:" << vec3(minx, miny, minz) << endl;
//		cout << "max:" << vec3(maxx, maxy, maxz) << endl;
		// calculate mat4 based on mins and maxs
		const float move_x = center_x ? -(minx + (maxx - minx) / 2) : 0;
		float move_y;
		if (!y_on_floor) {
			move_y = center_y ? -(miny + (maxy - miny) / 2) : 0;
		} else {
			move_y = center_y ? 0 - miny : 0;
		}
		const float move_z = center_z ? -(minz + (maxz - minz) / 2) : 0;
//		cout << "move:" << vec3(move_x, move_y, move_z) << endl;

		xform = mat4_translate({ move_x, move_y, move_z, 1 });

		idx = 0;
		in.begin(t);
		return mat4::ident();
	}

	virtual bool next(const int t, mat4& m) {
		int& idx = this->idx[t];
		mat4& xform = this->xform[t];

		mat4 in_mat;
		auto alive = in.next(t, in_mat);
		if (alive) {
//			m = mat4_mul(xform, in_mat);
			mat4_mul(xform, in_mat, m);
			idx++;
			return true;
		} else {
			return false;
		}
	}
private:
	Meshy& in;
	const bool center_x;
	const bool center_y;
	const bool center_z;
	const bool y_on_floor;

	std::array<mat4,16> xform;
	std::array<int,16> idx;
};

#endif //__MESH_H

