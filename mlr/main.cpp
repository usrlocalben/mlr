
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
#include "stats.h"
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


class KeyboardCamera {
public:
	KeyboardCamera() :camrot({ 0, 0, 0 }), campos({ 0, 0, 0, 1 }), camdir_ahead({ 0, 0, -1, 0 }), camdir_right({ 1, 0, 0, 0 }){}

	void on_w() { campos += camdir_ahead; }
	void on_s() { campos -= camdir_ahead; }
	void on_a() { campos -= camdir_right; }
	void on_d() { campos += camdir_right; }
	void on_q() { campos -= vec4(0, 1, 0, 0); }
	void on_e() { campos += vec4(0, 1, 0, 0); }

	void rot_x(const float a) { camrot += vec3(a, 0, 0); }
	void rot_y(const float a) { camrot += vec3(0, a, 0); }
	void rot_z(const float a) { camrot += vec3(0, 0, a); }

	void update() {
		mat4 camera_rot = mat4_mul(mat4::rotate_x(-camrot.x), mat4_mul(mat4::rotate_y(camrot.y), mat4::rotate_z(camrot.z)));
		camdir_ahead = mat4_mul(camera_rot, vec4(0, 0, -1, 0));
		camdir_right = mat4_mul(camera_rot, vec4(1, 0, 0, 0));
		camera_matrix = mat4_mul(mat4::position(campos), camera_rot);
	}

	mat4 camera_matrix;

private:
	vec3 camrot;
	vec4 campos;
	vec4 camdir_ahead;
	vec4 camdir_right;
};

class WallClock {
public:
	WallClock() :paused(false) {}
	double read() {
		if (paused) {
			return pause_time;
		} else {
			return t.time();
		}
	}
	void set_pause(bool should_pause) {
		if (should_pause) {
			pause_time = t.time();
			paused = true;
		} else {
			paused = false;
		}
	}
	void set_pause_time(double t) {
		pause_time = t;
	}
	void toggle() {
		set_pause(!paused);
	}
private:
	Timer t;
	bool paused;
	double pause_time;
};

KeyboardCamera mastercamera;
WallClock masterclock;




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

	Telemetry telemetry(get_cpu_count());

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

		if (jsonfile.root().get("start_paused").toBool()) {
			masterclock.set_pause(true);
			masterclock.set_pause_time(1000);
		}
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
			telemetry.mark(0);
SAMPLE(subtimer,ax_framestart);

			TrueColorPixel *ob = frame.getBuffer();
			if (!ob) break;
			//ob += ((480 - 360) >> 1) * 640;

			mastercamera.update();
			demo.render(render_width, render_height, render_width, ob, mastercamera.camera_matrix, masterclock.read());

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

/*	     if (key == Key::W) { campos += vec3(0, 0, -1); }
	else if (key == Key::S) { campos -= vec3(0, 0, -1); }
	else if (key == Key::A) { campos -= vec3(1, 0, 0); }
	else if (key == Key::D) { campos += vec3(1, 0, 0); }
	else if (key == Key::E) { campos += vec3(0, 1, 0); }
	else if (key == Key::Q) { campos -= vec3(0, 1, 0); }
	*/
void Application::onKeyPressed(DisplayInterface& display, Key key) {
	     if (key == Key::W) { mastercamera.on_w(); }
    else if (key == Key::S) { mastercamera.on_s(); }
    else if (key == Key::A) { mastercamera.on_a(); }
    else if (key == Key::D) { mastercamera.on_d(); }
    else if (key == Key::Q) { mastercamera.on_q(); }
    else if (key == Key::E) { mastercamera.on_e(); }
	else if (key == Key::U) { mastercamera.rot_x( 0.1); }
	else if (key == Key::J) { mastercamera.rot_x(-0.1); }
	else if (key == Key::H) { mastercamera.rot_y( 0.1); }
	else if (key == Key::K) { mastercamera.rot_y(-0.1); }
	else if (key == Key::Y) { mastercamera.rot_z( 0.1); }
	else if (key == Key::I) { mastercamera.rot_z(-0.1); }
	else if (key == Key::P) { masterclock.toggle(); }
}

void Application::onKeyDown(DisplayInterface& display, Key key) {
	if (key == Key::Escape) {
		quit = true;
	}
}

bool Application::onClose(DisplayInterface& display) {
	return quit = true;
}

