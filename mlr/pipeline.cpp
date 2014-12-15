
#include "stdafx.h"

#include "aligned_allocator.h"

#include "vec.h"
#include "pipeline.h"

struct FaceIdx {
	unsigned v;
	unsigned t;
	unsigned n;
};

struct Face {
	std::array<FaceIdx,3> idx;

	//std::array<unsigned,3> edgelist;
	//std::array<unsigned,3> edgefaces;

	//vec4 nx,ny,nz;
	//vec4 px,py,pz;

	unsigned mf;
	unsigned mb;

	vec4 normal;
};


template <typename VERTEX_PROGRAM>
void loadVertex( VERTEX_PROGRAM& vertexprogram, const vec4& vsrc, const vec4& nsrc, const int i ) {
	
	Vertex& dst = vb[ vbase + i ];

	if ( isLoaded(dst) ) return;

	vec4 vvp = vsrc;
	vec4 vvn = nsrc;

	vertexprogram.proc(i, vvp, vvn);

	dst.p = vvp;
	dst.c = vp->to_clip_space(dst.p);
	dst.s = vp->to_screen_space(dst.c);

	dst.clipflags = hc_clipflags_guardband(dst.s);
	dst.f = vp->prep(dst.s);

	dst.n = vvn;
	
	setLoaded(dst);
}


template<bool enable_renormalize>
void addFace(const Face& fsrc) {

}



mesh out {

beginBatch();

allocateVerts( mesh vert count );
loadNormals( my normals )
load_uv(my uv )
mark()

for ( auto& face : my faces ) {
	SomeVertProgram vp(object_to_camera, shader_vars);
	pipe.load_vertex1( vp, myverts[face.idx[0].v, myvertnorms[face.idx[0].v], face.idx[0].v );
	pipe.load_vertex1( vp, myverts[face.idx[1].v, myvertnorms[face.idx[1].v], face.idx[1].v );
	pipe.load_vertex1( vp, myverts[face.idx[2].v, myvertnorms[face.idx[2].v], face.idx[2].v );
	pipe.add_face1<true>(0, face, mat? );
}
endBatch();



}
