#include "Octree.h"

#include <Walnut/Timer.h>
#include <unordered_map>

Octree::Octree() {}

void Octree::init(uint16_t max_size)
{
	nodes.emplace_back(1, u_shortV3{ 0 }, u_shortV3{ (uint16_t)(max_size - 1) });

	OctreeNode* node = &nodes[0];
	uint32_t firstChild = -1;
	subdivide_node(node, firstChild);
}

void Octree::add_node(uint32_t first_child, u_shortV3 bottom_corner, u_shortV3 top_corner, uint32_t flags)
{
	nodes.emplace_back(first_child, bottom_corner, top_corner, flags);
}

void Octree::insert_node(uint16_t x, uint16_t y, uint16_t z, uint32_t data)
{
	OctreeNode* node = &nodes[0];
	uint32_t curr_index = 0, prev_index = -1;

	uint16_t mid_x, mid_y, mid_z;
	uint8_t pos;

	// TODO: Collapse nodes
	std::deque<uint32_t> try_collapse;

	while (!node->bounds_is_zero())
	{
		mid_x = (uint16_t) ((node->bottom_corner.x + node->top_corner.x) / 2.0f);
		mid_y = (uint16_t) ((node->bottom_corner.y + node->top_corner.y) / 2.0f);
		mid_z = (uint16_t) ((node->bottom_corner.z + node->top_corner.z) / 2.0f);

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

		if (node->has_data() && prev_index != 0)
			try_collapse.push_back(prev_index);
		
		node->flags |= 0x40000000; // has data 0b01
		node = &nodes[curr_index];
	}

	node->flags = 0xC0000000 | data; // has full data 0b11


	// Try Collapse
	if (try_collapse.size() == 0)
		return;

	std::sort(try_collapse.begin(), try_collapse.end(), std::greater<uint32_t>());

	for (uint32_t i = 0; i < try_collapse.size(); i++)
	{
		std::vector<uint32_t> removed_nodes = {};// collapse(try_collapse[i]);
		if (removed_nodes.size() == 0)
			break;

		int a = 0;
	}
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

		mid_x = (uint16_t) ((node->bottom_corner.x + node->top_corner.x) / 2.0f);
		mid_y = (uint16_t) ((node->bottom_corner.y + node->top_corner.y) / 2.0f);
		mid_z = (uint16_t) ((node->bottom_corner.z + node->top_corner.z) / 2.0f);

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

		to_add.clear();
	}
}

