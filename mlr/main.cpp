
#include "stdafx.h"

#include <sstream>
#include <vector>
#include <thread>
#include <map>

#include <conio.h>

#include <bass.h>

#include "aligned_allocator.h"
#include "PixelToaster.h"
#include "picopng.h"

#include "vec.h"
#include "obj.h"
#include "main.h"
#include "mesh.h"
#include "demo.h"
#include "utils.h"
#include "player.h"
#include "rocket.h"
#include "profont.h"
#include "jsonfile.h"
#include "viewport.h"
#include "texture.h"





static struct sync_cb player_cb = {
	Player::callback_pause,
	Player::callback_setRow,
	Player::callback_isPlaying
};


using namespace std;
using namespace PixelToaster;
using boost::format;


extern int hackmode_enable;





class OutputFrame {

	Framebuffer * const ptfb;
	PixelToaster::Display& display;
public:
	OutputFrame(PixelToaster::Display& display)
		:display(display)
		, ptfb(display.update_begin())
	{
		if (ptfb) {
			if (ptfb->format != Format::XRGB8888) {
				cout << "pixeltoaster framebuffer is not truecolor" << endl;
				while (1);
			}
			/*
			if (ptfb->stride != width * sizeof(TrueColorPixel)) {
			cout << "pixeltoaster framebuffer stride mismatch" << endl;
			while (1);
			}
			*/
		}
	}

	~OutputFrame() {
		display.update_end();
	}

	TrueColorPixel * const getBuffer() {
		if (ptfb) {
			return reinterpret_cast<TrueColorPixel*>(ptfb->pixels);
		} else {
			return nullptr;
		}
	}

};



vec3 camrot(0, 0, 0);
vec3 campos(0, 10, 10);




Application::Application()
	:render_width(640),
	 render_height(360),
	 device_width(640),
	 device_height(360),
	 fullscreen(false)
{}




int Application::run(const bool started_from_gui) {

	MeshStore meshstore;
	MaterialStore materialstore;
	TextureStore texturestore;

	Telemetry telemetry;

	texturestore.loadDirectory("data\\textures\\");
	meshstore.loadDirectory("data\\meshes\\", materialstore, texturestore);



	Rocket rocket("data\\sync");
	ProPrinter pp;


	JsonFile jsonfile("data\\demo.json");

	Player player(
		jsonfile.root().get("soundtrack").get("filename").toString(),
		jsonfile.root().get("soundtrack").get("bpm").toNumber(),
		jsonfile.root().get("soundtrack").get("rows_per_beat").toInt()
		);

	if (!started_from_gui) {
		device_width = jsonfile.root().get("device_size").get("x").toInt();
		device_height = jsonfile.root().get("device_size").get("y").toInt();
		render_width = jsonfile.root().get("render_size").get("x").toInt();
		render_height = jsonfile.root().get("render_size").get("y").toInt();
		fullscreen = jsonfile.root().get("start_in_fullscreen").toBool();
	}

	if (hackmode_enable) device_height = 360;
	display.open(
		"pixeltoaster ^_^",
		device_width, device_height,
		fullscreen ? Output::Fullscreen : Output::Windowed,
		Mode::TrueColor
	);
	display.listener(this);


	int framecounter = 0;
	player.play();



#define SAMPLE(src,a) a = (a*0.9)+(src.delta()*1000*0.1)
	Timer frametimer;	double ax_frame = 0;
	Timer subtimer;		double ax_prep = 0, ax_framestart = 0, ax_render = 0, ax_end = 0;

	Demo demo(rocket, texturestore, meshstore, materialstore, telemetry);

	while (display.open()) {

		telemetry.start();

		frametimer.reset();
		subtimer.reset();

		framecounter++;
		double row = player.getRow();
		rocket.update( row, &player_cb, (void*)&player);
SAMPLE(subtimer,ax_prep);

		{
			OutputFrame frame(display);
			telemetry.mark();
SAMPLE(subtimer,ax_framestart);

			TrueColorPixel *ob = frame.getBuffer();
			if (!ob) break;
			//ob += ((480 - 360) >> 1) * 640;

			mat4 camera;
			camera = mat4_mul(mat4::position(campos), mat4::rotate_x(camrot.x));
			demo.render(render_width, render_height, render_width, ob, camera);

			{
				stringstream ss;
				double fps = 1.0 / ax_frame * 1000;
				ss << boost::format("% 4.4f") % ax_render << " ms, fps: " << boost::format("%.0f") % fps;
				pp.draw(ss.str(), ob + (16*render_width)+16, render_width);
			}

		telemetry.end();
//		telemetry.print();
		telemetry.draw(render_width, ob);
			
			
SAMPLE(subtimer,ax_render);
		}
SAMPLE(subtimer,ax_end);

		player.tick();
SAMPLE(frametimer, ax_frame);

	}
	return 0;
}

