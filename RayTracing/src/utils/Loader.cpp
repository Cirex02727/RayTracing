#include "Loader.h"

#include <iostream>

void Loader::load_vox(const char* file_name, Octree& octree)
{
	std::fstream file(file_name, std::ios::binary | std::ios::in);
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

void Loader::load_oct(const char* file_name, Octree& octree)
{
	std::fstream file(file_name, std::ios::binary | std::ios::in);

	// Read Nodes Size
	int size;
	file.read((char*) &size, sizeof(int));

	// Read Nodes
	for (int i = 0; i < size; i++)
	{
		uint32_t first_child, flags;
		u_shortV3 bottom_corner, top_corner;
		
		file.read((char*) &first_child, sizeof(uint32_t));

		file.read((char*) &bottom_corner.x, sizeof(uint16_t) * 3);

		uint16_t voxel_size;
		file.read((char*) &voxel_size, sizeof(uint16_t));
		top_corner = u_shortV3(bottom_corner.x + voxel_size, bottom_corner.y + voxel_size, bottom_corner.z + voxel_size);

		file.read((char*) &flags, sizeof(uint32_t));

		octree.add_node(first_child, bottom_corner, top_corner, flags);
	}

	file.close();
}

void Loader::load_scene(const char* file_name, Camera& camera, bool& render_light, bool& render_normal)
{
	std::fstream file(file_name, std::ios::binary | std::ios::in);

	// Write Camera
	float vertical_fov;
	float near_clip;
	float far_clip;
	file.read((char*) &vertical_fov, sizeof(float));
	file.read((char*) &near_clip, sizeof(float));
	file.read((char*) &far_clip, sizeof(float));

	glm::vec3 position, direction;
	file.read((char*) &position, sizeof(glm::vec3));
	file.read((char*) &direction, sizeof(glm::vec3));

	camera.SetData(vertical_fov, near_clip, far_clip, position, direction);

	file.read((char*) &render_light, sizeof(bool));
	file.read((char*) &render_normal, sizeof(bool));

	file.close();
}

void Loader::dump_oct(const char* file_name, Octree& octree)
{
	std::fstream file(file_name, std::ios::binary | std::ios::out);

	// Write Nodes Size
	int size = octree.m_Nodes.size();
	file.write((char*) &size, sizeof(int));

	// Write Nodes
	for (OctreeNode& node : octree.m_Nodes)
	{
		file.write((char*) &node.first_child, sizeof(uint32_t));

		file.write((char*) &node.bottom_corner.x, sizeof(uint16_t) * 3);

		uint16_t voxel_size = node.top_corner.x - node.bottom_corner.x;
		file.write((char*) &voxel_size, sizeof(uint16_t));

		file.write((char*) &node.flags, sizeof(uint32_t));
	}

	file.close();
}

void Loader::dump_scene(const char* file_name, Camera& camera, bool& render_light, bool& render_normal)
{
	std::fstream file(file_name, std::ios::binary | std::ios::out);

	// Write Camera
	float vertical_fov = camera.GetVerticalFOV();
	float near_clip = camera.GetNearClip();
	float far_clip = camera.GetFarClip();
	file.write((char*) &vertical_fov, sizeof(float));
	file.write((char*) &near_clip, sizeof(float));
	file.write((char*) &far_clip, sizeof(float));

	glm::vec3 position = camera.GetPosition();
	glm::vec3 direction = camera.GetDirection();
	file.write((char*) &position, sizeof(glm::vec3));
	file.write((char*) &direction, sizeof(glm::vec3));

	file.write((char*) &render_light, sizeof(bool));
	file.write((char*) &render_normal, sizeof(bool));

	file.close();
}

void Loader::load_chunk(std::fstream& file, Octree& octree)
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
