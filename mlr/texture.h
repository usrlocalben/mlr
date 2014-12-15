
#ifndef __TEXTURE_H
#define __TEXTURE_H

#include "stdafx.h"

#include <vector>
#include <string>

#include "PixelToaster.h"
#include "aligned_allocator.h"

using namespace PixelToaster;

struct Texture {
	vectorsse<FloatingPointPixel> b;
	int width;
	int height;
	int stride;
	std::string name;
	int pow;
	bool mipmap;

	void maybe_make_mipmap();
	void saveTga(const std::string& fn) const;
};

class TextureStore {
private:
	std::vector<Texture> store;
public:
	TextureStore();
//	const Texture& get(string const key);
	void append(Texture t);
	const Texture * const find(const std::string& needle) const;
	void loadDirectory(const std::string& prepend);
	void loadAny(const std::string& prepend, const std::string& fname);
	void print();
};

Texture loadPng(const std::string filename, const std::string name, const bool premultiply);


#endif //__TEXTURE_H
