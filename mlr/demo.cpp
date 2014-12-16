
#include "stdafx.h"

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>

#include <conio.h>

#include <Windows.h>

#include "main.h"
#include "tri.h"
#include "tri_sd.h"
#include "demo.h"
#include "canvas.h"
#include "render.h"
#include "rocket.h"
#include "viewport.h"
#include "texture.h"

using namespace std;
using boost::format;

ShadedShader my_shader;

SOACanvas rendertarget;
SOADepth depthtarget;

vectorsse<vec4> colorpack;


Timer gt;

inline static double random()
{
	// Custom random number generator...
	static unsigned seed1 = 0x23125253;
	static unsigned seed2 = 0x7423e823;
	seed1 = 36969 * (seed1 & 65535) + (seed1 >> 16);
	seed2 = 18000 * (seed2 & 65535) + (seed2 >> 16);
	return (double)((seed1 << 16) + seed2) / (double)0xffffffffU;
}

inline static float frandom() {
	return static_cast<float>(random());
}


Demo::Demo(Rocket& rocket, TextureStore& texturestore, MeshStore& meshstore, MaterialStore& materialstore, Telemetry& telemetry) :
	config_width(0),
	config_height(0),
	rocket(rocket),
	meshstore(meshstore),
	texturestore(texturestore),
	materialstore(materialstore),
	telemetry(telemetry),
	pipeline(16)
{
	on_resize(0, 0);

	for (int i = 0; i < 64; i++) {
		colorpack.push_back(vec4(frandom(), frandom(), frandom(), 0));
	}
}

Demo::~Demo()
{
}


void Demo::render(
	const int target_width,
	const int target_height,
	const int target_stride,
	TrueColorPixel * const __restrict target,
	const mat4& ui_camera
)
{
	if (target_width != config_width || target_height != config_height)
		on_resize(target_width, target_height);

	auto zorp = rocket.getf("zorp");
	auto cram = rocket.getf("cram");
	auto narb = rocket.getf("narb");
	auto rot_x = rocket.getf("rotx");
	auto rot_y = rocket.getf("roty");
	auto mov_x = rocket.getf("mov_x");
	auto mov_y = rocket.getf("mov_y");
	auto mov_z = rocket.getf("mov_z");
	auto cubescale_x = rocket.getf("cubescale_x");
	auto cubescale_y = rocket.getf("cubescale_y");
	auto cubescale_z = rocket.getf("cubescale_z");
	auto cubescale = vec3(cubescale_x, cubescale_y, cubescale_z);
	Viewport vp(config_width, config_height, float(config_width) / float(config_height), zorp);

	telemetry.mark();

	depthtarget.clear(wholescreen);
	rendertarget.clear(wholescreen);
	telemetry.mark();

	pipeline.reset(config_width, config_height);
	pipeline.setViewport(&vp);

//	auto cm = mat4_look_from_to(vec4(0, 0, 30, 1), vec4(0, 0, 0, 1));
//	auto cm = mat4::ident();
//	pipeline.addCamera(cm);
//	pipeline.addCamera(mat4_translate(vec3(0, 0, 100)));
	pipeline.addCamera(ui_camera);

	//auto themesh = meshstore.find_by_name("textest1.obj");
	auto themesh = meshstore.find("cube1x1.obj");

	auto T = gt.time();
//	auto T = double(123459589.34539);

	vector<unique_ptr<Meshy>> stack;
	int idx = 0;
	stack.push_back(unique_ptr<Meshy>(new MeshySet(&themesh, mat4::ident())));
	stack.push_back(unique_ptr<Meshy>(new MeshyTranslate(*stack[idx], mat4_scale(cubescale)))); idx++;
	stack.push_back(unique_ptr<Meshy>(new MeshyMultiply(*stack[idx], 20, vec3(16, 0, 0), vec3(0, 0, 0)))); idx++;
	stack.push_back(unique_ptr<Meshy>(new MeshyMultiply(*stack[idx], 20, vec3(0, 16, 0), vec3(0, 0, 0)))); idx++;
	stack.push_back(unique_ptr<Meshy>(new MeshyMultiply(*stack[idx], 20, vec3(0, 0, 16), vec3(0, 0, 0)))); idx++;
	stack.push_back(unique_ptr<Meshy>(new MeshyCenter(*stack[idx], true, true, true, false))); idx++;
//	stack.push_back(unique_ptr<Meshy>(new MeshyTranslate(*stack[idx], mat4_translate(vec3(0, 15, 0))))); idx++;
	stack.push_back(unique_ptr<Meshy>(new MeshyTranslate(*stack[idx], mat4_translate(vec3(fract(T*mov_x)*16, fract(T*mov_y)*16, fract(T*mov_z)*16))))); idx++;
	stack.push_back(unique_ptr<Meshy>(new MeshyTranslate(*stack[idx], mat4_rotate_y(T*rot_y)))); idx++;
	stack.push_back(unique_ptr<Meshy>(new MeshyTranslate(*stack[idx], mat4_rotate_x(T*rot_x)))); idx++;
	pipeline.addMeshy(*stack[idx]);


	auto tiles = meshstore.find("tiles1.obj");
	stack.push_back(unique_ptr<Meshy>(new MeshySet(&tiles, mat4::ident()))); idx++;
	stack.push_back(unique_ptr<Meshy>(new MeshyCenter(*stack[idx], true, false, true, false))); idx++;
	stack.push_back(unique_ptr<Meshy>(new MeshyTranslate(*stack[idx], mat4_rotate_y(gt.time()*0.05f)))); idx++;
//	pipeline.addMeshy(*stack[idx]);

	telemetry.mark();
	function<void()> _mark = bind(&Telemetry::mark2, &telemetry);


	pipeline.render(depthtarget.rawptr(), rendertarget.rawptr(), this->materialstore, this->texturestore, _mark);
	telemetry.inc();

	convertCanvas(wholescreen, target_width, target, rendertarget.rawptr(), PostprocessNoop());
	telemetry.mark();
}


void Demo::on_resize(const int new_x, const int new_y)
{
	config_width = new_x;
	config_height = new_y;
	rendertarget.setup(config_width, config_height);
	depthtarget.setup(config_width, config_height);
	wholescreen.x0 = 0;  //64; // 300;
	wholescreen.y0 = 0;  // 64; //90;
	wholescreen.x1 = config_width;
	wholescreen.y1 = config_height;
}
