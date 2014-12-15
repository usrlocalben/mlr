
#ifndef __PICOPNG_H
#define __PICOPNG_H

#include "stdafx.h"

#include <vector>
#include "aligned_allocator.h"

extern void loadFile(std::vector<unsigned char>& buffer, const std::string& filename);

extern int decodePNG(
	 vectorsse<unsigned char>& out_image
	,unsigned long& image_width
	,unsigned long& image_height
	,const unsigned char* in_png
	,size_t in_size
	,bool convert_to_rgba32=true
);


#endif //__PICOPNG_H
