#pragma once

#include "../Ray.h"

#include <vector>
#include <queue>

// 6 bytes | 48 bit
struct u_shortV3
{
	uint16_t x, y, z;

	u_shortV3()
		: x(0), y(0), z(0) {}

	u_shortV3(uint16_t n)
		: x(n), y(n), z(n) {}

	u_shortV3(uint16_t x_, uint16_t y_, uint16_t z_)
		: x(x_), y(y_), z(z_) {}

	uint16_t max()
	{
		if (x > y)
			if (x > z) return x;
			else       return z;
		else
			if (y > z) return y;
			else       return z;
	}

	bool is_zero()
	{
		return x == 0 && y == 0 && z == 0;
	}

	glm::vec3 operator/(float n) const
	{
		return { x / n, y / n, z / n };
	}

	glm::vec3 operator+(const glm::vec3& v) const
	{
		return { x + v.x, y + v.y, z + v.z };
	}

	glm::vec3 operator+(const u_shortV3& v) const
	{
		return { x + v.x, y + v.y, z + v.z };
	}

	glm::vec3 operator-(u_shortV3& v) const
	{
		return { x - v.x, y - v.y, z - v.z };
	}

	bool operator>=(u_shortV3& v) const
	{
		return x >= v.x && y >= v.y && z >= v.z;
	}

	bool operator<=(u_shortV3& v) const
	{
		return x <= v.x && y <= v.y && z <= v.z;
	}

	bool operator==(const u_shortV3& v) const
	{
		return x == v.x && y == v.y && z == v.z;
	}

	bool operator==(const glm::vec3& v) const
	{
		return x == v.x && y == v.y && z == v.z;
	}
};

// 20 bytes | 180 bit
struct OctreeNode
{
	// Order: Top | Left | Front (TLF, TLB, TRF, TRB, BLF, BLB, BRF, BRB)
	uint32_t first_child;
	u_shortV3 bottom_corner, top_corner;

	// 1b IsFull | 1b HasData | 2b ChildFormat | 28b Data
	uint32_t flags;
	
	OctreeNode(u_shortV3& bottom_corn, u_shortV3& top_corn, uint8_t i, uint8_t child_count)
		: first_child(-1), flags(0)
	{
		calc_corners_by_pos_id(bottom_corn, top_corn, i, child_count);
		calculate_format();
	}

	OctreeNode(uint32_t f_child, u_shortV3& bottom_corn, u_shortV3& top_corn)
		: first_child(f_child), bottom_corner(bottom_corn), top_corner(top_corn), flags(0)
	{
		calculate_format();
	}

	OctreeNode(uint32_t f_child, u_shortV3& bottom_corn, u_shortV3& top_corn, uint32_t flags)
		: first_child(f_child), bottom_corner(bottom_corn), top_corner(top_corn), flags(flags)
	{
		calculate_format();
	}

	bool bounds_is_zero()
	{
		return top_corner == bottom_corner;
	}

	void calculate_format()
	{
		const glm::vec3 size = top_corner - bottom_corner;

		// Octree | 8 childs
		// flags &= 0xCFFFFFFF;
		if ((size.x > 0 && size.y > 0 && size.z > 0) || (size.x == 0 && size.y == 0 && size.z == 0))
		{
			flags |= 0x10000000;
			return;
		}

		if (((size.x > 0 && size.y > 0) || (size.y > 0 && size.z > 0) || (size.z > 0 && size.x > 0)) &&
			(size.x == 0 || size.y == 0 || size.z == 0)) // Quadtree | 4 childs
		{
			flags |= 0x20000000;
			return;
		}

		// Line | 2 childs
		flags |= 0x30000000;
	}

