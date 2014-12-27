
#include "stdafx.h"

#include <tuple>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/algorithm/string.hpp>

#include "utils.h"
#include "mesh.h"
#include "vec.h"

using namespace std;
using boost::algorithm::trim;

struct svec3 {
	float x, y, z;
	void zero() { x = y = z = 0; }
	vec4 xyz1() const { return vec4(x,y,z,1); }
	vec4 xyz0() const { return vec4(x,y,z,0); }
};
struct svec2 {
	float x, y;
	void zero() { x = y = 0; }
	vec4 xy00() const { return vec4(x,y,0,0)+vec4(100.0f,100.0f,0,0); }
};
struct faceidx { int vv, vt, vn; };

svec2 ss_get_vec2(stringstream& ss) {
	svec2 sv;
	ss >> sv.x >> sv.y;
	return sv;
}
svec3 ss_get_vec3(stringstream& ss) {
	svec3 sv;
	ss >> sv.x >> sv.y >> sv.z;
	return sv;
}
faceidx to_faceidx(const string& data) {
	auto tmp = explode(data, '/'); // "nn/nn/nn" or "nn//nn", 1+ indexed!!
	return{
		tmp[0].length() ? std::stol(tmp[0])-1 : 0, // vv
		tmp[1].length() ? std::stol(tmp[1])-1 : 0, // vt
		tmp[2].length() ? std::stol(tmp[2])-1 : 0  // vn
	};
}

vector<string> file_to_lines(const string& filename)
{
	ifstream f(filename);
	vector<string> data;

	string ln;
	while ( !getline(f,ln).eof() ) {
		data.push_back(ln);
	}

	return data;
}


struct ObjMaterial {
	string name;
	string texture;
	svec3 ka;
	svec3 kd;
	svec3 ks;
	float specpow;
	float density;

	void reset() {
		name = "$not set$";
		texture = "";
		ka.zero();
		kd.zero();
		ks.zero();
		specpow = 1;
		density = 1;
	}

	Material to_material() const {
		Material mm;
		mm.ka = vec3(ka.x,ka.y,ka.z);
		mm.kd = vec3(kd.x,kd.y,kd.z);
		mm.ks = vec3(ks.x,ks.y,ks.z);
		mm.specpow = specpow;
		mm.d = density;
		mm.name = name;
		mm.imagename = texture;
		mm.shader = "obj";
		return mm;
	}
};




class MtlLoader {

	ObjMaterial m;

	MaterialStore mlst;

	void push() {
		m.kd.x = powf(m.kd.x, 2.2f);
		m.kd.y = powf(m.kd.y, 2.2f);
		m.kd.z = powf(m.kd.z, 2.2f);
		mlst.store.push_back(m.to_material());
	}

public:
	MaterialStore load(const string& fn);
};


MaterialStore MtlLoader::load(const string& fn)
{
	auto lines = file_to_lines(fn);

	m.reset();
	for ( auto& line : lines ) {

		auto tmp = line.substr(0, line.find('#'));
		trim(tmp);
		if (tmp.size() == 0) continue;
		stringstream ss(tmp, stringstream::in);

		string cmd;
		ss >> cmd;
		if (cmd == "newmtl") {  //name
			if (m.name != "$none$") {
				push();
				m.reset();
			}
			ss >> m.name;
		} else if (cmd == "Ka") { // ambient color
			m.ka = ss_get_vec3(ss);
		} else if (cmd == "Kd") { // diffuse color
			m.kd = ss_get_vec3(ss);
		} else if (cmd == "Ks") { // specular color
			m.ks = ss_get_vec3(ss);
		} else if (cmd == "Ns") { // phong exponent
			ss >> m.specpow;
		} else if (cmd == "d") { // opacity ("d-issolve")
			ss >> m.density;
		} else if (cmd == "map_Kd") { //diffuse texturemap
			ss >> m.texture;
		}
	}
	if (m.name != "$none$") {
		push();
	}

	return mlst;
}


std::tuple<Mesh,MaterialStore> loadObj(const string& prepend, const string& fn)
{
	cout << "---- loading [" << fn << "]:" << endl;

	MtlLoader mtlloader;
	auto lines = file_to_lines(prepend+fn);

	string group_name("defaultgroup");
	int material_idx;

	MaterialStore materials;
	Mesh mesh;

	mesh.name = fn;

	for ( auto& line : lines ) {

		auto tmp = line.substr(0, line.find('#'));
		trim(tmp);
		if (tmp.size() == 0) continue;
		stringstream ss(tmp, stringstream::in);

		string cmd;
		ss >> cmd;
		if (cmd == "mtllib") { // material library
			string mtlfn;
			ss >> mtlfn;
			cout << "material library is [" << mtlfn << "]\n";
			MtlLoader mtlloader;
			materials = mtlloader.load(prepend + mtlfn);
		} else if (cmd == "g") { // group
			ss >> group_name;
		} else if (cmd == "usemtl") { // material setting
			string material_name;
			ss >> material_name;
			material_idx = materials.find(material_name);
		} else if (cmd == "v") { // vertex
			mesh.bvp.push_back(ss_get_vec3(ss).xyz1());
		} else if (cmd == "vn") { // vertex normal
			mesh.bpn.push_back(ss_get_vec3(ss).xyz0());
		} else if (cmd == "vt") { // vertex uv
			mesh.buv.push_back(ss_get_vec2(ss).xy00());
		} else if (cmd == "f") { // face indices
			string data = line.substr(line.find(' ') + 1, string::npos);
			auto faces = explode(data, ' ');
			vector<faceidx> points;
			for (auto& facechunk : faces) {
				points.push_back(to_faceidx(facechunk));
			}
			for (unsigned i = 0; i < points.size() - 2; i++){
				Face fd;
				fd.mf = material_idx;
				fd.ivp = { { points[0].vv, points[i + 1].vv, points[i + 2].vv } };
				fd.iuv = { { points[0].vt, points[i + 1].vt, points[i + 2].vt } };
				fd.ipn = { { points[0].vn, points[i + 1].vn, points[i + 2].vn } };
				mesh.faces.push_back(fd);
				// XXX group_name is unused
			}
		}
	}

	mesh.calcBounds();
	mesh.calcNormals();
	mesh.assignEdges();
	return std::tuple<Mesh,MaterialStore>(mesh, materials);
}


