
#ifndef __CLIP_H
#define __CLIP_H

#include "stdafx.h"

#include "vec.h"

const unsigned CLIP_LEFT   = 1 << 0;
const unsigned CLIP_RIGHT  = 1 << 1;
const unsigned CLIP_BOTTOM = 1 << 2;
const unsigned CLIP_TOP    = 1 << 3;
const unsigned CLIP_NEAR   = 1 << 4;
const unsigned CLIP_FAR    = 1 << 5;

const float GUARDBAND_FACTOR = 2.0f;
const vec4 GUARDBAND_V(GUARDBAND_FACTOR);


std::array<unsigned,5> CLIP_LIST = { CLIP_LEFT, CLIP_RIGHT, CLIP_BOTTOM, CLIP_TOP, CLIP_NEAR };


class Guardband {

public:

static __forceinline unsigned clipPoint(const vec4& p) {
	const float wscale = p.w * GUARDBAND_FACTOR;

	unsigned cf = 0;
	if (wscale+p.x < 0) cf |= CLIP_LEFT;
	if (wscale-p.x < 0) cf |= CLIP_RIGHT;
	if (wscale+p.y < 0) cf |= CLIP_BOTTOM;
	if (wscale-p.y < 0) cf |= CLIP_TOP;
	if (   p.w+p.z < 0) cf |= CLIP_NEAR;
//	if (   p.w-p.z < 0) cf |= CLIP_FAR;
	return cf;
}


	/*
static __forceinline unsigned clipPoint(const vec4& p) {
	//const float wscale = p.w * GUARDBAND_FACTOR;
	auto wscale = p.wwww() * GUARDBAND_V;

	auto gminus = wscale - p;
	auto gplus = wscale + p;
	auto plus = p.wwww() + p;

	return (gplus.x < 0) << 0 |
		(gminus.x < 0) << 1 |
		(gplus.y < 0) << 2 |
		(gminus.y < 0) << 3 |
	       ( plus.z < 0)<<4;
}
*/

static __forceinline bool is_inside(const unsigned clipplane, const vec4& p) {
	const float wscaled = p.w * GUARDBAND_FACTOR;
	switch ( clipplane ) {
		case CLIP_LEFT   : return wscaled + p.x >= 0;
		case CLIP_RIGHT  : return wscaled - p.x >= 0;
		case CLIP_BOTTOM : return wscaled + p.y >= 0;
		case CLIP_TOP    : return wscaled - p.y >= 0;
		case CLIP_NEAR   : return p.w+p.z >= 0;
		case CLIP_FAR    : return p.w-p.z >= 0;
	}
	_ASSERT(0);
	return 0;// silence warning
}


static __forceinline float clipLine(const unsigned clipplane, const vec4& a, const vec4& b) {
	const float aws = a.w * GUARDBAND_FACTOR;
	const float bws = b.w * GUARDBAND_FACTOR;
	switch ( clipplane ) {
		case CLIP_LEFT   : return (aws+a.x) / ((aws+a.x)-(bws+b.x));
		case CLIP_RIGHT  : return (aws-a.x) / ((aws-a.x)-(bws-b.x));
		case CLIP_BOTTOM : return (aws+a.y) / ((aws+a.y)-(bws+b.y));
		case CLIP_TOP    : return (aws-a.y) / ((aws-a.y)-(bws-b.y));
		case CLIP_NEAR   : return (a.w+a.z) / ((a.w+a.z)-(b.w+b.z));
		case CLIP_FAR    : return (a.w-a.z) / ((a.w-a.z)-(b.w-b.z));
	}
	_ASSERT(0);
	return 0;// silence warning
}

};


#endif //__CLIP_H

