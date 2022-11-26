#pragma once

#include "Walnut/Image.h"

#include "Camera.h"
#include "Ray.h"
#include "Scene.h"

#include "data_structs/Octree.h"

#include <memory>
#include <glm/glm.hpp>

class Renderer
{
#define INDEX(x, y, z) x + (y * m_VoxelSize) + (z * m_VoxelSize * m_VoxelSize)

public:
	Renderer();

	void OnResize(uint32_t width, uint32_t height);
	void Render(const Scene& scene, const Camera& camera, bool& render_light, bool& render_normal);
	
	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

private:
	struct HitPaylod
	{
		OctreeNode* OctreeNode;
		glm::vec3 WorldPosition;
		glm::vec3 WorldNormal;

		int ObjectIndex;
	};

	void DrawOctree(Octree& octree, uint8_t depth = 3, uint32_t index = 0);

	void DrawQuad(glm::vec3& p0, glm::vec3& p1, uint32_t color);

	void DrawLine(glm::vec3& point0, glm::vec3& point1, uint32_t color);

	void DrawPoint(glm::ivec2& p, uint32_t color = 0xff0000ff, uint8_t thickness = 1);

	glm::vec4 PerPixel(uint32_t x, uint32_t y, bool& render_light, bool& render_normal); // RayGen

	bool VoxelTraceRay(Ray& ray, HitPaylod& paylod);

	bool TraceRay(const Ray& ray, HitPaylod& paylod);
	bool ClosestHit(const Ray& ray, float hitDistance, int objectIndex, HitPaylod& paylod);
	bool Miss(const Ray& ray, HitPaylod& paylod);

private:
	std::shared_ptr<Walnut::Image> m_FinalImage;

	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;

	uint32_t* m_ImageData = nullptr;

public:
	std::vector<Octree> m_Octrees;
};
