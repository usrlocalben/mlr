
#include "stdafx.h"

#include <functional>
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

int matlow, mathigh;

Demo::Demo(Rocket& rocket, TextureStore& texturestore, MeshStore& meshstore, MaterialStore& materialstore, Telemetry& telemetry) :
	config_width(0),
	config_height(0),
	rocket(rocket),
	meshstore(meshstore),
	texturestore(texturestore),
	materialstore(materialstore),
	telemetry(telemetry),
	pipeline(1)
{
	on_resize(0, 0);

	for (int i = 0; i < 64; i++) {
		colorpack.push_back(vec4(frandom(), frandom(), frandom(), 0));
	}

	matlow = materialstore.store.size();
	for (int i = 0; i < 3; i++) {
		Material m;
		m.imagename = "";
		float intensity = frandom() * 128;
		m.kd = vec3(frandom(), frandom(), frandom()) * vec3(intensity);
		materialstore.store.push_back(m);
	}
	mathigh = materialstore.store.size()-1;
}

Demo::~Demo()
{
}


void Demo::render(
	const int target_width,
	const int target_height,
	const int target_stride,
	TrueColorPixel * const __restrict target,
	const mat4& ui_camera,
	const double ui_time
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

	pipeline.reset(config_width, config_height);
	pipeline.setViewport(&vp);

//	auto cm = mat4_look_from_to(vec4(0, 0, 30, 1), vec4(0, 0, 0, 1));
//	auto cm = mat4::ident();
//	pipeline.addCamera(cm);
//	pipeline.addCamera(mat4_translate(vec3(0, 0, 100)));
	pipeline.addCamera(ui_camera);

	//auto themesh = meshstore.find_by_name("textest1.obj");
	auto themesh = meshstore.find("cube1x1.obj");

	auto T = ui_time; // gt.time();
//	auto T = double(123459589.34539);

	vector<unique_ptr<Meshy>> stack;
#define previous_op (*stack[stack.size()-1])

	stack.push_back(make_unique<MeshySet>      (&themesh,    mat4::scale(cubescale)));
	stack.push_back(make_unique<MeshyMultiply> (previous_op, 20, vec3(16, 0, 0), vec3(0, 0, 0)));
	stack.push_back(make_unique<MeshyMultiply> (previous_op, 20, vec3(0, 16, 0), vec3(0, 0, 0)));
	stack.push_back(make_unique<MeshyMultiply> (previous_op, 20, vec3(0, 0, 16), vec3(0, 0, 0)));
	stack.push_back(make_unique<MeshyCenter>   (previous_op, true, true, true, false));
	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4::position(vec3(fract(T*mov_x)*16, fract(T*mov_y)*16, fract(T*mov_z)*16))));
	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4::rotate_x(T*rot_y)));
	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4::rotate_y(T*rot_x)));
//	pipeline.addMeshy(previous_op);


	stack.push_back(make_unique<MeshySet>      (&themesh,    mat4::scale(cubescale)));
	stack.push_back(make_unique<MeshyScatter>  (previous_op, 2000, vec3(500, cram, narb))); // narb, cram, 200)));
	stack.push_back(make_unique<MeshyCenter>   (previous_op, false, true, true, false));
	stack.push_back(make_unique<MeshyMultiply> (previous_op, 3, vec3(500, 0, 0), vec3(0, 0, 0)));
	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4::rotate_x(T*rot_y)));
	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4::rotate_y(3.14/2)));
	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4::position(vec3(0,0,500+(fract(T*mov_x)-0)*500))));
	stack.push_back(make_unique<MeshyMaterial> (previous_op, matlow, mathigh));
	pipeline.addMeshy(previous_op);

	auto tiles = meshstore.find("tiles1.obj");
	stack.push_back(make_unique<MeshySet>      (&tiles,      mat4::rotate_x(3.14 / 2.0f)));
	stack.push_back(make_unique<MeshyCenter>   (previous_op, true, true, true, false));
	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4::position(vec3(0, 0, -200))));
//	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4_rotate_y(gt.time()*0.05f)));
//	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4_rotate_y(gt.time()*0.05f)));
//	pipeline.addMeshy(previous_op);

	auto flame = meshstore.find("flame1.obj");
	stack.push_back(make_unique<MeshySet>      (&flame,      mat4::rotate_x(3.14 / 2.0f)));
	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4::scale(vec3(1, 1, cram))));
	stack.push_back(make_unique<MeshyCenter>   (previous_op, true, true, true, true));
	stack.push_back(make_unique<MeshyTranslate>(previous_op, mat4::rotate_y(T*rot_x)));
//	pipeline.addMeshy(previous_op);

	telemetry.mark();
	function<void(bool)> _mark = bind(&Telemetry::mark2, &telemetry, placeholders::_1);

	pipeline.render(depthtarget, rendertarget, this->materialstore, this->texturestore, _mark, target, target_width);
	telemetry.inc();

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
