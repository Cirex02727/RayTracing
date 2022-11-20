#pragma once

#include <vector>

#include "../Ray.h"

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

	u_shortV3 add(u_shortV3& v)
	{
		return u_shortV3(x + v.x, y + v.y, z + v.z);
	}

	glm::vec3 sub(const glm::vec3& v)
	{
		return glm::vec3(x - v.x, y - v.y, z - v.z);
	}

	glm::vec3 mul(const glm::vec3& v)
	{
		return glm::vec3(x * v.x, y * v.y, z * v.z);
	}

	bool greater_equals(u_shortV3& v)
	{
		return x >= v.x && y >= v.y && z >= v.z;
	}

	bool less_equals(u_shortV3& v)
	{
		return x <= v.x && y <= v.y && z <= v.z;
	}

	bool equal(u_shortV3& v)
	{
		return x == v.x && y == v.y && z == v.z;
	}

	bool is_zero()
	{
		return x == 0 && y == 0 && z == 0;
	}
};

// 20 bytes | 180 bit
struct OctreeNode
{
	// Order: Top | Left | Front (TLF, TLB, TRF, TRB, BLF, BLB, BRF, BRB)
	uint32_t first_child;
	u_shortV3 bottom_corner, top_corner;

	// 1b IsFull | 1b HasData | 30b Data
	uint32_t flags;
	
	OctreeNode(u_shortV3& bottom_corn, u_shortV3& top_corn, uint8_t i)
		: first_child(-1), flags(0)
	{
		calc_corners_by_pos_id(bottom_corn, top_corn, i);
	}

	OctreeNode(uint32_t f_child, u_shortV3& bottom_corn, u_shortV3& top_corn)
		: first_child(f_child), bottom_corner(bottom_corn), top_corner(top_corn), flags(0) {}

	bool bounds_is_zero()
	{
		return top_corner.equal(bottom_corner);
	}

	void calc_corners_by_pos_id(u_shortV3& bottom_corn, u_shortV3& top_corn, uint8_t i)
	{
		u_shortV3 mid = {
			(uint16_t) ((top_corn.x + bottom_corn.x) * 0.5f),
			(uint16_t) ((top_corn.y + bottom_corn.y) * 0.5f),
			(uint16_t) ((top_corn.z + bottom_corn.z) * 0.5f)
		};

		if (mid.equal(bottom_corn))
		{
			bottom_corner = top_corner = bottom_corn;
		}

		switch (i)
		{
		case 0: // Top Left Front
			bottom_corner = u_shortV3{ bottom_corn.x, (uint16_t) (mid.y + 1), bottom_corn.z };
			top_corner =    u_shortV3{ mid.x, top_corn.y, mid.z };
			return;
		case 1: // Top Left Back
			bottom_corner = u_shortV3{ bottom_corn.x, (uint16_t) (mid.y + 1), (uint16_t) (mid.z + 1) };
			top_corner =    u_shortV3{ mid.x, top_corn.y, top_corn.z };
			return;
		case 2: // Top Right Front
			bottom_corner = u_shortV3{ (uint16_t) (mid.x + 1), (uint16_t) (mid.y + 1), bottom_corn.z };
			top_corner =    u_shortV3{ top_corn.x, top_corn.y, mid.z };
			return;
		case 3: // Top Right Back
			bottom_corner = u_shortV3{ (uint16_t) (mid.x + 1), (uint16_t) (mid.y + 1), (uint16_t) (mid.z + 1) };
			top_corner =    u_shortV3{ top_corn.x, top_corn.y, top_corn.z };
			return;
		case 4: // Bottom Left Front
			bottom_corner = u_shortV3{ bottom_corn.x, bottom_corn.y, bottom_corn.z };
			top_corner =    u_shortV3{ mid.x, mid.y, mid.z };
			return;
		case 5: // Bottom Left Back
			bottom_corner = u_shortV3{ bottom_corn.x, bottom_corn.y, (uint16_t) (mid.z + 1) };
			top_corner =    u_shortV3{ mid.x, mid.y, top_corn.z };
			return;
		case 6: // Bottom Right Front
			bottom_corner = u_shortV3{ (uint16_t) (mid.x + 1), bottom_corn.y, bottom_corn.z };
			top_corner =    u_shortV3{ top_corn.x, mid.y, mid.z };
			return;
		case 7: // Bottom Right Back
			bottom_corner = u_shortV3{ (uint16_t) (mid.x + 1), bottom_corn.y, (uint16_t) (mid.z + 1) };
			top_corner =    u_shortV3{ top_corn.x, mid.y, top_corn.z };
			return;
		}
	}

	bool is_full()
	{
		return (flags & 0x80000000) == 0x80000000; // 0b10
	}

	bool has_data()
	{
		return (flags & 0x40000000) == 0x40000000; // 0b01
	}

	float calc_dist_from_ray_pow2(const glm::vec3& rayOrigin)
	{
		u_shortV3 mid = {
			(uint16_t)((top_corner.x + bottom_corner.x) * 0.5f),
			(uint16_t)((top_corner.y + bottom_corner.y) * 0.5f),
			(uint16_t)((top_corner.z + bottom_corner.z) * 0.5f)
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

	void init(uint16_t max_size);

	void insert_node(uint16_t x, uint16_t y, uint16_t z, uint32_t data);
	void insert_range_node(u_shortV3& min_bound, u_shortV3& max_bound, uint32_t data);

	uint16_t find_node_data(uint16_t x, uint16_t y, uint16_t z);

	OctreeNode* find_node(uint16_t x, uint16_t y, uint16_t z);

	std::tuple<OctreeNode*, glm::vec3, glm::vec3> ray_travel(const Ray& ray);

	static double ray_intersect_box(u_shortV3& min, u_shortV3& max, const Ray& ray);

private:
	void subdivide_node(OctreeNode*& mod_node, uint32_t& first_child);

	uint16_t find_node_data(uint16_t x, uint16_t y, uint16_t z, OctreeNode* node);

	OctreeNode* find_node(uint16_t x, uint16_t y, uint16_t z, OctreeNode* node);

	std::tuple<OctreeNode*, glm::vec3, glm::vec3> proc_ray_travel(const Ray& ray, OctreeNode* node);

	bool point_in_box(u_shortV3& min, u_shortV3& max, glm::vec3& p);

public:
	std::vector<OctreeNode> nodes;
};
