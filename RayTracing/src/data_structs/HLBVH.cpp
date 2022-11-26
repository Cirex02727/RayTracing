#include "HLBVH.h"

HLBVH::HLBVH(std::vector<Octree>& primitives, int maxPrimsInNode)
	: m_MaxPrimsInNode(std::min(255, maxPrimsInNode)), m_Primitives(primitives)
{
	if (primitives.size() == 0)
		return;

	// Build BVH From Primitives
	std::vector<BVHPrimitiveInfo> primitiveInfo(primitives.size());
	for (uint32_t i = 0; i < primitives.size(); i++)
		primitiveInfo[i] = { i, &primitives[i].m_Nodes[0] };

	int totalNodes = 0;
	uint32_t root_index = Build(primitiveInfo, &totalNodes, primitives);
}

uint32_t HLBVH::Build(const std::vector<BVHPrimitiveInfo>& primitiveInfo, int* totalNodes, std::vector<Octree>& orderedPrims)
{
	glm::vec3 bounds;
	// for (const BVHPrimitiveInfo& info : primitiveInfo)
	// 	bounds = Union(bounds, info.GetCentroid());

	return 0;
}
