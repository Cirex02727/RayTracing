#include "Renderer.h"

#include "Walnut/Random.h"
#include "Walnut/Timer.h"

#include "utils/Loader.h"
#include "utils/Utils.h"

#include <glm/gtx/string_cast.hpp>

#include <thread>

Renderer::Renderer()
{
	// Walnut::Random::Init();

	m_Octrees = std::vector<Octree>(2);

	// Generate Octree
	{
		Walnut::Timer t;

		// Loader::load_vox("./models/model_128.vox", m_Octree);
		// m_Octrees[0].collapse_nodes();
		// Loader::dump_oct("./models/model_128.oct", m_Octree);

		Loader::load_oct("./models/model_4.oct", m_Octrees[0]);
		// m_Octrees[0].collapse_nodes();
		m_Octrees[0].build_lods();
		m_Octrees[0].calculate_max_depth();


		uint16_t octree_size = 64;
		/*
		m_Octrees[0].init(octree_size);
		for(uint16_t z = 0; z < octree_size; z++)
			for (uint16_t y = 0; y < octree_size; y++)
				for (uint16_t x = 0; x < octree_size; x++)
				{
					float n = Walnut::Random::Float();
					if (n >= 0.9f)
						m_Octrees[0].insert_node(x, y, z, ((x * 255 / octree_size) << 16) | ((y * 255 / octree_size) << 8) | (z * 255 / octree_size));
				}
		*/

		std::cout << "Fill Octree0: " << t.ElapsedMillis() << " ms " << ((m_Octrees[0].m_Nodes.size() * sizeof(OctreeNode)) / 1024.0f / 1024) << "MB" << std::endl;

		/*
		octree_size = 64;
		m_Octrees[1].init(octree_size);
		for (uint16_t z = 0; z < octree_size; z++)
			for (uint16_t y = 0; y < octree_size; y++)
				for (uint16_t x = 0; x < octree_size; x++)
				{
					float n = Walnut::Random::Float();
					if (n >= 0.95f)
						m_Octrees[1].insert_node(x, y, z, ((x * 255 / octree_size) << 16) | ((y * 255 / octree_size) << 8) | (z * 255 / octree_size));
				}
		*/

		Loader::load_oct("./models/model_32.oct", m_Octrees[1]);
		m_Octrees[1].build_lods();
		m_Octrees[1].calculate_max_depth();

		std::cout << "Fill Octree1: " << t.ElapsedMillis() << " ms " << ((m_Octrees[1].m_Nodes.size() * sizeof(OctreeNode)) / 1024.0f / 1024) << "MB" << std::endl;

		m_Octrees[0].m_Position = glm::vec3{ 0, 0, 0 };
		m_Octrees[1].m_Position = glm::vec3{ 16, 0, 16 };


		t.Reset();

		for (Octree& octree : m_Octrees)
		{
			octree.build_lods();
		}

		std::cout << "Build lods: " << m_Octrees.size() << " x " << t.ElapsedMillis() << " ms" << std::endl;
	}

	// Benchmark
	bool benchmark = false;
	if(benchmark)
	{
		Walnut::Timer t;

		float totTime = 0;
		uint32_t step = 200, count = 1'000'000;

		for (uint32_t i = 0; i < step; i++)
		{
			t.Reset();

			// Find Node
			OctreeNode* n;
			short max_depth = 0;
			for (uint32_t i = 0; i < count; i++)
				bool found = m_Octrees[0].find_node(0, 0, 0, n, max_depth);

			totTime += t.ElapsedMillis();
		}
		std::cout << "Find Node x" << step << " | " << totTime << " ms | " << (totTime / step) << " ms each | " << (((totTime / step) / count) * 1'000'000) << " ns each | " << count << std::endl;


		totTime = 0;

		for (uint32_t i = 0; i < step; i++)
		{
			t.Reset();

			// Find Node Data
			for (uint32_t i = 0; i < count; i++)
				uint16_t v = m_Octrees[0].find_node_data(0, 0, 0);

			totTime += t.ElapsedMillis();
		}
		std::cout << "Find Node Data x" << step << " | " << totTime << " ms | " << (totTime / step) << " ms each | " << (((totTime / step) / count) * 1'000'000) << " ns each | " << count << std::endl;


		totTime = 0;

		for (uint32_t i = 0; i < step; i++)
		{
			t.Reset();

			// Ray Travel
			Ray ray{};
			ray.Origin = { -1, 12, -1 };
			ray.Direction = { 0, -0.589469, 0.8077909 };
			ray.InvDirection = glm::vec3(1) / ray.Direction;
			ray.MaxDistance = 100;

			for (uint32_t i = 0; i < count; i++)
				auto [node, pos, norm] = m_Octrees[0].ray_travel(ray);

			totTime += t.ElapsedMillis();
		}

		std::cout << "Ray Travel x" << step << " | " << totTime << " ms | " << (totTime / step) << " ms each | " << (((totTime / step) / count) * 1'000'000) << " ns each | " << count << std::endl;
	}
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		// No resize necessary
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];
}