void Octree::collapse_nodes()
{
	std::cout << "Check collapse paths" << std::endl;

	std::vector<std::pair<uint32_t, std::deque<uint32_t>>> collapse_paths = check_collapse_path(0);

	std::sort(collapse_paths.begin(), collapse_paths.end(),
		[](auto a, auto b)
		{
			return a.second.size() > b.second.size();
		});

	while (collapse_paths.size())
	{
		std::cout << "Process collapse depth: " << collapse_paths[0].second.size() << " paths: " << collapse_paths.size() << std::endl;

		// REFACTOR // CHANGE (Remove nodes in the for loop) {
		std::vector<std::pair<uint32_t, uint32_t>> to_remove;
		to_remove.reserve(collapse_paths.size() * 8);

		uint32_t start_delete_idx = -1, end_delete_idx = -1;
		for (int i = collapse_paths.size() - 1; i >= 0; i--)
		{
			auto& node_info = collapse_paths[i];
			if (i != collapse_paths.size() - 1 && node_info.second[0] == collapse_paths[i + 1].second[0])
			{
				if (start_delete_idx == -1)
				{
					start_delete_idx = end_delete_idx = i + 1;
				}
				start_delete_idx--;
				continue;
			}

			if (start_delete_idx != -1)
			{
				collapse_paths.erase(collapse_paths.begin() + start_delete_idx, collapse_paths.begin() + end_delete_idx);
				start_delete_idx = end_delete_idx = -1;
			}

			uint32_t first_child = nodes[node_info.second[0]].first_child;

			if (first_child == -1)
				continue;

			// Check node can collapse
			uint32_t sample_data = nodes[first_child].flags & 0x3FFFFFFF;
			bool can_collapse = true;
			for (uint8_t j = 1; j < 8; j++)
			{
				if ((nodes[first_child + j].flags & 0x3FFFFFFF) != sample_data)
				{
					can_collapse = false;
					break;
				}
			}

			if (!can_collapse)
			{
				collapse_paths.erase(collapse_paths.begin() + i);
				continue;
			}

			// Remove all childs
			for (uint8_t j = 0; j < 8; j++)
				to_remove.push_back({ first_child + j, node_info.first });
		}
		// REFACTOR // CHANGE }

		if (to_remove.size() == 0)
		{
			// Remove all valid steps
			for (int i = collapse_paths.size() - 1; i >= 0; i--)
				collapse_paths.erase(collapse_paths.begin() + i);

			continue;
		}

		// Sort by descending
		std::sort(to_remove.begin(), to_remove.end(),
			[](auto& a, auto& b)
			{
				return a.first > b.first;
			});

		std::cout << "Deleting " << to_remove.size() << " nodes" << std::endl;

		// Remove all nodes
		uint32_t start_idx = to_remove[0].first, end_idx = to_remove[0].first;
		for (int i = 1; i < to_remove.size(); i++)
		{
			if (to_remove[i].first + 1 == start_idx)
			{
				start_idx = to_remove[i].first;

				if(i != to_remove.size() - 1)
					continue;
			}

			nodes.erase(nodes.begin() + start_idx, nodes.begin() + end_idx + 1);
			start_idx = end_idx = to_remove[i].first;
		}


		// Reset all childs pointer
		std::cout << "Shifting all pointer" << std::endl;

		for (uint32_t i = 1; i < nodes.size(); i++)
		{
			OctreeNode* sub_node = &nodes[i];
			if (sub_node->first_child == -1)
				continue;

			// Binary Search
			int start_bound = 0, end_bound = to_remove.size() - 1, idx = -1;
			while (end_bound >= start_bound)
			{
				int mid_bound = start_bound + (end_bound - start_bound) / 2;

				if (to_remove[mid_bound].first == sub_node->first_child)
				{
					idx = mid_bound;
					break;
				}

				if (to_remove[mid_bound].first > sub_node->first_child)
				{
					start_bound = mid_bound + 1;
					continue;
				}

				end_bound = mid_bound - 1;
			}

			// If child removed, reset pointer
			if (idx != -1)
			{
				sub_node->first_child = -1;
				sub_node->flags = 0xC0000000 | to_remove[idx].second; // Has full data with sample
				continue;
			}

			// Shift all pointers
			uint32_t amount = 0;
			if (sub_node->first_child >= to_remove[0].first)
				amount = to_remove.size();
			else if (sub_node->first_child >= to_remove[to_remove.size() - 1].first)
			{
				int start_bound = 0, end_bound = to_remove.size() - 1, idx = 0;
				while (end_bound >= start_bound)
				{
					int mid_bound = start_bound + (end_bound - start_bound) / 2;

					if (to_remove[mid_bound].first <= sub_node->first_child && to_remove[mid_bound - 1].first > sub_node->first_child)
					{
						idx = mid_bound;
						break;
					}

					if (to_remove[mid_bound].first > sub_node->first_child)
					{
						start_bound = mid_bound + 1;
						continue;
					}

					end_bound = mid_bound - 1;
				}

				if (idx <= 0)
				{
					int a = 0;
				}

				amount = to_remove.size() - idx;
			}

			/*
			uint32_t amount = 0;
			for (int j = to_remove.size() - 1; j >= 0; j--)
			{
				if (sub_node->first_child < to_remove[j].first)
					break;

				amount++;
			}
			*/

			sub_node->first_child -= amount;
		}

		std::cout << "Shifting all paths" << std::endl;

		// Decrease valid steps
		for (uint32_t i = 0; i < collapse_paths.size(); i++)
		{
			auto& [_, path] = collapse_paths[i];
			path.erase(path.begin());

			// Shift all pointers
			for (uint32_t p = 0; p < path.size(); p++)
			{
				// Binary Search lower elements
				uint32_t amount = 0;
				if (path[p] >= to_remove[0].first)
					amount = to_remove.size();
				else if (path[p] >= to_remove[to_remove.size() - 1].first)
				{
					int start_bound = 0, end_bound = to_remove.size() - 1, idx = 0;
					while (end_bound >= start_bound)
					{
						int mid_bound = start_bound + (end_bound - start_bound) / 2;

						if (to_remove[mid_bound].first <= path[p] && to_remove[mid_bound - 1].first > path[p])
						{
							idx = mid_bound;
							break;
						}

						if (to_remove[mid_bound].first > path[p])
						{
							start_bound = mid_bound + 1;
							continue;
						}

						end_bound = mid_bound - 1;
					}

					if (idx <= 0)
					{
						int a = 0;
					}

					amount = to_remove.size() - idx;
				}

				/*
				uint32_t el = path[p];
				uint32_t count = std::count_if(to_remove.begin(), to_remove.end(),
					[&el](std::pair<uint32_t, uint32_t>& e)
					{
						return el > e.first;
					});
				*/

				/*
				uint32_t amount = 0;
				for (int j = to_remove.size() - 1; j >= 0; j--)
				{
					if (path[p] < to_remove[j].first)
						break;

					amount++;
				}
				*/

				path[p] -= amount;
			}
		}

		to_remove.clear();

		std::cout << "Check empty paths" << std::endl;

		// Remove all empty
		for (int i = collapse_paths.size() - 1; i >= 0; i--)
			if (collapse_paths[i].second.size() == 0)
				collapse_paths.erase(collapse_paths.begin() + i);
	}
}

