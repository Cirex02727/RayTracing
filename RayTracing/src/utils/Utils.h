#pragma once

#include "../Renderer.h"

#include <glm/glm.hpp>

class Utils
{
public:
	static uint32_t Vec4ToRGBA(const glm::vec4& color);
	
	static glm::vec3 IntToVec3Color(int color);
	
	static glm::vec3 Lighting(const glm::vec3& norm, const glm::vec3& pos, const glm::vec3& rd, const glm::vec3& col);

	static short GetLOD(float distance, uint16_t side_len, uint8_t max_depth);

	static void OctreeIdxRemap(uint8_t& pos, OctreeNode* node);

	static void UnionAABBs(glm::vec3& source_min, glm::vec3& source_max, const glm::vec3& other_min, const glm::vec3& other_max);

	static void UnionAABBs(glm::vec3& source_min, glm::vec3& source_max, const glm::vec3& other);

	static uint8_t MaxExtension(const glm::vec3& min, const glm::vec3& max);

	static float Utils::SurfaceArea(const glm::vec3& min, const glm::vec3& max);
};
