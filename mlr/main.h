
#include "stdafx.h"

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
	void mark2();
	void inc() { x++; }
	void end();
	void print() const;
	void draw(const unsigned stride, TrueColorPixel * const __restrict dst) const;
private:
	std::vector<Telemarker> markers;
	int x;
};

class Application : public PixelToaster::Listener {
public:
	int render_width;
	int render_height;
	int device_width;
	int device_height;
	bool fullscreen;

	Application();
	int run(const bool started_from_gui);
	bool defaultKeyHandlers();
	void onKeyPressed(DisplayInterface& display, Key key);
	void onKeyDown(DisplayInterface& display, Key key);
	bool onClose(DisplayInterface& display);

private:
	Display display;
	bool quit;
};


