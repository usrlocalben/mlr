
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

struct Teledata {
	std::vector<Telemarker> markers;
	Timer teletimer;
};

class Telemetry {
public:
	Telemetry(const int threads) :threads(threads) {
		data.resize(threads);
	}

	void start();
	void mark(const int thread);
	void inc();
	void end();
	void print() const;
	void draw(const unsigned stride, TrueColorPixel * const __restrict dst) const;
private:
	const int threads;
	std::vector<Teledata> data;
	int x;
};

#endif //__STATS_H

