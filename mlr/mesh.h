
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
	bool casts_shadows;
};


struct Material {
	vec3 ka;
	vec3 kd;
	vec3 ks;
	float specpow;
	float d;
	int pass;
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

struct PFace {
	std::array<int,3> ivp;
	std::array<int,3> iuv;
	std::array<int,3> ipn;
//	vec4 n;
	int mf;
//	int mb;
};

struct Face {
	std::array<int,3> ivp;
	std::array<int,3> iuv;
	std::array<int,3> ipn;
	vec4 n;
	int mf;
//	int mb;

	int edgelist[3];
	int edgefaces[3];
	vec4 nx,ny,nz;
	vec4 px,py,pz;

	__forceinline PFace make_rebased(const unsigned vbase, const unsigned tbase, const unsigned nbase) const {
		PFace f;
		f.ivp = { { ivp[0] + vbase, ivp[1] + vbase, ivp[2] + vbase } };
		f.iuv = { { iuv[0] + tbase, iuv[1] + tbase, iuv[2] + tbase } };
		f.ipn = { { ipn[0] + nbase, ipn[1] + nbase, ipn[2] + nbase } };
		f.mf = mf;
		return f;
	}
};

__forceinline PFace make_tri(
	const int mf,
	const int v1, const int v2, const int v3,
	const int t1, const int t2, const int t3,
	const int n1, const int n2, const int n3)
{
	PFace f;
	f.mf = mf;
	f.ivp = { { v1, v2, v3 } };
	f.iuv = { { t1, t2, t3 } };
	f.ipn = { { n1, n2, n3 } };
	return f;
}

struct Mesh {
	vec4 bbox[8];

	vectorsse<vec4> bvp; // vertex points
	vectorsse<vec4> bvn; // vertex normals

	vectorsse<vec4> bpn; // precalc'd normals
	vectorsse<vec4> buv; // texture coords
	vectorsse<Face> faces;

	std::string name;

	bool solid;
	std::string message;

	void print() const;
	void calcBounds();
	void calcNormals();
	void assignEdges();
};

class MeshStore {
public:

	int index_of(const std::string& name) const {
		for (unsigned i=0; i<store.size(); i++) {
			if (store[i].name == name) {
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


#endif //__MESH_H

