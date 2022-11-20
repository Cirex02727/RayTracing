#include "Loader.h"

#include <iostream>

void Loader::load_vox(const char* file_name, Octree& octree)
{
	std::ifstream file(file_name, std::ios::binary | std::ios::in);
	if (!file.is_open())
	{
		std::cout << "ERROR Open File: " << file_name << std::endl;
		return;
	}

	char version_id[4];
	int version;

	file.read((char*) &version_id, sizeof(version_id));
	file.read((char*) &version, sizeof(version));

	load_chunk(file, octree);

	file.close();
}

void Loader::load_chunk(std::ifstream& file, Octree& octree)
{
	// Chunk Info
	char chunk_id[4];
	int chunk_content, children_chunks;

	file.read((char*) &chunk_id, sizeof(chunk_id));
	file.read((char*) &chunk_content, sizeof(chunk_content));
	file.read((char*) &children_chunks, sizeof(children_chunks));

	char buffer[4];

	// SIZE
	file.read((char*) &buffer, sizeof(buffer)); // SIZE

	file.read((char*) &buffer, sizeof(buffer)); // Unknown 8 bytes
	file.read((char*) &buffer, sizeof(buffer)); //

	int size_x, size_y, size_z;

	file.read((char*) &size_x, sizeof(size_x));
	file.read((char*) &size_y, sizeof(size_y));
	file.read((char*) &size_z, sizeof(size_z));

	octree.init(size_x);

	// XYZI
	file.read((char*) &buffer, sizeof(buffer)); // XYZI

	int num_voxels;
	unsigned char x, y, z, color_id;

	file.read((char*) &num_voxels, sizeof(num_voxels));

	file.read((char*) &buffer, sizeof(buffer)); // Unknown 8 bytes
	file.read((char*) &buffer, sizeof(buffer)); //

	num_voxels -= 4;

	for (int j = 0; j < num_voxels; j += 4)
	{
		file.read((char*) &x, sizeof(x));
		file.read((char*) &y, sizeof(y));
		file.read((char*) &z, sizeof(z));
		file.read((char*) &color_id, sizeof(color_id));

		octree.insert_node(x, y, z, color_id);
	}
}
