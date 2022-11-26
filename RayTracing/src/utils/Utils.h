#pragma once

#include <glm/glm.hpp>

class Utils
{
public:
	static uint32_t Vec4ToRGBA(const glm::vec4& color);
	
	static glm::vec3 IntToVec3Color(int color);
	
	static glm::vec3 Lighting(const glm::vec3& norm, const glm::vec3& pos, const glm::vec3& rd, const glm::vec3& col);

	static short GetLOD(float distance, uint16_t side_len, uint8_t max_depth);
};
