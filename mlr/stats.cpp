
#include "stdafx.h"
#include <iostream>
#include <vector>

#include <boost/format.hpp>


#include "stats.h"

using namespace PixelToaster;
using namespace std;

const int NICE_BLUE = 0x2266aa;
const int NICE_GREEN = 0x33cc77;
const int NICE_LAVENDER = 0x8877aa;
const int NICE_FOREST = 0x558822;
const int NICE_MAGENTA = 0x9911aa;


vector<int> telecolors = {
	NICE_BLUE, NICE_GREEN, NICE_LAVENDER, NICE_FOREST, NICE_MAGENTA,
	NICE_BLUE, NICE_GREEN, NICE_LAVENDER, NICE_FOREST, NICE_MAGENTA,
	NICE_BLUE, NICE_GREEN, NICE_LAVENDER, NICE_FOREST, NICE_MAGENTA,
	NICE_BLUE, NICE_GREEN, NICE_LAVENDER, NICE_FOREST, NICE_MAGENTA
};


void Telemetry::start()
{
	for (auto& item : data) {
		item.markers.clear();
		item.teletimer.reset();
	}
	this->x = 0;
}

void Telemetry::mark(const int thread)
{
	auto& item = data[thread];
	item.markers.push_back({ item.teletimer.delta() * 1000, this->x });
}

void Telemetry::inc()
{
	x++;
}

void Telemetry::end()
{
}

/*
void Telemetry::print() const
{
	cout << "--------------" << endl;
	for (auto& item : markers) {
		cout << "x:" << item.x << ", t:" << boost::format("%.4f") % item.tm << endl;
	}
}
*/

void Telemetry::draw(const unsigned stride, TrueColorPixel * const __restrict dst) const
{
	const int offset = 5; //pixels
	const int graph_width = stride - offset * 2;
	//	const float ms_width = 40.0f;
	const float factor = 20.0f;

	int ypos = 40;
	for (auto& t : data) {

		int ci = 0;
		int px = offset;
		for (auto& item : t.markers) {
//			if (ci == 0) {
//				ci++; continue;
//			}

			int width = static_cast<int>(item.tm * factor + 0.5);
			for (int i = 0; px < stride - offset && i < width; i++, px++) {
				for (int row = 0; row < 5; row++) {
					dst[px + ((row + ypos)*stride)].integer = telecolors[item.x];
				}
			}
			if (width)
				px += 1;

			ci++;
		}
		ypos += 6;

	}

}