bool Application::defaultKeyHandlers() {
	return false;
}

void Application::onKeyPressed(DisplayInterface& display, Key key) {
	     if (key == Key::W) { campos += vec3(0, 0, -1); }
	else if (key == Key::S) { campos -= vec3(0, 0, -1); }
	else if (key == Key::A) { campos -= vec3(1, 0, 0); }
	else if (key == Key::D) { campos += vec3(1, 0, 0); }
	else if (key == Key::E) { campos += vec3(0, 1, 0); }
	else if (key == Key::Q) { campos -= vec3(0, 1, 0); }
	else if (key == Key::U) { camrot += vec3(0.1, 0, 0); }
	else if (key == Key::J) { camrot -= vec3(0.1, 0, 0); }
	else if (key == Key::H) { camrot += vec3(0, 0.1, 0); }
	else if (key == Key::K) { camrot -= vec3(0, 0.1, 0); }
	else if (key == Key::Y) { camrot += vec3(0, 0, 0.1); }
	else if (key == Key::I) { camrot -= vec3(0, 0, 0.1); }
}

void Application::onKeyDown(DisplayInterface& display, Key key) {
	if (key == Key::Escape) {
		quit = true;
	}
}

bool Application::onClose(DisplayInterface& display) {
	return quit = true;
}

const int NICE_BLUE = 0x2266aa;
const int NICE_GREEN = 0x33cc77;
const int NICE_LAVENDER = 0x8877aa;
const int NICE_FOREST = 0x558822;
const int NICE_MAGENTA = 0x9911aa;

Timer teletimer;
vector<int> telecolors = {
	NICE_BLUE, NICE_GREEN, NICE_LAVENDER, NICE_FOREST, NICE_MAGENTA, 
	NICE_BLUE, NICE_GREEN, NICE_LAVENDER, NICE_FOREST, NICE_MAGENTA,
	NICE_BLUE, NICE_GREEN, NICE_LAVENDER, NICE_FOREST, NICE_MAGENTA,
	NICE_BLUE, NICE_GREEN, NICE_LAVENDER, NICE_FOREST, NICE_MAGENTA
};


void Telemetry::start()
{
	this->markers.clear();
	this->x = 0;
	teletimer.reset();
}

void Telemetry::mark()
{
	this->markers.push_back({ teletimer.delta()*1000, this->x++ });
}

void Telemetry::mark2(bool advance)
{
	this->markers.push_back({ teletimer.delta()*1000, this->x });
	if (advance) this->x++;
}

void Telemetry::end()
{
}

void Telemetry::print() const
{
	cout << "--------------" << endl;
	for (auto& item : markers) {
		cout << "x:" << item.x << ", t:" << boost::format("%.4f") % item.tm << endl;
	}
}

void Telemetry::draw(const unsigned stride, TrueColorPixel * const __restrict dst) const
{
	const int offset = 5; //pixels
	const int graph_width = stride - offset * 2;
//	const float ms_width = 40.0f;
	const float factor = 20.0f;

	int ci = 0;
	int px = offset;
	for (auto& item : markers) {
		if (ci == 0) {
			ci++; continue;
		}
		int width = static_cast<int>(item.tm * factor + 0.5);
		for (int i = 0; px < stride - offset && i < width; i++, px++) {
			for (int row = 0; row < 5; row++) {
				dst[px + ((row+offset)*stride)].integer = telecolors[item.x];
			}
		}
		if (width)
			px += 1;
		ci++;
//		cout << "x:" << item.x << ", t:" << boost::format("%.4f") % item.tm << endl;
	}

}