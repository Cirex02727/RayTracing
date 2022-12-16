#pragma once

#include <glm/glm.hpp>

struct Ray
{
	glm::vec3 Origin;
	glm::vec3 Direction;
	glm::vec3 InvDirection;

	double MaxDistance;
};

struct OctreeNode;

struct RayHit
{
	OctreeNode* Node;
	glm::vec3 Position;
	glm::vec3 Normal;
};
