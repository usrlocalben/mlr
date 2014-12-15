
#include "stdafx.h"
#include <algorithm>
#include <boost/format.hpp>
#include <fstream>

#include <Windows.h>

#include "ryg_srgb.h"

#include "vec.h"
#include "dib24.h"
#include "utils.h"
#include "picopng.h"
#include "texture.h"

using namespace std;
using boost::format;


Texture loadPng(const string filename, const string name, const bool premultiply) {

	vector<unsigned char> data;
	vectorsse<unsigned char> image;

	loadFile(data, filename);

	unsigned long w, h;
	int error = decodePNG(image, w, h, data.empty() ? 0 : &data[0], (unsigned long)data.size());
	if (error != 0) {
		cout << "error(" << error << ") decoding png from [" << filename << "]" << endl;
		while (1);
	}

	vectorsse<FloatingPointPixel> pc;

	pc.resize(w*h);

	auto * ic = &image[0];
	for (unsigned i = 0; i < w*h; i++) {
		auto& dst = pc[i];
		dst.r = srgb8_to_float(*(ic++)); // *(ic++) / 255.0f;
		dst.g = srgb8_to_float(*(ic++)); // *(ic++) / 255.0f;
		dst.b = srgb8_to_float(*(ic++)); // *(ic++) / 255.0f;
		dst.a = *(ic++) / 255.0f; // is alpha srgb?
		if (premultiply) {
			dst.r *= dst.a;
			dst.g *= dst.a;
			dst.b *= dst.a;
		}
	}

	return{ pc, w, h, w, name };
}

Texture loadJpg(const string filename, const string name) {
	
	DIB24 dib;

	dib.loadPicture(s2ws(filename).c_str());
	
	vectorsse<FloatingPointPixel> pc;
	pc.resize(dib.width*dib.height);

	auto * dst = &pc[0];
	for (int row = 0; row < dib.height; row++) {
		unsigned char *ic = dib[row];
		for (int x = 0; x < dib.width; x++) {
			dst->b = srgb8_to_float(*(ic++));
			dst->g = srgb8_to_float(*(ic++));
			dst->r = srgb8_to_float(*(ic++));
			dst->a = 1.0f;
			dst++;
		}
	}
	return{ pc, dib.width, dib.height, dib.width, name };
}

Texture checkerboard2x2() {
	vectorsse<FloatingPointPixel> db;
	db.push_back(FloatingPointPixel(0, 0, 0, 0));
	db.push_back(FloatingPointPixel(1, 0, 1, 0));
	db.push_back(FloatingPointPixel(1, 0, 1, 0));
	db.push_back(FloatingPointPixel(0, 0, 0, 0));
	return{ db, 2, 2, 2, "checkerboard" };
}


Texture loadAny(const string& prefix, const string& fn, const string& name, const bool premultiply) {
//	string tmp = fn;
//	transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
	cout << "texturestore: loading " << prefix << fn << endl;
	string ext = fn.substr(fn.length() - 4, 4);
	if (ext == ".png") {
		return loadPng(prefix+fn, name, premultiply);
	} else if (ext == ".jpg") {
		return loadJpg(prefix + fn, name);
	} else {
		cout << "unsupported texture extension \"" << ext << "\"" << endl;
		while (1);
	}
}

bool isPowerOfTwo(unsigned x) {
	while (((x & 1) == 0) && x > 1) {
		x >>= 1;
	}
	return x == 1;
}

int ilog2(unsigned x) {
	int pow = 0;
	while (x) {
		x >>= 1;
		pow++;
	}
	return pow - 1;
}

void Texture::maybe_make_mipmap()
{
	if (width != height) {
		pow = -1;
		mipmap = false;
		return;
	}

	if (!isPowerOfTwo(width)) {
		pow = -1;
		mipmap = false;
		return;
	}

	pow = ilog2(width);
	if (pow > 9) {
		pow = -1;
		mipmap = false;
		return;
	}

	b.resize(width * height * 2);

	int src_size = width;          // inital w/h of source
	int src = 0;                   // begin reading from the root image start
	int dst = src_size * src_size; // start output at the end of the root image
	for (int mip_level = pow-1; mip_level >= 0; mip_level--) {
//		cout << "making " << (sw >> 1) << endl;
		for (int src_y = 0; src_y < src_size; src_y += 2) {
			int dstrow = dst;
			for (int src_x = 0; src_x < src_size; src_x += 2) {

				const auto row1ofs = src + ( src_y     *width) + src_x;
				const auto row2ofs = src + ((src_y + 1)*width) + src_x;

				const auto sum2x2 = vec4(b[row1ofs].v) + vec4(b[row1ofs+1].v) +
				                    vec4(b[row2ofs].v) + vec4(b[row2ofs+1].v);

				const auto avg2x2 = sum2x2 / vec4(4.0f);

				b[dstrow++].v = avg2x2.v;
			}
			dst += stride;
		}
		src += src_size * stride; // advance read pos to mip-level we just drew
		src_size >>= 1;

	}
	this->mipmap = true;
	this->height *= 2;
}


void Texture::saveTga(const string& fn) const
{
	unsigned char hdr[18];
	memset(hdr, 0, 18);
	hdr[2] = 2; // true color
	hdr[12] = width & 255;
	hdr[13] = width >> 8;
	hdr[14] = height & 255;
	hdr[15] = height >> 8;
	hdr[16] = 32; // 32 bpp

	auto fd = ofstream(fn, ios_base::out | ios_base::binary);
	fd.write((const char*)hdr, 18);
	for (int row = height-1; row >=0; row--){
		for (int col = 0; col < width; col++){
			auto fpp = b[row*width + col];
			unsigned char ucb[4];
			ucb[0] = float_to_srgb8(fpp.b);
			ucb[1] = float_to_srgb8(fpp.g);
			ucb[2] = float_to_srgb8(fpp.r);
			ucb[3] = 255;
			fd.write((const char*)&ucb, 4);
		}
	}
}

TextureStore::TextureStore()
{
	this->append(checkerboard2x2());
}

void TextureStore::append(Texture t) {
	const int idx = store.size();
	store.push_back(t);
	//return idx;
}

const Texture * const TextureStore::find(const string& needle) const {
	for (auto& item : store) {
		if (item.name == needle) return &item;
	}
	return nullptr;
}


void TextureStore::loadAny(const string& prepend, const string& fname) {
	auto search = this->find(fname);
	if (search != nullptr) return;
	
	Texture newtex = ::loadAny(prepend, fname, fname, true);
	newtex.maybe_make_mipmap();
	this->append(newtex);
}


void TextureStore::loadDirectory(const string& prepend) {

	vector<string> extlst{ "*.png", "*.jpg" };

	for (auto& ext : extlst) {
		for (auto& fn : fileglob(prepend + ext)) {
			//cout << "scanning [" << prepend << "][" << fn << "]" << endl;
			this->loadAny(prepend, fn);
		}
	}
}


void TextureStore::print() {
	int i = 0;
	for (auto& item : store) {
		cout << "#" << format("% 3d") % i << " \"" << format("%-20s") % item.name << "\"  " << format("% 4d x% 4d") % item.width % item.height;
		cout << "  data@ 0x" << boost::format("%08x") % &(item.b[0]) << endl;
		i++;
	}
}