	void calc_corners_by_pos_id(u_shortV3& bottom_corn, u_shortV3& top_corn, uint8_t i, uint8_t child_count)
	{
		u_shortV3 mid = {
				(uint16_t)((top_corn.x + bottom_corn.x) / 2.0f),
				(uint16_t)((top_corn.y + bottom_corn.y) / 2.0f),
				(uint16_t)((top_corn.z + bottom_corn.z) / 2.0f)
		};

		if (child_count == 4)
		{
			if (bottom_corn.x == top_corn.x && i > 1)
			{
				i += 2;
			}
			else if (bottom_corn.y == top_corn.y)
			{
				i += 4;
			}
			else if (bottom_corn.z == top_corn.z)
			{
				i *= 2;
			}
		}
		else if (child_count == 2)
		{
			if (bottom_corn.x != top_corn.x)
			{
				i = i * 2 + 4;
			}
			else if (bottom_corn.y != top_corn.y)
			{
				i *= 4;
			}
			else if (bottom_corn.z != top_corn.z)
			{
				i += 4;
			}
		}

		/*
		if (mid.equal(bottom_corn))
		{
			bottom_corner = top_corner = bottom_corn;
		}
		*/

		switch (i)
		{
		case 0: // Top Left Front
			bottom_corner = u_shortV3{ bottom_corn.x, (uint16_t)(mid.y + 1), bottom_corn.z };
			top_corner = u_shortV3{ mid.x, top_corn.y, mid.z };
			return;
		case 1: // Top Left Back
			bottom_corner = u_shortV3{ bottom_corn.x, (uint16_t)(mid.y + 1), (uint16_t)(mid.z + 1) };
			top_corner = u_shortV3{ mid.x, top_corn.y, top_corn.z };
			return;
		case 2: // Top Right Front
			bottom_corner = u_shortV3{ (uint16_t)(mid.x + 1), (uint16_t)(mid.y + 1), bottom_corn.z };
			top_corner = u_shortV3{ top_corn.x, top_corn.y, mid.z };
			return;
		case 3: // Top Right Back
			bottom_corner = u_shortV3{ (uint16_t)(mid.x + 1), (uint16_t)(mid.y + 1), (uint16_t)(mid.z + 1) };
			top_corner = u_shortV3{ top_corn.x, top_corn.y, top_corn.z };
			return;
		case 4: // Bottom Left Front
			bottom_corner = u_shortV3{ bottom_corn.x, bottom_corn.y, bottom_corn.z };
			top_corner = u_shortV3{ mid.x, mid.y, mid.z };
			return;
		case 5: // Bottom Left Back
			bottom_corner = u_shortV3{ bottom_corn.x, bottom_corn.y, (uint16_t)(mid.z + 1) };
			top_corner = u_shortV3{ mid.x, mid.y, top_corn.z };
			return;
		case 6: // Bottom Right Front
			bottom_corner = u_shortV3{ (uint16_t)(mid.x + 1), bottom_corn.y, bottom_corn.z };
			top_corner = u_shortV3{ top_corn.x, mid.y, mid.z };
			return;
		case 7: // Bottom Right Back
			bottom_corner = u_shortV3{ (uint16_t)(mid.x + 1), bottom_corn.y, (uint16_t)(mid.z + 1) };
			top_corner = u_shortV3{ top_corn.x, mid.y, top_corn.z };
			return;
		}
	}

	uint32_t data()
	{
		return flags & 0x0FFFFFFF;
	}

	bool is_full()
	{
		return (flags & 0x80000000) == 0x80000000; // 0b10
	}

	bool has_data()
	{
		return (flags & 0x40000000) == 0x40000000; // 0b01
	}

	uint8_t child_format()
	{
		return (flags >> 28) & 0x3;
	}

	uint8_t child_count()
	{
		switch (child_format())
		{
		case 1:  return 8;
		case 2:  return 4;
		case 3:  return 2;
		default: return 0;
		}
	}

	float calc_dist_from_ray_pow2(const glm::vec3& rayOrigin)
	{
		u_shortV3 mid = {
			(uint16_t)((top_corner.x + bottom_corner.x) / 2.0f),
			(uint16_t)((top_corner.y + bottom_corner.y) / 2.0f),
			(uint16_t)((top_corner.z + bottom_corner.z) / 2.0f)
		};
		return (mid.x - rayOrigin.x) * (mid.x - rayOrigin.x) +
			(mid.y - rayOrigin.y) * (mid.y - rayOrigin.y) +
			(mid.z - rayOrigin.z) * (mid.z - rayOrigin.z);
	}

	glm::vec3 get_color()
	{
		return glm::vec3
		{
			((flags >> 16) & 0xFF) / 255.0f,
			((flags >> 8) & 0xFF) / 255.0f,
			(flags & 0xFF) / 255.0f,
		};
	}
};

class Octree
{
public:
	Octree();

	void init(uint16_t size_x, uint16_t size_y, uint16_t size_z);

	void add_node(uint32_t first_child, u_shortV3 bottom_corner, u_shortV3 top_corner, uint32_t flags);

	void insert_node(uint16_t x, uint16_t y, uint16_t z, uint32_t data);
	void insert_range_node(u_shortV3& min_bound, u_shortV3& max_bound, uint32_t data);

	void collapse_nodes();

	void build_lods();

	void calculate_max_depth();

	uint16_t find_node_data(uint16_t x, uint16_t y, uint16_t z);

	bool find_node(uint16_t x, uint16_t y, uint16_t z, OctreeNode*& result, short& max_depth);

	bool ray_travel(const Ray& ray, RayHit* hit);

	static float ray_intersect_box(const u_shortV3& min, const u_shortV3& max, const Ray& ray);
	static float ray_intersect_box(const glm::vec3& min, const glm::vec3& max, const Ray& ray);

private:
	void subdivide_node(OctreeNode*& mod_node, uint32_t& first_child);

	std::vector<std::pair<uint32_t, std::deque<uint32_t>>> check_collapse_path(uint32_t node_idx);

	std::vector<uint32_t> collapse(uint32_t& node_idx);

	std::tuple<bool, uint32_t, std::vector<uint32_t>> check_recursively_equal_childs(OctreeNode* node);

	void remove_nodes(std::vector<uint32_t>& sub_nodes);

	uint32_t calculate_node_lod(OctreeNode* node);

	uint8_t find_max_depth(OctreeNode* node);

	uint16_t find_node_data(uint16_t x, uint16_t y, uint16_t z, OctreeNode* node);

	bool find_node(uint16_t x, uint16_t y, uint16_t z, OctreeNode* node, OctreeNode*& result, short& max_depth);

	bool proc_ray_travel(const Ray& ray, OctreeNode* node, RayHit* hit);

	static bool point_in_box(const u_shortV3& min, const u_shortV3& max, const glm::vec3& p);

public:
	glm::vec3 m_Position;
	uint8_t m_MaxDepth;
	std::vector<OctreeNode> m_Nodes;
};
