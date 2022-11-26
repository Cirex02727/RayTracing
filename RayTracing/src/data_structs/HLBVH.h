#pragma once

#include "Octree.h"

struct BVHPrimitiveInfo
{
	uint32_t octree_index;
	const OctreeNode* root;
};

class HLBVH
{
public:
	HLBVH(std::vector<Octree>& primitives, int maxPrimsInNode);

private:
	uint32_t Build(const std::vector<BVHPrimitiveInfo>& primitiveInfo, int* totalNodes, std::vector<Octree>& orderedPrims);

private:
	std::vector<Octree>& m_Primitives;
	int m_MaxPrimsInNode;
};
