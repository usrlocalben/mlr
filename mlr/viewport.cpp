
#include "stdafx.h"

#include <iostream>
#include <boost/format.hpp>

#include <conio.h>

#include "viewport.h"


using namespace std;
using namespace boost;


/*
* standard opengl perspective transform
*/
mat4 create_opengl_p(const float l, const float r, const float t, const float b, const float n, const float f)
{
	return mat4_init(
		(2*n)/(r-l)      ,0         ,(r+l)/(r-l)        ,0
	        ,0      ,(2*n)/(t-b)    ,(t+b)/(t-b)        ,0
	        ,0           ,0      ,(-(f+n))/(f-n)  ,(-2*f*n)/(f-n)
	        ,0           ,0            ,-1              ,0
	);
}


/*
* standard opengl perspective transform
*/
mat4 create_opengl_ortho(const float left, const float right, const float top, const float bottom, const float near, const float far)
{
	return mat4_init(
	 	   2/(right-left)               ,0                       ,0       ,-(right+left)/(right-left)
		         ,0               ,2/(top-bottom)                ,0       ,-(top+bottom)/(top-bottom)
		         ,0                     ,0                 ,-2/(far-near)   ,-(far+near)/(far-near)
		         ,0                     ,0                       ,0                    ,1
	);
}


/*
* perspective transform with infinite far clip distance
* from the paper "Practical and Robust Stenciled Shadow Volumes for Hardware-Accelerated Rendering" (2002, nvidia)
*/
mat4 create_opengl_pinf(const float left, const float right, const float top, const float bottom, const float near, const float far)
{
	return mat4_init(
		(2*near)/(right-left)           ,0           , (right+left)/(right-left)            ,0
		         ,0           ,(2*near)/(top-bottom) , (top+bottom)/(top-bottom)            ,0
		         ,0                     ,0                       ,-1                     ,-2*near
		         ,0                     ,0                       ,-1                        ,0
	);
}




Viewport::Viewport(const unsigned sx, const unsigned sy, const float aspect, const float fovy)
	:width(sx),
	height(sy),
	//aspect = sx/sy // 4:3 is 1.333
	aspect(aspect),
	fovy(fovy),
	znear(1),
	zfar(500)
{
	const auto DEG2RAD = PI / 180.0f;
	const auto top = znear * tanf(fovy/2 * DEG2RAD);
	const auto bottom = -top;
	const auto left = bottom * aspect;
	const auto right = top * aspect;

	mp = create_opengl_p(left, right, top, bottom, znear, zfar);

	const auto x0 = 0;
	const auto y0 = 0;
	const auto y1 = sy;
	const auto x1 = sx;

	auto md_yfix = mat4_init(
	       1,        0,       0,       0,
		   0,       -1,       0,       0,
		   0,        0,       1,       0,
		   0,        0,       0,       1);

	auto md_scale = mat4_init(
		width/2,     0,       0,       0,
		   0,    height/2,    0,       0,
		   0,        0,       1,       0, //zfar-znear,  0,
		   0,        0,       0,       1);

	auto md_origin = mat4_init(
		   1,        0,       0,  x0+width /2,
		   0,        1,       0,  y0+height/2,
		   0,        0,       1,       0,
		   0,        0,       0,       1);

	md = mat4_mul(md_origin, mat4_mul(md_scale, md_yfix));

	frust[0] = Plane::from_origin({ mp.f[0][3] + mp.f[0][0], mp.f[1][3] + mp.f[1][0], mp.f[2][3] + mp.f[2][0], mp.f[3][3] + mp.f[3][0] });	// left		(facing right)
	frust[1] = Plane::from_origin({ mp.f[0][3] + mp.f[0][1], mp.f[1][3] + mp.f[1][1], mp.f[2][3] + mp.f[2][1], mp.f[3][3] + mp.f[3][1] });	// bottom	(facing up)
	frust[2] = Plane::from_origin({ mp.f[0][3] + mp.f[0][2], mp.f[1][3] + mp.f[1][2], mp.f[2][3] + mp.f[2][2], mp.f[3][3] + mp.f[3][2] });	// front	(facing back)
	frust[3] = Plane::from_origin({ mp.f[0][3] - mp.f[0][0], mp.f[1][3] - mp.f[1][0], mp.f[2][3] - mp.f[2][0], mp.f[3][3] - mp.f[3][0] });	// right	(facing left)
	frust[4] = Plane::from_origin({ mp.f[0][3] - mp.f[0][1], mp.f[1][3] - mp.f[1][1], mp.f[2][3] - mp.f[2][1], mp.f[3][3] - mp.f[3][1] });	// top		(facing down)
	frust[5] = Plane::from_origin({ mp.f[0][3] - mp.f[0][2], mp.f[1][3] - mp.f[1][2], mp.f[2][3] - mp.f[2][2], mp.f[3][3] - mp.f[3][2] });	// back		(facing front)
}