void Renderer::Render(const Scene& scene, const Camera& camera, bool& render_light, bool& render_normal)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	std::vector<std::thread> threads;
	unsigned char num_threads = 6; // Default 6

	uint32_t stepX = m_FinalImage->GetWidth() / num_threads;
	uint32_t stepY = m_FinalImage->GetHeight() / num_threads;
	
	float totMs = 0;
	for (unsigned char j = 0; j < num_threads; j++)
	{
		for (unsigned char i = 0; i < num_threads; i++)
		{
			threads.emplace_back([this, &stepX, i, &stepY, j, &num_threads, &totMs, &render_light, &render_normal]() {
				Walnut::Timer t;
				
				uint32_t startX = stepX * i, startY = stepY * j;
				uint32_t endX = i == num_threads - 1 ? m_FinalImage->GetWidth() : stepX * (i + 1);
				uint32_t endY = j == num_threads - 1 ? m_FinalImage->GetHeight() : stepY * (j + 1);

				for (uint32_t y = startY; y < endY; y++)
				{
					for (uint32_t x = startX; x < endX; x++)
					{
						//else
						glm::vec4 color = PerPixel(x, y, render_light, render_normal);

						// Crosshair
						if (std::abs(m_FinalImage->GetWidth() / 2.0f - x) < 3 &&
							std::abs(m_FinalImage->GetHeight() / 2.0f - y) < 3)
							color = glm::vec4(1.0, 0.0, 0.0, 1.0);

						color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
						m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::Vec4ToRGBA(color);
					}
				}

				totMs += t.ElapsedMillis();
			});
		}
	}

	for (auto& thread : threads) {
		thread.join();
	}

	// Draw Octrees
	bool is_draw_gizmo = false;

	float octree_time = 0;
	if (is_draw_gizmo)
	{
		Walnut::Timer t;

		for (Octree& octree : m_Octrees)
			DrawOctree(octree, 1);

		octree_time = t.ElapsedMillis();
	}

	std::cout << "Render (" << m_FinalImage->GetWidth() << "x" << m_FinalImage->GetHeight() << ") in "
		<< totMs << "ms (" << (totMs / (num_threads * num_threads)) << " ms each) Gizmo: " << octree_time << std::endl;

	m_FinalImage->SetData(m_ImageData);
}

void Renderer::DrawOctree(Octree& octree, uint8_t depth, uint32_t index)
{
	DrawQuad(octree.m_Nodes[index].bottom_corner.add(octree.m_Position), octree.m_Nodes[index].top_corner.add(octree.m_Position), 0xffff00ff);

	if (depth > 0 && octree.m_Nodes[index].first_child != -1)
		for (uint8_t i = 0; i < 8; i++)
			DrawOctree(octree, depth - 1, octree.m_Nodes[index].first_child + i);
}

