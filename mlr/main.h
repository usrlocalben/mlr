
#include "stdafx.h"

#include "PixelToaster.h"

using namespace PixelToaster;


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


