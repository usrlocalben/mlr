
#include "stdafx.h"

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
	for ( auto& vertex : bvp ) {
		bvn.push_back(vec4::zero());
	}

	for ( auto& face : faces ) {
		vec4 a = bvp[face.ivp[2]] - bvp[face.ivp[0]];
		vec4 b = bvp[face.ivp[1]] - bvp[face.ivp[0]];
		vec4 n = cross(b,a);
		face.n = normalized(n);

		bvn[face.ivp[0]] += n;
		bvn[face.ivp[1]] += n;
		bvn[face.ivp[2]] += n;
	}

	for ( auto& vertex_normal : bvn ) {
		vertex_normal = normalized(vertex_normal);
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
	for ( unsigned i=0; i<store.size(); i++ ) {
		if ( store[i].name == name )
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