void Renderer::DrawQuad(glm::vec3& p0, glm::vec3& p1, uint32_t color)
{
	DrawLine(glm::vec3{ p0.x, p0.y, p0.z }, glm::vec3{ p1.x + 1, p0.y, p0.z }, color);
	DrawLine(glm::vec3{ p0.x, p0.y, p0.z }, glm::vec3{ p0.x, p1.y + 1, p0.z }, color);
	DrawLine(glm::vec3{ p0.x, p0.y, p0.z }, glm::vec3{ p0.x, p0.y, p1.z + 1 }, color);

	DrawLine(glm::vec3{ p0.x, p1.y + 1, p0.z }, glm::vec3{ p1.x + 1, p1.y + 1, p0.z }, color);
	DrawLine(glm::vec3{ p0.x, p1.y + 1, p0.z }, glm::vec3{ p0.x, p1.y + 1, p1.z + 1 }, color);

	DrawLine(glm::vec3{ p1.x + 1, p0.y, p0.z }, glm::vec3{ p1.x + 1, p1.y + 1, p0.z }, color);

	DrawLine(glm::vec3{ p1.x + 1, p1.y + 1, p1.z + 1 }, glm::vec3{ p0.x, p1.y + 1, p1.z + 1 }, color);
	DrawLine(glm::vec3{ p1.x + 1, p1.y + 1, p1.z + 1 }, glm::vec3{ p1.x + 1, p0.y, p1.z + 1 }, color);
	DrawLine(glm::vec3{ p1.x + 1, p1.y + 1, p1.z + 1 }, glm::vec3{ p1.x + 1, p1.y + 1, p0.z }, color);

	DrawLine(glm::vec3{ p1.x + 1, p0.y, p1.z + 1 }, glm::vec3{ p0.x, p0.y, p1.z + 1 }, color);
	DrawLine(glm::vec3{ p1.x + 1, p0.y, p1.z + 1 }, glm::vec3{ p1.x + 1, p0.y, p0.z }, color);

	DrawLine(glm::vec3{ p0.x, p0.y, p1.z + 1 }, glm::vec3{ p0.x, p1.y + 1, p1.z + 1 }, color);
}

void Renderer::DrawLine(glm::vec3& point0, glm::vec3& point1, uint32_t color)
{
	glm::ivec2 p0, p1;
	{
		glm::vec4 clipSpacePos = m_ActiveCamera->GetProjection() * (m_ActiveCamera->GetView() * glm::vec4(point0, 1.0));
		clipSpacePos /= clipSpacePos.w;

		p0 =
		{
			((clipSpacePos.x + 1.0) / 2.0) * m_FinalImage->GetWidth(),
			((clipSpacePos.y + 1.0) / 2.0) * m_FinalImage->GetHeight()
		};

		clipSpacePos = m_ActiveCamera->GetProjection() * (m_ActiveCamera->GetView() * glm::vec4(point1, 1.0));
		clipSpacePos /= clipSpacePos.w;

		p1 =
		{
			((clipSpacePos.x + 1.0) / 2.0) * m_FinalImage->GetWidth(),
			((clipSpacePos.y + 1.0) / 2.0) * m_FinalImage->GetHeight()
		};
	}

	glm::ivec2 d = glm::abs(p1 - p0);
	d.y *= -1;

	glm::ivec2 s = { (p0.x < p1.x ? 1 : -1), (p0.y < p1.y ? 1 : -1) };
	int error = d.x + d.y, e2 = 0;

	while (true)
	{
		DrawPoint(p0, color);
		if (p0.x == p1.x && p0.y == p1.y)
			break;

		e2 = 2.0 * error;
		if (e2 > d.y)
		{
			error += d.y;
			p0.x += s.x;
		}
		if (e2 < d.x)
		{
			error += d.x;
			p0.y += s.y;
		}
	}
}

