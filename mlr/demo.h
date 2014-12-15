
#ifndef __DEMO_H
#define __DEMO_H

#include "stdafx.h"

#include "PixelToaster.h"

class Demo
{
public:
	Demo(
		class Rocket& rocket,
		class TextureStore& texturestore,
		class MeshStore& meshstore,
		class MaterialStore& materialstore,
		class Telemetry& telemetry
	);
	~Demo();
	void render(
		const int target_width,
		const int target_height,
		const int target_stride,
		TrueColorPixel * const __restrict target,
		const mat4& ui_camera
	);

private:
	void on_resize(const int new_width, const int new_height);

	int config_width, config_height;
	irect wholescreen;

	class Rocket& rocket;

	class MeshStore& meshstore;
	class TextureStore& texturestore;
	class MaterialStore& materialstore;

	class Telemetry& telemetry;

};

#endif //__DEMO_H
