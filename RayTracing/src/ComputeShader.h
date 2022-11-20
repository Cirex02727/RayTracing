#pragma once

#include "Walnut/Application.h"
#include "Walnut/Image.h"

#include <glm/glm.hpp>

#include <memory>

class ComputeShader
{
public:
	ComputeShader(uint32_t width, uint32_t height);
	~ComputeShader();

	void Render();

	void UpdateBuffers(glm::vec3& color);

	std::shared_ptr<Walnut::Image> GetOutImage() const { return m_OutImage; }

private:
	void prepareStorageBuffers();
	void setupDescriptorSetLayout();
	void preparePipelines();
	void setupDescriptorPool();
	void setupDescriptorSet();
	void prepareCompute();
	void buildComputeCommandBuffer();

	inline static uint32_t GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
	{
		VkPhysicalDeviceMemoryProperties prop;
		vkGetPhysicalDeviceMemoryProperties(Walnut::Application::GetPhysicalDevice(), &prop);
		for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
		{
			if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
				return i;
		}

		return 0xffffffff;
	}

private:
	std::shared_ptr<Walnut::Image> m_OutImage;

	VkDevice Device;

	VkBuffer InBuffer;
	VkDeviceMemory InBufferMemory;
	size_t InAlignedSize;

	VkDescriptorSetLayout DescriptorSetLayout;
	VkPipelineLayout PipelineLayout;

	VkPipeline Pipeline;

	VkDescriptorPool DescriptorPool;
	VkDescriptorSet DescriptorSet;

	VkCommandBuffer CommandBuffer;
	uint32_t QueueFamily;
	VkCommandPool CommandPool;
	VkQueue Queue;
	VkFence Fence;
};
