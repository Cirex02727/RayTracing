#pragma once

#include <vulkan/vulkan_core.h>

class VulkanUtils
{
public:
	static void Init();

	static VkResult CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer& buffer, VkDeviceMemory& memory, void* data);

	static VkResult UpdateBuffer(VkDeviceMemory& memory, VkDeviceSize size, void* data);

	static uint32_t GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits);

private:
	static VkDevice m_Device;
};
