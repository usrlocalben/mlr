
#include "stdafx.h"

#include <map>
#include <vector>
#include <string>
#include <iostream>

#include "obj.h"
#include "mesh.h"
#include "utils.h"
#include "texture.h"

using namespace std;


void Mesh::print() const
{
	cout << "mesh[" << this->name << "]:" << endl;
	cout << "  vert(" << this->bvp.size() << "), normal(" << this->bpn.size() << "), uv(" << this->buv.size() << ")" << endl;
	cout << "  faces(" << this->faces.size() << ")" << endl;
}

void Mesh::calcBounds()
{
	vec4 pmin = this->bvp[0];
	vec4 pmax = pmin;
	for (auto& item : this->bvp) {
		pmin = vmin(pmin, item);
		pmax = vmax(pmax, item);
	}

	bbox[0] = { pmin.x, pmin.y, pmin.z, 1 };
	bbox[1] = { pmin.x, pmin.y, pmax.z, 1 };
	bbox[2] = { pmin.x, pmax.y, pmin.z, 1 };
	bbox[3] = { pmin.x, pmax.y, pmax.z, 1 };
	bbox[4] = { pmax.x, pmin.y, pmin.z, 1 };
	bbox[5] = { pmax.x, pmin.y, pmax.z, 1 };
	bbox[6] = { pmax.x, pmax.y, pmin.z, 1 };
	bbox[7] = { pmax.x, pmax.y, pmax.z, 1 };
}


void Mesh::calcNormals()
{
	bvn.clear();
	for (auto& vertex : bvp) {
		bvn.push_back(vec4::zero());
	}

	for (auto& face : faces) {
		vec4 a = bvp[face.ivp[2]] - bvp[face.ivp[0]];
		vec4 b = bvp[face.ivp[1]] - bvp[face.ivp[0]];
		vec4 n = cross(b,a);
		face.n = normalized(n);

		bvn[face.ivp[0]] += n;
		bvn[face.ivp[1]] += n;
		bvn[face.ivp[2]] += n;
	}

	for (auto& vertex_normal : bvn) {
		vertex_normal = normalized(vertex_normal);
	}
}

struct _edgedata {
	int face_id_a;
	int face_id_b;
	int edge_id;
};

unsigned make_edge_key(const unsigned vidx1, const unsigned vidx2)
{
	if (vidx1 < vidx2)
		return vidx1<<16 | vidx2;
	else
		return vidx2<<16 | vidx1;
}

void Mesh::assignEdges()
{
	map<unsigned,_edgedata> edgemap;
	int next_id = 0;

	solid = true;

	for (int i=0; i<faces.size(); i++) {
		Face& face = faces[i];
		for (int ei=0; ei<3; ei++) {
			const unsigned edgekey = make_edge_key(face.ivp[ei], face.ivp[(ei+1)%3]);
			if (edgemap.count(edgekey) == 0) {
				int this_edge_id = next_id++;
				face.edgelist[ei] = this_edge_id;
				edgemap[edgekey] = { i, -1, this_edge_id };
			} else {
				if (edgemap[edgekey].face_id_b != -1) {
					solid = false;
					message = "edge over shared";
					return;
				}
				edgemap[edgekey].face_id_b = i;
				face.edgelist[ei] = edgemap[edgekey].edge_id;
			}
		}//foreach edges
	}//foreach faces


	for (int i=0; i<faces.size(); i++) {
		Face& face = faces[i];
		for (int ei=0; ei<3; ei++) {
			const unsigned edgekey = make_edge_key(face.ivp[ei], face.ivp[(ei+1)%3]);
			auto& edgedata = edgemap[edgekey];
			if (edgedata.face_id_b == -1) {
				solid = false;
				message = "open edge";
				return;
			}
			face.edgefaces[ei] = edgedata.face_id_a == i ? edgedata.face_id_b : edgedata.face_id_a;
		}
	}//foreach faces

	for (auto& face : faces) {

		const auto& adj1 = faces[face.edgefaces[0]];
		const auto& adj2 = faces[face.edgefaces[1]];
		const auto& adj3 = faces[face.edgefaces[2]];

		const auto& pi = bvp[ face.ivp[0] ];
		const auto& p1 = bvp[ adj1.ivp[0] ];
		const auto& p2 = bvp[ adj2.ivp[0] ];
		const auto& p3 = bvp[ adj3.ivp[0] ];

		const auto& ni = face.n;
		const auto& n1 = adj1.n;
		const auto& n2 = adj2.n;
		const auto& n3 = adj3.n;

		face.px = vec4( pi.x, p1.x, p2.x, p3.x );
		face.py = vec4( pi.y, p1.y, p2.y, p3.y );
		face.pz = vec4( pi.z, p1.z, p2.z, p3.z );

		face.nx = vec4( ni.x, n1.x, n2.x, n3.x );
		face.ny = vec4( ni.y, n1.y, n2.y, n3.y );
		face.nz = vec4( ni.z, n1.z, n2.z, n3.z );
	}
}


void MaterialStore::print() const
{
	cout << "----- material pack -----" << endl;
	for (auto& item : this->store) {
		item.print();
	}
	cout << "---  end of materials ---" << endl;
}


int MaterialStore::find(const std::string& name) const {
	for (unsigned i=0; i<store.size(); i++) {
		if (store[i].name == name)
			return i;
	}
	return 0;
}


void Material::print() const
{
	cout << "material[" << this->name << "]:" << endl;
	cout << "  ka" << this->ka << ", ";
	cout << "kd" << this->kd << ", ";
	cout << "ks" << this->ks << endl;
	cout << "  specpow(" << this->specpow << "), density(" << this->d << ")" << endl;
	cout << "  texture[" << this->imagename << "]" << endl;

}


void MeshStore::loadDirectory(const string& prefix, MaterialStore& materialstore, TextureStore& texturestore)
{
	const string spec = "*.obj";

	for (auto& fn : fileglob(prefix + spec)) {
		//cout << "loading mesh [" << prefix << "][" << fn << "]" << endl;

		Mesh load_mesh;
		MaterialStore load_materials;
		tie(load_mesh, load_materials) = loadObj(prefix, fn);

		unsigned material_base_idx = materialstore.store.size();

		for (auto& item : load_materials.store) {
			if (item.imagename != "") {
				texturestore.loadAny(prefix, item.imagename);
			}
			materialstore.store.push_back(item);
		}

		for (auto& item : load_mesh.faces)
			item.mf += material_base_idx;

		store.push_back(load_mesh);
	}
}
