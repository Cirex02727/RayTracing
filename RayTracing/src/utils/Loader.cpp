#include "Loader.h"

#include "Walnut/Timer.h"

#include "../Core.h"

#include <iostream>

const uint32_t Loader::PALETTE[256] = {
	0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
	0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
	0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
	0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
	0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
	0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
	0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
	0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
	0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
	0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
	0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
	0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
	0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
	0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
	0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
	0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
};

Octree* Loader::load_vox(const char* file_name, std::vector<Octree>& octrees)
{
	std::fstream file(file_name, std::ios::binary | std::ios::in);
	if (!file.is_open())
	{
		std::cout << "ERROR Open File: " << file_name << std::endl;
		return nullptr;
	}

#ifdef RAY_DEBUG
	Walnut::Timer t;
#endif // RAY_DEBUG

	char version_id[4];
	int version;

	file.read((char*) &version_id, sizeof(version_id));
	file.read((char*) &version, sizeof(version));

	Octree& octree = octrees.emplace_back();

	load_chunk(file, octree);

	file.close();

	float load_vox = t.ElapsedMillis();
	LOG("Load Vox '" << file_name << "': " << load_vox << "ms");

	octree.collapse_nodes();
	octree.build_lods();
	octree.calculate_max_depth();

	LOG("Post-Load Vox '" << file_name << "': " << (t.ElapsedMillis() - load_vox) << "ms");

	
	std::string str(file_name);

	size_t slash_idx = str.rfind('\\', str.length());
	if (slash_idx == std::string::npos)
		slash_idx = str.rfind('/', str.length());

	size_t dot_idx = str.rfind('.', str.length());
	std::string res = "./models/octrees/" + str.substr(slash_idx + 1, dot_idx - slash_idx - 1) + ".oct";

	Loader::dump_oct(res.c_str(), octree);

	LOG("Fill Octree(" << (octrees.size() - 1) << "): " << t.ElapsedMillis() << " ms " << ((octree.m_Nodes.size() * sizeof(OctreeNode)) / 1024.0f / 1024) << " MB");

	return &octree;
}

Octree* Loader::load_oct(const char* file_name, std::vector<Octree>& octrees)
{
	// Check if .oct exist
	struct stat buffer;
	if (stat(file_name, &buffer) != 0)
	{
		std::cout << "File not found: " << file_name << std::endl;
		return nullptr;
	}


#ifdef RAY_DEBUG
	Walnut::Timer t;
#endif // RAY_DEBUG

	std::fstream file(file_name, std::ios::binary | std::ios::in);

	// Read Nodes Size
	int size;
	file.read((char*) &size, sizeof(int));

	Octree& octree = octrees.emplace_back();

	// Read Nodes
	for (int i = 0; i < size; i++)
	{
		uint32_t first_child, flags;
		u_shortV3 bottom_corner, top_corner;
		
		file.read((char*) &first_child, sizeof(uint32_t));

		file.read((char*) &bottom_corner.x, sizeof(uint16_t) * 3);

		file.read((char*) &top_corner.x, sizeof(uint16_t) * 3);

		file.read((char*) &flags, sizeof(uint32_t));

		octree.add_node(first_child, bottom_corner, top_corner, flags);
	}

	octree.calculate_max_depth();

	file.close();

	LOG("Load Oct '" << file_name << "': " << t.ElapsedMillis() << " ms " << ((octree.m_Nodes.size() * sizeof(OctreeNode)) / 1024.0f / 1024) << " MB");

	return &octree;
}

void Loader::load_scene(const char* file_name, Camera& camera, bool& render_light, bool& render_normal)
{
#ifdef RAY_DEBUG
	Walnut::Timer t;
#endif // RAY_DEBUG

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

	LOG("Load Scene '" << file_name << "': " << t.ElapsedMillis() << "ms");
}

void Loader::dump_oct(const char* file_name, Octree& octree)
{
#ifdef RAY_DEBUG
	Walnut::Timer t;
#endif // RAY_DEBUG

	std::fstream file(file_name, std::ios::binary | std::ios::out);

	// Write Nodes Size
	int size = octree.m_Nodes.size();
	file.write((char*) &size, sizeof(int));

	// Write Nodes
	for (OctreeNode& node : octree.m_Nodes)
	{
		file.write((char*) &node.first_child, sizeof(uint32_t));

		file.write((char*) &node.bottom_corner.x, sizeof(uint16_t) * 3);

		file.write((char*) &node.top_corner.x, sizeof(uint16_t) * 3);

		file.write((char*) &node.flags, sizeof(uint32_t));
	}

	file.close();

	LOG("Dumping Oct: " << t.ElapsedMillis());
}

void Loader::dump_scene(const char* file_name, Camera& camera, bool& render_light, bool& render_normal)
{
#ifdef RAY_DEBUG
	Walnut::Timer t;
#endif // RAY_DEBUG

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

	LOG("Dumping Scene: " << t.ElapsedMillis());
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

	octree.init(size_x, size_z, size_y);

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

		octree.insert_node(x, z, y, Loader::PALETTE[color_id]);
	}
}
