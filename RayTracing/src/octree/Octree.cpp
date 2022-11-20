#include "Octree.h"

#include <queue>
#include <Walnut/Timer.h>

Octree::Octree() {}

void Octree::init(uint16_t max_size)
{
	nodes.emplace_back(1, u_shortV3{ 0 }, u_shortV3{ (uint16_t)(max_size - 1) });

	OctreeNode* node = &nodes[0];
	uint32_t firstChild = -1;
	subdivide_node(node, firstChild);
}

void Octree::insert_node(uint16_t x, uint16_t y, uint16_t z, uint32_t data)
{
	OctreeNode* node = &nodes[0];
	uint32_t curr_index = 0, prev_index = -1;

	uint16_t mid_x, mid_y, mid_z;
	uint8_t pos;

	while (!node->bounds_is_zero())
	{
		mid_x = (uint16_t) ((node->bottom_corner.x + node->top_corner.x) * 0.5f);
		mid_y = (uint16_t) ((node->bottom_corner.y + node->top_corner.y) * 0.5f);
		mid_z = (uint16_t) ((node->bottom_corner.z + node->top_corner.z) * 0.5f);

		pos = -1;
		if (x <= mid_x)
		{
			if (y <= mid_y)
			{
				if (z <= mid_z)
					pos = 4;
				else
					pos = 5;
			}
			else
			{
				if (z <= mid_z)
					pos = 0;
				else
					pos = 1;
			}
		}
		else
		{
			if (y <= mid_y)
			{
				if (z <= mid_z)
					pos = 6;
				else
					pos = 7;
			}
			else
			{
				if (z <= mid_z)
					pos = 2;
				else
					pos = 3;
			}
		}

		prev_index = curr_index;
		curr_index = node->first_child + pos;
		if (node->first_child == -1 ||
			curr_index >= nodes.size())
		{
			subdivide_node(node, curr_index);
			curr_index += pos;

			node = &nodes[prev_index];
		}

		node->flags |= 0x40000000; // has data 0b01
		node = &nodes[curr_index];
	}

	node->flags = 0xC0000000 | data; // has full data 0b11
}

void Octree::insert_range_node(u_shortV3& min_bound, u_shortV3& max_bound, uint32_t data)
{
	std::queue<uint32_t> to_process = {};
	to_process.push(0);

	std::vector<uint8_t> to_add = {};
	to_add.reserve(8);

	uint32_t index;
	uint16_t mid_x, mid_y, mid_z;

	while (to_process.size() > 0)
	{
		index = to_process.front();
		to_process.pop();

		OctreeNode* node = &nodes[index];
		if (node->bounds_is_zero())
		{
			node->flags = 0xC0000000 | data; // has full data 0b11
			continue;
		}

		mid_x = (uint16_t) ((node->bottom_corner.x + node->top_corner.x) * 0.5f);
		mid_y = (uint16_t) ((node->bottom_corner.y + node->top_corner.y) * 0.5f);
		mid_z = (uint16_t) ((node->bottom_corner.z + node->top_corner.z) * 0.5f);

		if (min_bound.less_equals(node->bottom_corner) && max_bound.greater_equals(node->top_corner))
		{
			node = &nodes[index];
			node->flags = 0xC0000000 | data; // has full data 0b11

			continue;
		}

		node->flags |= 0x40000000; // has data 0b01

		if (min_bound.x <= mid_x)
		{
			if (max_bound.y > mid_y)
			{
				if (min_bound.z <= mid_z)
					to_add.push_back(0);

				if (max_bound.z > mid_z)
					to_add.push_back(1);
			}

			if (min_bound.y <= mid_y)
			{
				if (min_bound.z <= mid_z)
					to_add.push_back(4);

				if (max_bound.z > mid_z)
					to_add.push_back(5);
			}
		}

		if (max_bound.x > mid_x)
		{
			if (max_bound.y > mid_y)
			{
				if (min_bound.z <= mid_z)
					to_add.push_back(2);

				if (max_bound.z > mid_z)
					to_add.push_back(3);
			}

			if (min_bound.y <= mid_y)
			{
				if (min_bound.z <= mid_z)
					to_add.push_back(6);

				if (max_bound.z > mid_z)
					to_add.push_back(7);
			}
		}

		index = node->first_child;
		for (uint8_t n : to_add)
		{
			if (index == -1 ||
				index + n >= nodes.size())
					subdivide_node(node, index);

			to_process.push(index + n);
		}

		/*
		if (min_bound.x <= mid_x && max_bound.y > mid_y && min_bound.z <= mid_z)
		{
			curr_index = node->first_child;
			to_process.emplace_back(&nodes[curr_index], curr_index);
		}
		if (min_bound.x <= mid_x && max_bound.y > mid_y && max_bound.z > mid_z)
		{
			curr_index = node->first_child + 1;
			to_process.emplace_back(&nodes[curr_index], curr_index);
		}
		if (max_bound.x > mid_x && max_bound.y > mid_y && min_bound.z <= mid_z)
		{
			curr_index = node->first_child + 2;
			to_process.emplace_back(&nodes[curr_index], curr_index);
		}
		if (max_bound.x > mid_x && max_bound.y > mid_y && max_bound.z > mid_z)
		{
			curr_index = node->first_child + 3;
			to_process.emplace_back(&nodes[curr_index], curr_index);
		}
		if (min_bound.x <= mid_x && min_bound.y <= mid_y && min_bound.z <= mid_z)
		{
			curr_index = node->first_child + 4;
			to_process.emplace_back(&nodes[curr_index], curr_index);
		}
		if (min_bound.x <= mid_x && min_bound.y <= mid_y && max_bound.z > mid_z)
		{
			curr_index = node->first_child + 5;
			to_process.emplace_back(&nodes[curr_index], curr_index);
		}
		if (max_bound.x > mid_x && min_bound.y <= mid_y && min_bound.z <= mid_z)
		{
			curr_index = node->first_child + 6;
			to_process.emplace_back(&nodes[curr_index], curr_index);
		}
		if (max_bound.x > mid_x && min_bound.y <= mid_y && max_bound.z > mid_z)
		{
			curr_index = node->first_child + 7;
			to_process.emplace_back(&nodes[curr_index], curr_index);
		}
		*/

		to_add.clear();
	}
}

