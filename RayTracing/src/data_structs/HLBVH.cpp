#include "HLBVH.h"

#include "../utils/Utils.h"

#include "Walnut/Timer.h"

#include <iostream>

#include <glm/ext/matrix_transform.hpp>

HLBVH::HLBVH(std::vector<Octree>& primitives, int maxPrimsInNode)
	: m_MaxPrimsInNode(std::min(255, maxPrimsInNode)), m_Primitives(primitives), m_Nodes() {}

void HLBVH::Build()
{
	if (m_Primitives.size() == 0)
		return;

	Walnut::Timer t;

	// Initialize _primitiveInfo_ array for primitives
	std::vector<BVHPrimitiveInfo> primitiveInfo(m_Primitives.size());
	for (uint32_t i = 0; i < m_Primitives.size(); i++)
	{
		primitiveInfo[i] = { i, m_Primitives[i].m_Nodes[0].bottom_corner + m_Primitives[i].m_Position,
								m_Primitives[i].m_Nodes[0].top_corner    + m_Primitives[i].m_Position };

		primitiveInfo[i].centroid = { (primitiveInfo[i].bottom_corner.x + primitiveInfo[i].top_corner.x) / 2,
									  (primitiveInfo[i].bottom_corner.y + primitiveInfo[i].top_corner.y) / 2,
									  (primitiveInfo[i].bottom_corner.z + primitiveInfo[i].top_corner.z) / 2 };
	}

	// Recursively build BVH tree for primitives
	int totalNodes = 0;
	std::vector<Octree> orderedPrims;
	orderedPrims.reserve(m_Primitives.size());

	BVHBuildNode* root = build(primitiveInfo, 0, primitiveInfo.size(), &totalNodes, orderedPrims);

	m_Primitives = move(orderedPrims);
	std::cout << "BVH created with " << totalNodes << " nodes for " << (int)m_Primitives.size() << " primitives (" << float(totalNodes * sizeof(LinearBVHNode)) / (1024.f * 1024.f) << " MB) in " << t.ElapsedMillis() << "ms" << std::endl;

	m_Nodes = std::vector<LinearBVHNode>(totalNodes);

	uint32_t offset = 0;
	flattenBVHTree(root, &offset);
}

bool HLBVH::Intersect(const Ray& ray, RayHit* hit) const
{
	if (m_Nodes.size() <= 0 || Octree::ray_intersect_box(m_Nodes[0].bound_min, m_Nodes[0].bound_max, ray) == -1)
		return false;

	return findPossibleHits(ray, &m_Nodes[0], hit);
}

