#pragma once
#include "../data_structs/Octree.h"

#include <fstream>
#include "../Camera.h"

class Loader
{
public:
	static Octree* load_vox(const char* file_name, std::vector<Octree>& octrees);

	static Octree* load_oct(const char* file_name, std::vector<Octree>& octrees);

	static void load_scene(const char* file_name, Camera& camera, bool& render_light, bool& render_normal);

	static void dump_oct(const char* file_name, Octree& octree);

	static void dump_scene(const char* file_name, Camera& camera, bool& render_light, bool& render_normal);

private:
	static void load_chunk(std::fstream& file, Octree& octree);

public:
	static const uint32_t PALETTE[256];
};