void Octree::subdivide_node(OctreeNode*& node, uint32_t& first_child)
{
	node->first_child = first_child = (uint32_t) (nodes.size());
	u_shortV3 bot = node->bottom_corner, top = node->top_corner;

	for (uint8_t i = 0; i < 8; i++)
		nodes.emplace_back(bot, top, i);
}

std::vector<std::pair<uint32_t, std::deque<uint32_t>>> Octree::check_collapse_path(uint32_t node_idx)
{
	OctreeNode* node = &nodes[node_idx];
	if (node->first_child == -1)
		return {};

	std::vector<std::pair<uint32_t, std::deque<uint32_t>>> paths;

	uint32_t sample_data = nodes[node->first_child].flags & 0x3FFFFFFF;
	bool valid_end_node = true;
	for (uint8_t i = 0; i < 8; i++)
	{
		uint32_t idx = node->first_child + i;
		OctreeNode* sub_node = &nodes[idx];

		if (!sub_node->has_data() ||
			(sub_node->is_full() && (sub_node->flags & 0x3FFFFFFF) != sample_data))
		{
			valid_end_node = false;
			continue;
		}

		if (!sub_node->is_full())
			valid_end_node = false;

		std::vector<std::pair<uint32_t, std::deque<uint32_t>>> sub_paths = check_collapse_path(idx);
		if (sub_paths.size() == 0)
			continue;

		if(paths.capacity() < paths.size() + sub_paths.size())
			paths.reserve(paths.size() + sub_paths.size());

		paths.insert(paths.end(), sub_paths.begin(), sub_paths.end());
	}

	if (node_idx != 0)
	{
		if (paths.size() == 0)
		{
			if (valid_end_node && sample_data == 0)
			{
				int a = 0;
			}

			if (valid_end_node)
				paths.push_back({ sample_data, { node_idx } });
		}
		else
		{
			for (uint32_t i = 0; i < paths.size(); i++)
				paths[i].second.push_back(node_idx);
		}
	}

	return paths;
}

std::vector<uint32_t> Octree::collapse(uint32_t& node_idx)
{
	OctreeNode* node = &nodes[node_idx];
	if (node->first_child == -1)
		return {};

	// Check Collapse
	auto [check, sample, checked_nodes] = check_recursively_equal_childs(node);
	if (!check || checked_nodes.size() == 0)
		return {};

	// Collapse
	remove_nodes(checked_nodes);
	
	// Reset all childs pointer
	for (uint32_t i = 0; i < nodes.size(); i++)
	{
		OctreeNode* sub_node = &nodes[i];
		if (sub_node->first_child == -1)
			continue;

		if (std::find(checked_nodes.begin(), checked_nodes.end(), sub_node->first_child) != checked_nodes.end())
		{
			sub_node->first_child = -1;
			sub_node->flags = 0xC0000000 | sample; // Has full data with sample
			continue;
		}

		// Shift all pointers
		uint32_t amount = 0;
		for (int j = checked_nodes.size() - 1; j >= 0; j--)
		{
			if (sub_node->first_child < checked_nodes[j])
				break;

			amount++;
		}

		sub_node->first_child -= amount;
	}

	return checked_nodes;
}

