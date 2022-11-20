#include "Renderer.h"

#include "Walnut/Random.h"
#include "Walnut/Timer.h"

#include "utils/Loader.h"

#include <thread>
#include <glm/gtx/string_cast.hpp>

namespace Utils
{
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}

	static glm::vec3 lighting(const glm::vec3& norm, const glm::vec3& pos, const glm::vec3& rd, const glm::vec3& col) {
		glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0, 3.0, -1.0));
		float diffuseAttn = std::max(glm::dot(norm, lightDir), 0.0f);
		glm::vec3 light = glm::vec3(1.0, 0.9, 0.9);

		glm::vec3 ambient = glm::vec3(0.2, 0.2, 0.3);

		glm::vec3 reflected = reflect(rd, norm);
		float specularAttn = std::max(glm::dot(reflected, lightDir), 0.0f);

		return col * (diffuseAttn * light * glm::vec3(1.0f) + specularAttn * light * glm::vec3(0.6f) + ambient);
	}

	static glm::vec3 IntToVec3Color(int color)
	{
		return glm::vec3(
			color & 0xff,
			(color >> 8) & 0xff,
			(color >> 16) & 0xff
		);
	}
}

Renderer::Renderer()
{
	Walnut::Random::Init();

	// Loader::load_vox("./models/model_128.vox", m_Octree);

	// Loader::dump_oct("./models/model_128.oct", m_Octree);

	// m_Octree.nodes.clear();

	Loader::load_oct("./models/model_128.oct", m_Octree);

	// Generate Octree
	{
		Walnut::Timer t;
		
		// Insert Data
		/*
		uint16_t octree_size = 256;
		for (uint16_t z = 0; z < octree_size; z++)
			for (uint16_t y = 0; y < octree_size; y++)
				for (uint16_t x = 0; x < octree_size; x++)
				{
					float n = Walnut::Random::Float();
					if (n >= 0.9f)
						m_Octree.insert_node(x, y, z, ((x * 255 / octree_size) << 16) | ((y * 255 / octree_size) << 8) | (z * 255 / octree_size));
				}
		*/

		/*
		for (uint16_t z = 0; z < 16; z++)
			for (uint16_t y = 0; y < 16; y++)
				for (uint16_t x = 0; x < 16; x++)
					if(x == y && y == z)
						m_Octree.insert_node(x, y, z, (x << 16) | (y << 8) | z);
		*/

		// m_Octree.insert_node(0, 256, 0, 0xff0000);
		// m_Octree.insert_node(511, 511, 0, 0x00ff00);
		// m_Octree.insert_range_node(u_shortV3{ 0 }, u_shortV3{ 511, 255, 511 }, 0x0000ff);

		std::cout << "Fill Octree: " << t.ElapsedMillis() << " ms " << ((m_Octree.nodes.size() * sizeof(OctreeNode)) / 1024.0f / 1024) << "MB" << std::endl;
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
			for (uint32_t i = 0; i < count; i++)
				OctreeNode* n = m_Octree.find_node(0, 0, 0);

			totTime += t.ElapsedMillis();
		}
		std::cout << "Find Node x" << step << " | " << totTime << " ms | " << (totTime / step) << " ms each | " << (((totTime / step) / count) * 1'000'000) << " ns each | " << count << std::endl;


		totTime = 0;

		for (uint32_t i = 0; i < step; i++)
		{
			t.Reset();

			// Find Node Data
			for (uint32_t i = 0; i < count; i++)
				uint16_t v = m_Octree.find_node_data(0, 0, 0);

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
				auto [node, pos, norm] = m_Octree.ray_travel(ray);

			totTime += t.ElapsedMillis();
		}

		std::cout << "Ray Travel x" << step << " | " << totTime << " ms | " << (totTime / step) << " ms each | " << (((totTime / step) / count) * 1'000'000) << " ns each | " << count << std::endl;
	}

	/*
	* General Benchmark: i5 10600K
	* 
	* Find Node      x200.000 |  4956.89 ms |    0.02478450 ms each | 24.78450 ns each |       1.000
	* Find Node Data x200.000 |  4997.88 ms |    0.02498940 ms each | 24.98940 ns each |       1.000
	* Ray Travel     x200.000 |  1681.49 ms |    0.00840746 ms each |  8.40746 ns each |       1.000
	* 
	* Find Node          x200 |  5037.47 ms |   25.18730000 ms each | 25.18730 ns each |   1.000.000
	* Find Node Data     x200 |  5039.10 ms |   25.19550000 ms each | 25.19550 ns each |   1.000.000
	* Ray Travel         x200 |  1663.51 ms |    8.31756000 ms each |  8.31756 ns each |   1.000.000
	* 
	* Find Node            x5 | 12191.30 ms | 2438.25000000 ms each | 24.38250 ns each | 100.000.000
	* Find Node Data       x5 | 12412.80 ms | 2482.56000000 ms each | 24.82560 ns each | 100.000.000
	* Ray Travel           x5 |  4163.80 ms |  832.75900000 ms each |  8.32759 ns each | 100.000.000
	* 
	* 
	* General Info:
	* 
	* Viewport Size | 1600 x 874
	*/
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
						glm::vec4 color = PerPixel(x, y, render_light, render_normal);

						// Crosshair
						if (std::abs(m_FinalImage->GetWidth() / 2.0f - x) < 3 &&
							std::abs(m_FinalImage->GetHeight() / 2.0f - y) < 3)
							color = glm::vec4(1.0, 0.0, 0.0, 1.0);
						
						color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
						m_ImageData[x + y * m_FinalImage->GetWidth()] = Utils::ConvertToRGBA(color);
					}
				}

				totMs += t.ElapsedMillis();
			});
		}
	}

	for (auto& thread : threads) {
		thread.join();
	}

	std::cout << "Render (" << m_FinalImage->GetWidth() << "x" << m_FinalImage->GetHeight() << ") in "
		<< totMs << "ms (" << (totMs / (num_threads * num_threads)) << " ms each)" << std::endl;

	m_FinalImage->SetData(m_ImageData);
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

	bool is_crosshair = false;
	if (std::abs(m_FinalImage->GetWidth() / 2.0f - x) < 1 &&
		std::abs(m_FinalImage->GetHeight() / 2.0f - y) < 1)
	{
		is_crosshair = true;
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

		// if (is_crosshair)
		// 	std::cout << "Normal Crosshair: " << glm::to_string(paylod.WorldNormal) << std::endl;

		if (i == 1)
			break;

		multiplier *= 0.7f;

		ray.Origin = paylod.WorldPosition + paylod.WorldNormal * 0.0001f;
		ray.Direction = glm::reflect(ray.Direction, paylod.WorldNormal);
	}

	return glm::vec4(color, 1.0f);
}

bool Renderer::VoxelTraceRay(const Ray& ray, HitPaylod& paylod)
{
	/*
	if (Octree::ray_intersect_box(u_shortV3{5, 0, 14}, u_shortV3{6, 1, 15}, ray) != -1)
	{
		paylod.WorldPosition = glm::vec3(5, 0, 14);
		paylod.OctreeNode = nullptr;
		paylod.WorldNormal = glm::vec3();
		return true;
	}
	*/

	// Octree ray trace
	auto [node, pos, norm] = m_Octree.ray_travel(ray);
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
	paylod.OctreeNode = nullptr;
	//paylod.HitDistance = -1.0f;
	return false;
}
