#pragma once
#include "../octree/Octree.h"

#include <fstream>
#include "../Camera.h"

class Loader
{
public:
	static void load_vox(const char* file_name, Octree& octree);

	static void load_oct(const char* file_name, Octree& octree);

	static void load_scene(const char* file_name, Camera& camera, bool& render_light, bool& render_normal);

	static void dump_oct(const char* file_name, Octree& octree);

	static void dump_scene(const char* file_name, Camera& camera, bool& render_light, bool& render_normal);

private:
	static void load_chunk(std::fstream& file, Octree& octree);
};
