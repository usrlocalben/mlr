
#include "stdafx.h"

#include <iostream>
#include <boost/format.hpp>

#include "conio.h"

#include "vec.h"

using namespace std;
using namespace boost;


ostream& operator<<(ostream& os, const vec4& v) {
	os << boost::format("(%.4f,%.4f,%.4f,%.4f)") % v.x % v.y % v.z % v.w;
	return os;
}
ostream& operator<<(ostream& os, const vec3& v) {
	os << boost::format("(%.4f,%.4f,%.4f)") % v.x % v.y % v.z;
	return os;
}





mat4 mat4_inverse(const mat4& src)
{
	float inv[16], det;
	const float *m = reinterpret_cast<const float*>(src.f);

	inv[0] = m[ 5] * m[10] * m[15] -
	         m[ 5] * m[11] * m[14] -
	         m[ 9] * m[ 6] * m[15] +
	         m[ 9] * m[ 7] * m[14] +
	         m[13] * m[ 6] * m[11] -
	         m[13] * m[ 7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	//MY_ASSERT(det != 0);
	//    if (det == 0)
	//        return false;

	__m128 idet = _mm_set1_ps(1.0 / det);

	mat4 invOut(inv);
	invOut.m1 = _mm_mul_ps(invOut.m1, idet);
	invOut.m2 = _mm_mul_ps(invOut.m2, idet);
	invOut.m3 = _mm_mul_ps(invOut.m3, idet);
	invOut.m4 = _mm_mul_ps(invOut.m4, idet);

	//    for (i = 0; i < 16; i++)
	//        invOut[i] = inv[i] * det;

	return invOut;
}





/*
 * distance v is from plane p
 */
float Plane::distance(const vec4 &v) const
{
	if (0) {
		cout << endl;
		cout << "Distance Test:" << endl;
		cout << "point: " << v << endl;
		cout << "plane ABCD: " << n << endl;
//		cout << "plane   D: " << format("%11.4f") % this->n.w << endl;

		const float xx = dot(this->n, v);
		cout << "dot3(n,p) = " << format("%11.4f") % xx << endl;
//		cout << " plus dist: " << format("%11.4f") % (xx+this->n.w) << endl;
		cout << endl;
		_getch();
	}
	return dot(this->n, v); // +this->d;
}


Plane Plane::from_origin(const vec4& pos)
{
	Plane p;
	p.n = pos;
	const float len = sqrt(p.n.x*p.n.x + p.n.y*p.n.y + p.n.z*p.n.z);
	p.n.x /= len;
	p.n.y /= len;
	p.n.z /= len;
	p.n.w /= len;
//	p.d = p.n.w;
//	p.n.w = 0;
	return p;
}


ostream& operator<<(ostream& os, const Plane& p) {
	//os << "dist(" << boost::format("% 11.4f") % p.d << "), (" << boost::format("% 11.4f,% 11.4f,% 11.4f") % p.n.x % p.n.y % p.n.z << ")";
	os << "plane" << p.n;
	return os;
}


__forceinline void mat4_print(mat4& r) {
	auto v1 = vec4(r.v[0]);
	auto v2 = vec4(r.v[1]);
	auto v3 = vec4(r.v[2]);
	auto v4 = vec4(r.v[3]);
	cout << "[" << boost::format("% 11.4f,% 11.4f,% 11.4f,% 11.4f") % v1.x % v2.x % v3.x % v4.x << "]" << endl;
	cout << "[" << boost::format("% 11.4f,% 11.4f,% 11.4f,% 11.4f") % v1.y % v2.y % v3.y % v4.y << "]" << endl;
	cout << "[" << boost::format("% 11.4f,% 11.4f,% 11.4f,% 11.4f") % v1.z % v2.z % v3.z % v4.z << "]" << endl;
	cout << "[" << boost::format("% 11.4f,% 11.4f,% 11.4f,% 11.4f") % v1.w % v2.w % v3.w % v4.w << "]" << endl;
}


/*
* same thing but input is a normalized direction
* instead of a point to look at
*/
mat4 mat4_look_from_towards(const vec4& from, const vec4& towards)
{
	vec4 dir;
	vec4 up;
	vec4 right;

	// build ->vector from 'from' to 'to'
	dir = towards * -1;
	//	dir = normalized(dir); // assume it is a unit

	// build a temporary 'up' vector
	if (fabs(dir.x)<FLT_EPSILON && fabs(dir.z)<FLT_EPSILON) {
		/*
		* special case where dir & up would be degenerate
		* this fix came from <midnight>
		* http://www.flipcode.com/archives/3DS_Camera_Orientation.shtml
		* an alternate up vector is used
		*/
		_ASSERT(0); // i'd like to know if i ever hit this
		up = vec4(-dir.y, 0, 0, 0);
	}
	else {
		up = vec4(0, 1, 0, 0);
	}

	// dir & right are already normalized
	// so up should be already.

	// UP cross DIR gives us "right"
	right = normalized(cross(up, dir));

	// DIR cross RIGHT gives us a proper UP
	up = cross(dir, right);

	mat4 mrot;
	mrot = mat4_init(
		 right.x, right.y, right.z, 0
		, up.x, up.y, up.z, 0
		, dir.x, dir.y, dir.z, 0
		, 0, 0, 0, 1);

	mat4 mpos = mat4_translate(vec3(-from.x, -from.y, -from.z));

	return mat4_mul(mrot, mpos);
}


mat4 mat4_look_from_to(const vec4& from, const vec4& to)
{
	vec4 up;
	vec4 right;

	// build ->vector from 'from' to 'to'
	vec4 dir = normalized(from - to);

	// build a temporary 'up' vector
	if (fabs(dir.x)<FLT_EPSILON && fabs(dir.z)<FLT_EPSILON) {
		/*
		* special case where dir & up would be degenerate
		* this fix came from <midnight>
		* http://www.flipcode.com/archives/3DS_Camera_Orientation.shtml
		* an alternate up vector is used
		*/
		_ASSERT(0); // i'd like to know if i ever hit this
		up = vec4(-dir.y, 0, 0, 0);
	}
	else {
		up = vec4(0, 1, 0, 0);
	}

	// dir & right are already normalized
	// so up should be already.

	// UP cross DIR gives us "right"
	right = normalized(cross(up, dir));
	//	vec3_cross( right, up, dir );  vec3_normalize(right);

	// DIR cross RIGHT gives us a proper UP
	up = cross(dir, right);
	//	vec3_cross( up, dir, right );//vec3_normalize(up);

	mat4 mrot = mat4_init(
		right.x, right.y, right.z, 0
		, up.x, up.y, up.z, 0
		, dir.x, dir.y, dir.z, 0
		, 0, 0, 0, 1);

	mat4 mpos = mat4_init(
		1, 0, 0, -from.x
		, 0, 1, 0, -from.y
		, 0, 0, 1, -from.z
		, 0, 0, 0, 1);

	return mat4_mul(mrot, mpos);
}