void Renderer::DrawPoint(glm::ivec2& p, uint32_t color, uint8_t thickness)
{
	if (p.x - thickness < 0 || p.x + thickness >= m_FinalImage->GetWidth() ||
		p.y - thickness < 0 || p.y + thickness >= m_FinalImage->GetHeight())
		return;

	for (int y = p.y - thickness; y < p.y + thickness; y++)
		for (int x = p.x - thickness; x < p.x + thickness; x++)
			m_ImageData[x + y * m_FinalImage->GetWidth()] = color;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y, bool& render_light, bool& render_normal)
{
	Ray ray{};
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];
	ray.InvDirection = m_ActiveCamera->GetInvDirections()[x + y * m_FinalImage->GetWidth()];
	ray.MaxDistance = 500;

	glm::vec3 color(0.0f);

	float multiplier = 1.0f;

	// DEBUG: Is on crosshair
	if (std::abs(m_FinalImage->GetWidth() / 2.0f - x) < 1 &&
		std::abs(m_FinalImage->GetHeight() / 2.0f - y) < 1)
	{
		int a = 0;
	}

	int bounces = 1;
	for (int i = bounces; i >= 0; i--)
	{
		// Renderer::HitPaylod paylod = TraceRay(ray);
		Renderer::HitPaylod paylod{};
		if (!VoxelTraceRay(ray, paylod))
		{
			glm::vec3 skyColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			color += skyColor * multiplier;
			break;
		}

		glm::vec3 lightDir = glm::normalize(ray.Direction);
		float lightIntensity = glm::max(glm::dot(paylod.WorldNormal, -lightDir), 0.0f); // == cos(angle)
		
		// const Sphere& sphere = m_ActiveScene->Spheres[paylod.ObjectIndex];
		// glm::vec3 sphereColor = sphere.Albedo;
		// sphereColor *= lightIntensity;
		// color += sphereColor * multiplier;

		if (paylod.OctreeNode)
			if(render_normal)
				color = (paylod.WorldNormal + 1.0f) / 2.0f;
			else
				color = paylod.OctreeNode->get_color() * (render_light ? lightIntensity : 1.0f);
		else
			color = glm::vec3(1, 0, 1) * lightIntensity;

		if (i == 1)
			break;

		multiplier *= 0.7f;

		ray.Origin = paylod.WorldPosition + paylod.WorldNormal * 0.0001f;
		ray.Direction = glm::reflect(ray.Direction, paylod.WorldNormal);
	}

	return glm::vec4(color, 1.0f);
}

bool Renderer::VoxelTraceRay(Ray& ray, HitPaylod& paylod)
{
	// Octree ray trace
	int octree_index = 0;
	if (Octree::ray_intersect_box(
		m_Octrees[1].m_Nodes[0].bottom_corner.add(m_Octrees[1].m_Position),
		m_Octrees[1].m_Nodes[0].top_corner.add(m_Octrees[1].m_Position), ray) != -1)
		octree_index = 1;

	ray.Origin -= m_Octrees[octree_index].m_Position;
	auto [node, pos, norm] = m_Octrees[octree_index].ray_travel(ray);
	if (node != nullptr)
	{
		paylod.WorldPosition = pos;
		paylod.OctreeNode = node;
		paylod.WorldNormal = norm;
		return true;
	}
	else
		return Miss(ray, paylod);
}

bool Renderer::TraceRay(const Ray& ray, HitPaylod& paylod)
{
	// (bx^2 + by^2)t^2 + (2(zxbx + ayby))t + (ax^2 + ay^2 - r^2) = 0
	// where
	// a = ray origin
	// b = ray diraction
	// r = radius
	// t = hit distance

	int closestSphere = -1;
	float hitDistance = std::numeric_limits<float>::max();
	for (size_t i = 0; i < m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		// Quadratic formula discriminant:
		// b^2 - 4ac

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f)
			continue;

		// Quadratic formula:
		// (-b +- sqrt(discriminant)) / 2a

		//float t0 = (-b + glm::sqrt(discriminant)) / (2.0f * a); // Second hit distance (currently unused)
		float closestT = (-b - glm::sqrt(discriminant)) / (2.0f * a);
		if (closestT > 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphere = (int)i;
		}
	}

	if (closestSphere < 0)
		return Miss(ray, paylod);

	return ClosestHit(ray, hitDistance, closestSphere, paylod);
}

bool Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex, HitPaylod& paylod)
{
	//paylod.HitDistance = hitDistance;
	paylod.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	paylod.WorldPosition = origin + ray.Direction * hitDistance;
	paylod.WorldNormal = glm::normalize(paylod.WorldPosition);

	paylod.WorldPosition += closestSphere.Position;

	return true;
}

bool Renderer::Miss(const Ray& ray, HitPaylod& paylod)
{
	//paylod.HitDistance = -1.0f;
	return false;
}
