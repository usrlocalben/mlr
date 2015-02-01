
#ifndef __MCUBE_H
#define __MCUBE_H

#include "stdafx.h"

class IsoCubes {
public:
	IsoCubes() {
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
	void set_sampler(float (*new_sampler)(const vec4&)) {
		sampler = new_sampler;
	}

	void run(Pipedata& pipe, const mat4& xform);

private:
	void run_cube(Pipedata& pipe, const mat4& xform, const vec4& pos, float scale);
	vec4 calc_normal(const vec4& p) const;
	float calc_offset(const float a, const float b, const float target) const;

	int grid_size;
	float step_size;
	float target_value;
	float (*sampler)(const vec4& p);
};

#endif //__MCUBE_H
