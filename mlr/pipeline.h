
#include "stdafx.h"

#include "vec.h"

struct PipeVertex {
	vec4 p; // eyespace
	vec4 n; // normal
	vec4 c; // clipspace
	vec4 s; // screenspace
	vec4 f; // finalized 2d x, y, "z", 1/w
	unsigned clipflags;
	unsigned useq;
};

class Pipeline {

public:
	Pipeline();
	~Pipeline();

	void reset();
	void beginBatch();
	void mark();
	void endBatch();

private:
	vectorsse<Vertex> vb;
	vectorsse<vec2> uvb;
	vectorsse<vec4> nb;

	vector<Face> faces;
	vector<bool> backfacing;

	int batch_in_progress;

	int useq; // "loaded" token
	bool isLoaded(const Vertex& v) const { return v.useq == this->useq; }
	void setLoaded(Vertex& v) { v.useq = this->useq; }

};