std::tuple<bool, uint32_t, std::vector<uint32_t>> Octree::check_recursively_equal_childs(OctreeNode* node)
{
	std::vector<uint32_t> checked_nodes;

	uint32_t sample_data = nodes[node->first_child].flags & 0x3FFFFFFF;
	for (uint8_t i = 0; i < 8; i++)
	{
		OctreeNode* sub_node = &nodes[node->first_child + i];
		if (!sub_node->is_full() ||
			(sub_node->flags & 0x3FFFFFFF) != sample_data)
			return { false, 0, {} };

		if (sub_node->first_child != -1)
		{
			auto [check, _, proc_nodes] = check_recursively_equal_childs(sub_node);
			if (sub_node->first_child != -1 && !check)
				return { false, 0, {} };

			checked_nodes.insert(checked_nodes.end(), proc_nodes.begin(), proc_nodes.end());
		}

		checked_nodes.push_back(node->first_child + i);
	}

	return { true, sample_data, checked_nodes };
}

void Octree::remove_nodes(std::vector<uint32_t>& sub_nodes)
{
	// Sort by descending
	std::sort(sub_nodes.begin(), sub_nodes.end(), std::greater<uint32_t>());

	// Remove all nodes
	for (uint32_t& idx : sub_nodes)
	{
		nodes.erase(nodes.begin() + idx);
	}
}

uint16_t Octree::find_node_data(uint16_t x, uint16_t y, uint16_t z, OctreeNode* node)
{
	if (node->bounds_is_zero() || node->is_full())
		return node->flags;

	uint16_t mid_x = (uint16_t) ((node->bottom_corner.x + node->top_corner.x) / 2.0f);
	uint16_t mid_y = (uint16_t) ((node->bottom_corner.y + node->top_corner.y) / 2.0f);
	uint16_t mid_z = (uint16_t) ((node->bottom_corner.z + node->top_corner.z) / 2.0f);

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

	uint16_t mid_x = (uint16_t) ((node->bottom_corner.x + node->top_corner.x) / 2.0f);
	uint16_t mid_y = (uint16_t) ((node->bottom_corner.y + node->top_corner.y) / 2.0f);
	uint16_t mid_z = (uint16_t) ((node->bottom_corner.z + node->top_corner.z) / 2.0f);

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
	glm::vec3 ray_pos = d < 0 ? ray.Origin : ray.Origin + ray.Direction * glm::vec3(d);
	glm::vec3 pos = glm::floor(ray_pos);
	if (d < 0 && !point_in_box(node->bottom_corner, node->top_corner, pos))
		return { nullptr, glm::vec3(), glm::vec3() };

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
					glm::vec3 round_pos = glm::round(ray_pos);
					glm::vec3 localPoint = glm::abs(ray_pos - (round_pos + 0.5f));
					if (std::abs(0.5f - localPoint.x) < std::abs(0.5f - localPoint.y))
						if (std::abs(0.5f - localPoint.x) < std::abs(0.5f - localPoint.z))
							norm = glm::vec3(round_pos.x == (node->top_corner.x + 1) ? 1 : -1, 0, 0);
						else
							norm = glm::vec3(0, 0, round_pos.z == (node->top_corner.z + 1) ? 1 : -1);
					else
						if (std::abs(0.5f - localPoint.y) < std::abs(0.5f - localPoint.z))
							norm = glm::vec3(0, round_pos.y == (node->top_corner.y + 1) ? 1 : -1, 0);
						else
							norm = glm::vec3(0, 0, round_pos.z == (node->top_corner.z + 1) ? 1 : -1);
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
	return min.x <= p.x && p.x <= max.x && min.y <= p.y && p.y <= max.y && min.z <= p.z && p.z <= max.z;
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
