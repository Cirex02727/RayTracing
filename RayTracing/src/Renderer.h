#pragma once

#include "Walnut/Image.h"

#include "Camera.h"
#include "Ray.h"
#include "Scene.h"

#include "octree/Octree.h"

#include <memory>
#include <glm/glm.hpp>

class Renderer
{
#define INDEX(x, y, z) x + (y * m_VoxelSize) + (z * m_VoxelSize * m_VoxelSize)

public:
	Renderer();

	void OnResize(uint32_t width, uint32_t height);
	void Render(const Scene& scene, const Camera& camera);
	
	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

private:
	struct HitPaylod
	{
		OctreeNode* OctreeNode;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal;

		int ObjectIndex;
	};

	glm::vec4 PerPixel(uint32_t x, uint32_t y); // RayGen

	bool VoxelTraceRay(const Ray& ray, HitPaylod& paylod);

	bool TraceRay(const Ray& ray, HitPaylod& paylod);
	bool ClosestHit(const Ray& ray, float hitDistance, int objectIndex, HitPaylod& paylod);
	bool Miss(const Ray& ray, HitPaylod& paylod);

private:
	std::shared_ptr<Walnut::Image> m_FinalImage;

	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;

	uint32_t* m_ImageData = nullptr;

	Octree m_Octree;
};
