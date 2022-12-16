#pragma once

#include "Octree.h"

struct BVHPrimitiveInfo
{
	uint32_t octree_index;
	glm::vec3 bottom_corner, top_corner, centroid;
};

struct CompareToMid
{
	CompareToMid(int d, float m)
		: dim(d), mid(m) {}

	bool operator()(const BVHPrimitiveInfo& a) const
	{
		return a.centroid[dim] < mid;
	}

	int dim;
	float mid;
};

struct ComparePoints
{
	ComparePoints(int d)
		: dim(d) {}

	bool operator()(const BVHPrimitiveInfo& a, const BVHPrimitiveInfo& b) const
	{
		return a.centroid[dim] < b.centroid[dim];
	}

	int dim;
};

struct BVHBuildNode
{
	BVHBuildNode()
		: bottom_corner(0), top_corner(0), firstPrimOffset(0), nPrimitives(0), splitAxis(0)
	{ childs[0] = childs[1] = nullptr; }

	void InitLeaf(uint32_t first, uint32_t n, const glm::vec3& bottom_cor, const glm::vec3& top_corn)
	{
		firstPrimOffset = first;
		nPrimitives = n;
		bottom_corner = bottom_cor;
		top_corner = top_corn;
	}

	void InitInterior(uint32_t axis, BVHBuildNode* node0, BVHBuildNode* node1)
	{
		childs[0] = node0;
		childs[1] = node1;

		bottom_corner = glm::vec3{
			glm::min(node0->bottom_corner.x, node1->bottom_corner.x),
			glm::min(node0->bottom_corner.y, node1->bottom_corner.y),
			glm::min(node0->bottom_corner.z, node1->bottom_corner.z)
		};

		top_corner = glm::vec3{
			glm::max(node0->top_corner.x, node1->top_corner.x),
			glm::max(node0->top_corner.y, node1->top_corner.y),
			glm::max(node0->top_corner.z, node1->top_corner.z)
		};

		splitAxis = axis;
		nPrimitives = 0;
	}

	glm::vec3 bottom_corner, top_corner;
	BVHBuildNode* childs[2];
	uint32_t firstPrimOffset, nPrimitives;
	uint8_t splitAxis;
};

struct CompareToBucket {
	CompareToBucket(int split, int num, int d, const glm::vec3& min, const glm::vec3& max)
		: min(min), max(max), splitBucket(split), nBuckets(num), dim(d) {}

	bool operator()(const BVHPrimitiveInfo& p) const
	{
		int b = nBuckets * ((p.centroid[dim] - min[dim]) /
			(max[dim] - min[dim]));

		if (b == nBuckets)
			b = nBuckets - 1;

		return b <= splitBucket;
	}
	
	int splitBucket, nBuckets, dim;
	const glm::vec3& min, max;
};

struct LinearBVHNode
{
	glm::vec3 bound_min, bound_max;

	uint32_t primitivesOffset;
	uint32_t firstChildOffset, secondChildOffset;
	
	uint8_t nPrimitives;  // 0 -> interior node
	uint8_t axis;         // interior node: xyz
	uint8_t pad[2];       // ensure 32 byte total size
};

class HLBVH
{
public:
	HLBVH(std::vector<Octree>& primitives, int maxPrimsInNode);

	void Build();

	bool Intersect(const Ray& ray, RayHit* hit) const;

	LinearBVHNode* GetNodeIterator() { return &m_Nodes[0]; }

private:
	BVHBuildNode* build(std::vector<BVHPrimitiveInfo>& primitiveInfo, uint32_t start, uint32_t end, int* totalNodes, std::vector<Octree>& orderedPrims);

	uint32_t flattenBVHTree(BVHBuildNode* node, uint32_t* offset);

	bool findPossibleHits(const Ray& ray, const LinearBVHNode* node, RayHit* hit) const;

private:
	std::vector<Octree>& m_Primitives;
	std::vector<LinearBVHNode> m_Nodes;
	int m_MaxPrimsInNode;
};
