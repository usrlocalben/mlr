
#include "stdafx.h"

#include "profont.h"
#include "picopng.h"

using namespace std;
using namespace PixelToaster;

vectorsse<unsigned char> loadpng(string fn) {

	vector<unsigned char> buffer;
	vectorsse<unsigned char> image;
	//vector_uchar_sse image;

	loadFile(buffer, fn);
	unsigned long w, h;
	int error = decodePNG(image, w, h, buffer.empty() ? 0 : &buffer[0], (unsigned)buffer.size());

	if (error != 0) {
		cout << "png error: " << error << endl;
		while (1);
	}

	if (image.size() > 4) {
		//		cout << "width: " << w << ", height: " << h << endl;
		//		cout << "first pixel : " << hex << int(image[0]) << int(image[1]) << int(image[2]) << int(image[3]) << endl;
	}

	return image;
}


ProPrinter::ProPrinter()
	:fontcontainer(loadpng("data\\profont_6x11.png")),
	 glyph_dimensions(ivec2(6, 11))
{
	font = reinterpret_cast<TrueColorPixel*>(&fontcontainer[0]);

	string row1(R"( !"#$%&'()*+,-./0123456789:;<=>?)");
	string row2(R"(@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_)");
	string row3(R"(`abcdefghijklmnopqrstuvwxyz{|}~ )");

	{ int xpos = 0; for (char& ch : row1) charmap[ch] = ivec2(xpos++, 0); }
		{ int xpos = 0; for (char& ch : row2) charmap[ch] = ivec2(xpos++, 1); }
		{ int xpos = 0; for (char& ch : row3) charmap[ch] = ivec2(xpos++, 2); }
}

unsigned ProPrinter::charmap_to_offset(const ivec2& cxy) const {
	ivec2 gxy = cxy * glyph_dimensions;
	const int tex_width = 192;
	const int tex_height = 77;
	return gxy.y * tex_width + gxy.x;
}

unsigned ProPrinter::char_to_offset(const char ch) const {
	auto cxy = charmap.find(ch);
	if (cxy == charmap.end()) {
		return 0;
	} else {
		return charmap_to_offset(cxy->second);
	}
}

void ProPrinter::draw(const char ch, TrueColorPixel *dst, const int stride) const {
	TrueColorPixel *src = &font[char_to_offset(ch)];
	for (int y = 0; y < glyph_dimensions.y; y++) {
		for (int x = 0; x < glyph_dimensions.x; x++) {
			if (src[x].r == 0xff) dst[x].integer = 0;
			else dst[x].integer = 0xffffffff;
			//dst[x] = src[x];
		}
		dst += stride;
		src += 192; // glyph_dimensions.x;
	}
}

void ProPrinter::draw(const string str, TrueColorPixel *dst, const int stride) const {
	for (const char& ch : str) {
		draw(ch, dst, stride);
		dst += glyph_dimensions.x;
	}
}