BVHBuildNode* HLBVH::build(std::vector<BVHPrimitiveInfo>& primitiveInfo, uint32_t start, uint32_t end, int* totalNodes, std::vector<Octree>& orderedPrims)
{
	(*totalNodes)++;
	
	BVHBuildNode* node = new BVHBuildNode();

	// Compute bounds of all primitives in BVH node
	glm::vec3 bounds_min = primitiveInfo[start].bottom_corner, bounds_max = primitiveInfo[start].top_corner;
	for (uint32_t i = start + 1; i < end; i++)
		Utils::UnionAABBs(bounds_min, bounds_max, primitiveInfo[i].bottom_corner, primitiveInfo[i].top_corner);

	uint32_t nPrimitives = end - start;
	if (nPrimitives == 1)
	{
		// Create leaf _BVHBuildNode_
		uint32_t firstPrimOffset = orderedPrims.size();
		for (uint32_t i = start; i < end; i++) {
			orderedPrims.push_back(m_Primitives[primitiveInfo[i].octree_index]);
		}
		
		node->InitLeaf(firstPrimOffset, nPrimitives, bounds_min, bounds_max);
		return node;
	}

	// Compute bound of primitive centroids, choose split dimension _dim_
	glm::vec3 centroid_min, centroid_max;
	centroid_min = centroid_max = primitiveInfo[start].centroid;

	for (uint32_t i = start + 1; i < end; i++)
		Utils::UnionAABBs(centroid_min, centroid_max, primitiveInfo[i].centroid);
	int dim = Utils::MaxExtension(centroid_min, centroid_max);
	
	// Partition primitives into two sets and build children
	uint32_t mid = (start + end) / 2;
	if (centroid_max[dim] == centroid_min[dim])
	{
		// Create leaf _BVHBuildNode_
		uint32_t firstPrimOffset = orderedPrims.size();
		for (uint32_t i = start; i < end; i++) 
			orderedPrims.push_back(m_Primitives[primitiveInfo[i].octree_index]);

		node->InitLeaf(firstPrimOffset, nPrimitives, bounds_min, bounds_max);
		return node;
	}

	// Partition primitives using approximate SAH
	if (nPrimitives <= m_MaxPrimsInNode)
	{
		// Partition primitives into equally-sized subsets
		mid = (start + end) / 2;
		std::nth_element(&primitiveInfo[start], &primitiveInfo[mid],
			&primitiveInfo[end - 1] + 1, ComparePoints(dim));

		node->InitInterior(dim,
			build(primitiveInfo, start, mid,
				totalNodes, orderedPrims),
			build(primitiveInfo, mid, end,
				totalNodes, orderedPrims));

		return node;
	}

	// Allocate _BucketInfo_ for SAH partition buckets
	const int nBuckets = 12;
	struct BucketInfo {
		BucketInfo()
			: bounds_min(0), bounds_max(0), count(0) {}

		int count;
		glm::vec3 bounds_min, bounds_max;
	};
	BucketInfo buckets[nBuckets];

	// Initialize _BucketInfo_ for SAH partition buckets
	for (uint32_t i = start; i < end; ++i)
	{
		int b = nBuckets *
			((primitiveInfo[i].centroid[dim] - centroid_min[dim]) /
				(centroid_max[dim] - centroid_min[dim]));

		if (b == nBuckets)
			b = nBuckets - 1;

		buckets[b].count++;
		Utils::UnionAABBs(buckets[b].bounds_min, buckets[b].bounds_max, primitiveInfo[i].bottom_corner, primitiveInfo[i].top_corner);
	}

	// Compute costs for splitting after each bucket
	float cost[nBuckets - 1];
	for (int i = 0; i < nBuckets - 1; ++i)
	{
		glm::vec3 min0, max0, min1, max1;
		int count0 = 0, count1 = 0;
		for (int j = 0; j <= i; ++j)
		{
			Utils::UnionAABBs(min0, max0, buckets[j].bounds_min, buckets[j].bounds_max);
			count0 += buckets[j].count;
		}

		for (int j = i + 1; j < nBuckets; ++j)
		{
			Utils::UnionAABBs(min1, max1, buckets[j].bounds_min, buckets[j].bounds_max);
			count1 += buckets[j].count;
		}

		cost[i] = .125f + (count0 * Utils::SurfaceArea(min0, max0) + count1 * Utils::SurfaceArea(min1, max1)) /
			Utils::SurfaceArea(bounds_min, bounds_max);
	}

	// Find bucket to split at that minimizes SAH metric
	float minCost = cost[0];
	uint32_t minCostSplit = 0;
	for (int i = 1; i < nBuckets - 1; ++i)
	{
		if (cost[i] < minCost)
		{
			minCost = cost[i];
			minCostSplit = i;
		}
	}

	// Either create leaf or split primitives at selected SAH bucket
	if (nPrimitives > m_MaxPrimsInNode ||
		minCost < nPrimitives)
	{
		BVHPrimitiveInfo* pmid = std::partition(&primitiveInfo[start],
			&primitiveInfo[end - 1] + 1,
			CompareToBucket(minCostSplit, nBuckets, dim, centroid_min, centroid_max));
		mid = pmid - &primitiveInfo[0];
	}
	else
	{
		// Create leaf _BVHBuildNode_
		uint32_t firstPrimOffset = orderedPrims.size();
		for (uint32_t i = start; i < end; ++i) {
			uint32_t primNum = primitiveInfo[i].octree_index;
			orderedPrims.push_back(m_Primitives[primNum]);
		}
		node->InitLeaf(firstPrimOffset, nPrimitives, bounds_min, bounds_max);
	}
	
	node->InitInterior(dim,
		build(primitiveInfo, start, mid,
		totalNodes, orderedPrims),
		build(primitiveInfo, mid, end,
		totalNodes, orderedPrims));

	return node;
}

uint32_t HLBVH::flattenBVHTree(BVHBuildNode* node, uint32_t* offset)
{
	LinearBVHNode* linearNode = &m_Nodes[*offset];
	linearNode->bound_min = node->bottom_corner;
	linearNode->bound_max = node->top_corner;
	uint32_t myOffset = (*offset)++;

	if (node->nPrimitives > 0)
	{
		linearNode->primitivesOffset = node->firstPrimOffset;
		linearNode->nPrimitives = node->nPrimitives;
	}
	else
	{
		// Creater interior flattened BVH node
		linearNode->axis = node->splitAxis;
		linearNode->nPrimitives = 0;
		linearNode->firstChildOffset = flattenBVHTree(node->childs[0], offset);
		linearNode->secondChildOffset = flattenBVHTree(node->childs[1], offset);
	}

	delete node;
	return myOffset;
}

bool HLBVH::findPossibleHits(const Ray& ray, const LinearBVHNode* node, RayHit* hit) const
{
	// Process BVH node _node_ for traversal
	if (node->nPrimitives > 0)
	{
		Octree* octree = &m_Primitives[node->primitivesOffset];

		Ray localRay = ray;
		localRay.Origin -= octree->m_Position;

		return octree->ray_travel(localRay, hit);
	}

	float d0 = Octree::ray_intersect_box(m_Nodes[node->firstChildOffset].bound_min, m_Nodes[node->firstChildOffset].bound_max, ray);
	float d1 = Octree::ray_intersect_box(m_Nodes[node->secondChildOffset].bound_min, m_Nodes[node->secondChildOffset].bound_max, ray);
	if (d0 != -1 && d1 != -1)
	{
		if (d0 < d1)
		{
			if (findPossibleHits(ray, &m_Nodes[node->firstChildOffset], hit))
				return true;

			return findPossibleHits(ray, &m_Nodes[node->secondChildOffset], hit);
		}

		if (findPossibleHits(ray, &m_Nodes[node->secondChildOffset], hit))
			return true;

		return findPossibleHits(ray, &m_Nodes[node->firstChildOffset], hit);
	}
	else if(d0 != -1)
		return findPossibleHits(ray, &m_Nodes[node->firstChildOffset], hit);
	else if(d1 != -1)
		return findPossibleHits(ray, &m_Nodes[node->secondChildOffset], hit);

	return false;
}
