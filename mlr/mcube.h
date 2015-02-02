
#ifndef __MCUBE_H
#define __MCUBE_H

#include "stdafx.h"

extern const vec4 vertex_offset[8];
extern const int edge_connection[12][2];
extern const vec4 edge_direction[12];
extern int cube_edge_flags[256];
extern int tritable[256][16];

template<typename SURFACE_SAMPLER>
class IsoCubes {
public:
	IsoCubes(SURFACE_SAMPLER& surface) :surface(surface) {
		set_grid(16);
		set_target(48.0f);
	}
	void set_grid(const int a) {
		grid_size = a;
		step_size = 1.0f / a;
	}
	void set_target(const float a) {
		target_value = a;
	}

	void run(Pipedata& pipe, const mat4& xform)
	{
		auto scale = vec4(step_size, step_size, step_size, 1);
		auto halfscale = scale * vec4(0.5, 0.5, 0.5, 0);

		auto radius = sqrtf((step_size/2)*(step_size/2) * 3);
		last_iz = -1;
		for (int ix = 0; ix < grid_size; ix++)
		for (int iy = 0; iy < grid_size; iy++) {
			float ax = 0;
			for (int iz = 0; iz < grid_size; iz++) {
				if (ax > radius) {
					ax -= step_size;
				} else {
					auto pos = vec4(ix, iy, iz, 1) * scale;
					auto center = pos + halfscale;
					auto dist = abs(surface.sample(center));
					if (dist > radius) {
						ax = dist - radius;
					} else {
						run_cube(pipe, xform, pos, step_size, iz);
					}
				}
			}
		}
	}

	float calc_offset(const float a, const float b, const float target) const
	{
		if ( 1 ) {
			return (target - a)/(b-a);
		} else {
			double delta = b - a;
			if (delta == 0.0) {
				return 0.5f;
			} else {
				return (target - a)/delta;
			}
		}
	}

	vec4 calc_normal(const vec4& p) const
	{
		float x = sampler(p-vec4(0.01,0,0,0)) - sampler(p+vec4(0.01,0,0,0));
		float y = sampler(p-vec4(0,0.01,0,0)) - sampler(p+vec4(0,0.01,0,0));
		float z = sampler(p-vec4(0,0,0.01,0)) - sampler(p+vec4(0,0,0.01,0));
		return normalized(vec4(x,y,z,0));
	}

	/*
	vec4 calc_color(const vec4& p, const vec4& n)
	{
		auto x = n._x();
		auto y = n._y();
		auto z = n._z();
		return vec4(
			(x>0?x:0) + (y<0?-0.5*y:0) + (z<0?-0.5*z:0),
			(y>0?y:0) + (z<0?-0.5*z:0) + (x<0?-0.5*x:0),
			(z>0?z:0) + (x<0?-0.5*x:0) + (y<0?-0.5*y:0),
			1.0f
		);
	}
	*/

	vec4 calc_color(const vec4& p, const vec4& n)
	{
		return vec4(1,1,1,1);
	}

	void run_cube(Pipedata& pipe, const mat4& xform, const vec4& pos, float scale, const int this_iz)
	{
		float cube_value[8];
		vec4 edge_vertex[12];
		vec4 edge_normal[12];

		if (this_iz == last_iz+1) {
			for (int iv=0; iv<4; iv++) {
				cube_value[iv] = stored_value[iv];
			}
		} else {
			for (int iv=0; iv<4; iv++) {
				auto sample_pos = pos + vertex_offset[iv] * vec4(scale,scale,scale,1);
				cube_value[iv] = surface.sample(sample_pos);
			}
		}
		last_iz = this_iz;
		for (int iv=4; iv<8; iv++) {
			auto sample_pos = pos + vertex_offset[iv] * vec4(scale,scale,scale,1);
			stored_value[iv-4] = cube_value[iv] = surface.sample(sample_pos);
		}

		int flag_index = 0;
		for (int iv=0; iv<8; iv++) {
			if (cube_value[iv] <= target_value)
				flag_index |= 1<<iv;
		}

		auto edge_flags = cube_edge_flags[flag_index];

		if (!edge_flags) return;

		for (int edge=0; edge<12; edge++) {
			if (edge_flags & (1<<edge)) {
				auto offset = calc_offset(
					cube_value[ edge_connection[edge][0] ],
					cube_value[ edge_connection[edge][1] ],
					target_value);
				edge_vertex[edge] = pos + (vertex_offset[edge_connection[edge][0]] + vec4(offset,offset,offset,1) * edge_direction[edge]) * vec4(scale,scale,scale,1);
	//			edge_normal[edge] = calc_normal(edge_vertex[edge]);
			}
		}

		for (int tri=0; tri<5; tri++) {
			if (tritable[flag_index][3*tri] < 0) break; // end of the list
			for (int corner=0; corner<3; corner++) {
				auto vertex = tritable[flag_index][3*tri+corner];
	//			auto color = calc_color(edge_vertex[vertex], edge_normal[vertex]);
	//			pipe.glColor(color);
	//			pipe.glNormal(edge_normal[vertex]);
				pipe.glVertex(mat4_mul(xform,edge_vertex[vertex]));
	//			cout <<"added a vertex" << endl;
			}
		}
	}

private:
	int grid_size;
	float step_size;
	float target_value;
	const SURFACE_SAMPLER& surface;

	// these are not threadsafe!!
	int last_iz;
	float stored_value[4];
};


#endif //__MCUBE_H