void Octree::subdivide_node(OctreeNode*& node, uint32_t& first_child)
{
	node->first_child = first_child = (uint32_t) (nodes.size());
	u_shortV3 bot = node->bottom_corner, top = node->top_corner;

	for (uint8_t i = 0; i < 8; i++)
		nodes.emplace_back(bot, top, i);
}

uint16_t Octree::find_node_data(uint16_t x, uint16_t y, uint16_t z, OctreeNode* node)
{
	if (node->bounds_is_zero() || node->is_full())
		return node->flags;

	uint16_t mid_x = (uint16_t) ((node->bottom_corner.x + node->top_corner.x) * 0.5f);
	uint16_t mid_y = (uint16_t) ((node->bottom_corner.y + node->top_corner.y) * 0.5f);
	uint16_t mid_z = (uint16_t) ((node->bottom_corner.z + node->top_corner.z) * 0.5f);

	uint8_t pos = -1;
	if (x <= mid_x)
	{
		if (y <= mid_y)
		{
			if (z <= mid_z)
				pos = 4;
			else
				pos = 5;
		}
		else
		{
			if (z <= mid_z)
				pos = 0;
			else
				pos = 1;
		}
	}
	else
	{
		if (y <= mid_y)
		{
			if (z <= mid_z)
				pos = 6;
			else
				pos = 7;
		}
		else
		{
			if (z <= mid_z)
				pos = 2;
			else
				pos = 3;
		}
	}

	if (node->first_child == -1 ||
		node->first_child + pos >= nodes.size())
		return 0;

	return find_node_data(x, y, z, &nodes[node->first_child + pos]);
}

OctreeNode* Octree::find_node(uint16_t x, uint16_t y, uint16_t z, OctreeNode* node)
{
	if (node->bounds_is_zero() || node->is_full())
		return node;

	uint16_t mid_x = (uint16_t) ((node->bottom_corner.x + node->top_corner.x) * 0.5f);
	uint16_t mid_y = (uint16_t) ((node->bottom_corner.y + node->top_corner.y) * 0.5f);
	uint16_t mid_z = (uint16_t) ((node->bottom_corner.z + node->top_corner.z) * 0.5f);

	uint8_t pos = -1;
	if (x <= mid_x)
	{
		if (y <= mid_y)
		{
			if (z <= mid_z)
				pos = 4;
			else
				pos = 5;
		}
		else
		{
			if (z <= mid_z)
				pos = 0;
			else
				pos = 1;
		}
	}
	else
	{
		if (y <= mid_y)
		{
			if (z <= mid_z)
				pos = 6;
			else
				pos = 7;
		}
		else
		{
			if (z <= mid_z)
				pos = 2;
			else
				pos = 3;
		}
	}

	if (node->first_child == -1 ||
		node->first_child + pos >= nodes.size())
		return nullptr;

	return find_node(x, y, z, &nodes[node->first_child + pos]);
}

