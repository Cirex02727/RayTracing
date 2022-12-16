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

void Utils::OctreeIdxRemap(uint8_t& pos, OctreeNode* node)
{
	uint8_t child_format = node->child_format();
	if (child_format == 2)
	{
		if (node->bottom_corner.x == node->top_corner.x && pos > 3)
		{
			pos -= 2;
		}
		else if (node->bottom_corner.y == node->top_corner.y)
		{
			pos -= 4;
		}
		else if (node->bottom_corner.z == node->top_corner.z)
		{
			pos /= 2;
		}
	}
	else if (child_format == 3)
	{
		if (node->bottom_corner.x != node->top_corner.x)
		{
			pos = (pos - 4) / 2;
		}
		else if (node->bottom_corner.y != node->top_corner.y)
		{
			pos /= 4;
		}
		else if (node->bottom_corner.z != node->top_corner.z)
		{
			pos -= 4;
		}
	}
}

void Utils::UnionAABBs(glm::vec3& source_min, glm::vec3& source_max, const glm::vec3& other_min, const glm::vec3& other_max)
{
	source_min.x = glm::min(source_min.x, other_min.x);
	source_min.y = glm::min(source_min.y, other_min.y);
	source_min.z = glm::min(source_min.z, other_min.z);
	source_max.x = glm::max(source_max.x, other_max.x);
	source_max.y = glm::max(source_max.y, other_max.y);
	source_max.z = glm::max(source_max.z, other_max.z);
}

void Utils::UnionAABBs(glm::vec3& source_min, glm::vec3& source_max, const glm::vec3& other)
{
	source_min.x = glm::min(source_min.x, other.x);
	source_min.y = glm::min(source_min.y, other.y);
	source_min.z = glm::min(source_min.z, other.z);
	source_max.x = glm::max(source_max.x, other.x);
	source_max.y = glm::max(source_max.y, other.y);
	source_max.z = glm::max(source_max.z, other.z);
}

uint8_t Utils::MaxExtension(const glm::vec3& min, const glm::vec3& max)
{
	if (max.x - min.x > max.y - min.y)
		if (max.x - min.x > max.z - min.z) return 0;
		else                               return 2;
	else
		if (max.y - min.y > max.z - min.z) return 1;
		else                               return 2;
}

float Utils::SurfaceArea(const glm::vec3& min, const glm::vec3& max)
{
	glm::vec3 d = max - min;
	return 2.f * (d.x * d.y + d.x * d.z + d.y * d.z);
}
