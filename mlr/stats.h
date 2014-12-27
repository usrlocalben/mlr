
#ifndef __STATS_H
#define __STATS_H

#include "stdafx.h"
#include <vector>

#include "PixelToaster.h"

using namespace PixelToaster;

struct Telemarker {
	double tm;
	int x;
};

class Telemetry {
public:
	void start();
	void mark();
	void mark2(bool);
	void inc() { x++; }
	void end();
	void print() const;
	void draw(const unsigned stride, TrueColorPixel * const __restrict dst) const;
private:
	std::vector<Telemarker> markers;
	int x;
};

#endif //__STATS_H