// Math3D OCtree https://www.math3d.org/QtfRmk23Y
std::tuple<OctreeNode*, glm::vec3, glm::vec3> Octree::proc_ray_travel(const Ray& ray, OctreeNode* node)
{
	double d = ray_intersect_box(node->bottom_corner, node->top_corner, ray);
	if (d < 0 && !point_in_box(node->bottom_corner, node->top_corner, (glm::vec3) ray.Origin))
		return { nullptr, glm::vec3(), glm::vec3() };

	glm::vec3 ray_pos = d < 0 ? ray.Origin : ray.Origin + ray.Direction * glm::vec3(d);
	glm::vec3 pos = glm::round(ray_pos);

	glm::vec3 step = glm::sign(ray.Direction);
	glm::vec3 tDelta = glm::vec3{
		ray.Direction.x == 0 ? FLT_MIN : step.x / ray.Direction.x,
		ray.Direction.y == 0 ? FLT_MIN : step.y / ray.Direction.y,
		ray.Direction.z == 0 ? FLT_MIN : step.z / ray.Direction.z
	};

	float tMaxX, tMaxY, tMaxZ;

	glm::vec3 fr = ray_pos - pos;

	tMaxX = tDelta.x * ((ray.Direction.x > 0.0) ? (1.0f - fr.x) : fr.x);
	tMaxY = tDelta.y * ((ray.Direction.y > 0.0) ? (1.0f - fr.y) : fr.y);
	tMaxZ = tDelta.z * ((ray.Direction.z > 0.0) ? (1.0f - fr.z) : fr.z);

	glm::vec3 norm{};
	const int maxTrace = ray.MaxDistance - d;

	bool isInBox = false;

	for (int i = 0; i < maxTrace; i++)
	{
		if (point_in_box(node->bottom_corner, node->top_corner, pos))
		{
			isInBox = true;
			OctreeNode* subNode = find_node((uint16_t) pos.x, (uint16_t)pos.y, (uint16_t)pos.z);
			if (subNode != nullptr && subNode->is_full())
			{
				if (norm.x == 0 && norm.y == 0 && norm.z == 0)
				{
					glm::vec3 localPoint = glm::abs(ray_pos - (pos + 0.5f));
					if (std::abs(0.5f - localPoint.x) < std::abs(0.5f - localPoint.y))
						if (std::abs(0.5f - localPoint.x) < std::abs(0.5f - localPoint.z))  norm = glm::vec3(pos.x == 16 ? 1 : -1, 0, 0);
						else                             norm = glm::vec3(0, 0, pos.z == 16 ? 1 : -1);
					else
						if (std::abs(0.5f - localPoint.y) < std::abs(0.5f - localPoint.z)) norm = glm::vec3(0, pos.y == 16 ? 1 : -1, 0);
						else                             norm = glm::vec3(0, 0, pos.z == 16 ? 1 : -1);
				}
				return { subNode, pos, norm };
			}
		}
		else if (isInBox)
			break;

		if (tMaxX < tMaxY) {
			if (tMaxZ < tMaxX) {
				tMaxZ += tDelta.z;
				pos.z += step.z;
				norm = glm::vec3(0, 0, -step.z);
			}
			else {
				tMaxX += tDelta.x;
				pos.x += step.x;
				norm = glm::vec3(-step.x, 0, 0);
			}
		}
		else {
			if (tMaxZ < tMaxY) {
				tMaxZ += tDelta.z;
				pos.z += step.z;
				norm = glm::vec3(0, 0, -step.z);
			}
			else {
				tMaxY += tDelta.y;
				pos.y += step.y;
				norm = glm::vec3(0, -step.y, 0);
			}
		}
	}

	return { nullptr, glm::vec3(), glm::vec3() };
}

double Octree::ray_intersect_box(u_shortV3& min, u_shortV3& max, const Ray& ray)
{
	double tx1 = (min.x - ray.Origin.x) * ray.InvDirection.x;
	double tx2 = ((max.x + 1) - ray.Origin.x) * ray.InvDirection.x;

	double tmin = std::min(tx1, tx2);
	double tmax = std::max(tx1, tx2);

	double ty1 = (min.y - ray.Origin.y) * ray.InvDirection.y;
	double ty2 = ((max.y + 1) - ray.Origin.y) * ray.InvDirection.y;

	tmin = std::max(tmin, std::min(ty1, ty2));
	tmax = std::min(tmax, std::max(ty1, ty2));

	double tz1 = (min.z - ray.Origin.z) * ray.InvDirection.z;
	double tz2 = ((max.z + 1) - ray.Origin.z) * ray.InvDirection.z;

	tmin = std::max(tmin, std::min(tz1, tz2));
	tmax = std::min(tmax, std::max(tz1, tz2));

	return (tmax >= std::max(0.0, tmin) && tmin < 10000) ? tmin : -1;
}

bool Octree::point_in_box(u_shortV3& min, u_shortV3& max, glm::vec3& p)
{
	return min.x <= p.x && p.x <= (max.x + 1) && min.y <= p.y && p.y <= (max.y + 1) && min.z <= p.z && p.z <= (max.z + 1);
}

uint16_t Octree::find_node_data(uint16_t x, uint16_t y, uint16_t z)
{
	return find_node_data(x, y, z, &nodes[0]);
}

OctreeNode* Octree::find_node(uint16_t x, uint16_t y, uint16_t z)
{
	return find_node(x, y, z, &nodes[0]);
}

std::tuple<OctreeNode*, glm::vec3, glm::vec3> Octree::ray_travel(const Ray& ray)
{
	return proc_ray_travel(ray, &nodes[0]);
}
