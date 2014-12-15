
#ifndef __PROFONT_H
#define __PROFONT_H

#include "stdafx.h"

#include <map>

#include "PixelToaster.h"
#include "aligned_allocator.h"

#include "vec.h"

using namespace PixelToaster;



/*
SPACE,!,",#,$,%,&,',(,),*,+,COMMA,-,.,/,0,1,2,3,4,5,6,7,8,:,;,<,=,>,?
@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
`abcdefghijklmnopqrstuvwxyz{|}~MISSING

other interesting chars..

"degrees" y=4, x=1
"notequal" y=4, x=13
bullet y=4, x=5
plusminus, y=4, x=17
*/

class ProPrinter {

	const ivec2 glyph_dimensions;
	std::map<char, ivec2> charmap;
	vectorsse<unsigned char> fontcontainer;
	TrueColorPixel *font;

public:
	ProPrinter();
	unsigned int charmap_to_offset(const ivec2& cxy) const;
	unsigned int char_to_offset(const char ch) const;
	void draw(const char ch, TrueColorPixel *dst, const int stride) const;
	void draw(const std::string str, TrueColorPixel *dst, const int stride) const;

};

#endif //__PROFONT_H
