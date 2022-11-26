#include "Utils.h"

#include <utility>

uint32_t Utils::Vec4ToRGBA(const glm::vec4& color)
{
	uint8_t r = (uint8_t)(color.r * 255.0f);
	uint8_t g = (uint8_t)(color.g * 255.0f);
	uint8_t b = (uint8_t)(color.b * 255.0f);
	uint8_t a = (uint8_t)(color.a * 255.0f);

	uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
	return result;
}

glm::vec3 Utils::IntToVec3Color(int color)
{
	return glm::vec3(
		color & 0xff,
		(color >> 8) & 0xff,
		(color >> 16) & 0xff
	);
}

glm::vec3 Utils::Lighting(const glm::vec3& norm, const glm::vec3& pos, const glm::vec3& rd, const glm::vec3& col)
{
	glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0, 3.0, -1.0));
	float diffuseAttn = std::max(glm::dot(norm, lightDir), 0.0f);
	glm::vec3 light = glm::vec3(1.0, 0.9, 0.9);

	glm::vec3 ambient = glm::vec3(0.2, 0.2, 0.3);

	glm::vec3 reflected = reflect(rd, norm);
	float specularAttn = std::max(glm::dot(reflected, lightDir), 0.0f);

	return col * (diffuseAttn * light * glm::vec3(1.0f) + specularAttn * light * glm::vec3(0.6f) + ambient);
}

short Utils::GetLOD(float distance, uint16_t side_len, uint8_t max_depth)
{
	// Min Distance
	if (distance < side_len + 50) return -1;

	// Proportion to Max Distance
	return max_depth - std::min((uint8_t) std::round((distance - (side_len + 50)) * (max_depth - 1) / (450 - (side_len + 50))), (uint8_t) (max_depth - 1)) - 1;
}