void Viewport::test()
{
	cout << "plane 0: " << frust[0] << " - left, facing right" << endl;
	cout << "plane 1: " << frust[1] << " - right, facing left" << endl;
	cout << "plane 2: " << frust[2] << " - bottom, facing up" << endl;
	cout << "plane 3: " << frust[3] << " - top, facing down" << endl;
	cout << "plane 4: " << frust[4] << " - near, facing away" << endl;
	cout << "plane 5: " << frust[5] << " - far, facing towards" << endl;

	vec4 test_a(0, 0, -20, 1);  // point inside
	vec4 test_b(0, 1000, -200, 1); // point outside, far to the left
	auto& plane = frust[3];

	cout << "test point inside: " << test_a << endl;
	cout << "test point outside: " << test_b << endl;

	float test_dist = length(test_b - test_a);

	const float dist_a_eyespace = plane.distance(test_a);
	const float dist_b_eyespace = plane.distance(test_b);

	float t;
	vec4 test_inter;
	plane.clipLine(test_a, test_b, test_inter, t);

	cout << "distance between eye points: " << format("%11.4f") % test_dist << endl;
	cout << " plane distance for a: " << format("%11.4f") % dist_a_eyespace << endl;
	cout << " plane distance for b: " << format("%11.4f") % dist_b_eyespace << endl;
	cout << endl;

	cout << "intersection at (" << format("%11.4f") % t << ") along path from a->b" << endl;
	cout << "intersection: " << test_inter << endl;
	
	vec4 test_a_clipspace = this->eye_to_clip(test_a);
	vec4 test_b_clipspace = this->eye_to_clip(test_b);
	vec4 test_i_clipspace = this->eye_to_clip(test_inter);

	cout << "clipspace a: " << test_a_clipspace << endl;
	cout << "clipspace b: " << test_b_clipspace << endl;
	cout << "clipspace i: " << test_i_clipspace << endl;

	float tclip;
	{
		vec4 a = test_a_clipspace;
		vec4 b = test_b_clipspace;
		tclip = (a.w + a.x) / ((a.w + a.x) - (b.w + b.x));
	}

	cout << "distance to intersection using clipspace method: " << format("%11.4f") % tclip << endl;
	cout << endl;

	vec4 test_a_screenspace = this->clip_to_screen(test_a_clipspace);
	vec4 test_b_screenspace = this->clip_to_screen(test_b_clipspace);
	vec4 test_i_screenspace = this->clip_to_screen(test_i_clipspace);
	//test_a_screenspace /= test_a_clipspace.w;
	//test_b_screenspace /= test_b_clipspace.w;
	test_i_screenspace /= test_i_screenspace.w;
	cout << "screenspace a: " << test_a_screenspace << endl;
	cout << "screenspace b: " << test_b_screenspace << endl;
	cout << "screenspace i: " << test_i_screenspace << endl;

	auto test_i_framespace = test_i_screenspace + xyfix;
	cout << "framespace i: " << test_i_framespace << endl;

	float tclip2;
	{
		vec4 a = test_a_screenspace;
		vec4 b = test_b_screenspace;
		tclip2 = ((a.w*512.0f) + a.x) / (((a.w*512.0f) + a.x) - ((b.w*512.0f) + b.x));
	}

	cout << "distance to intersection using screenspace method: " << format("%11.4f") % tclip2 << endl;


	cout << "--------------" << endl;

	vec4 sim1(-10240.0f, 0, 19.800f, 20.000f);
	bool sim1_inside = sim1.x + (sim1.w * 512.0f) >= 0;

	cout << sim1 << endl;
	cout << "inside? " << (sim1_inside ? "yes" : "no") << endl;
	_getch();
	//exit(1);
}
