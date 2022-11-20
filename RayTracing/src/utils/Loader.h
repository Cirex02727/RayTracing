#pragma once
#include "../octree/Octree.h"

#include <fstream>

class Loader
{
public:
	static void load_vox(const char* file_name, Octree& octree);

private:
	static void load_chunk(std::ifstream& file, Octree& octree);
};
