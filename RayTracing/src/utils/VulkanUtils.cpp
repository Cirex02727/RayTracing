#include "VulkanUtils.h"

#include "Walnut/Application.h"

VkDevice VulkanUtils::m_Device = {};

void VulkanUtils::Init()
{
	m_Device = Walnut::Application::GetDevice();
}

VkResult VulkanUtils::CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer& buffer, VkDeviceMemory& memory, void* data)
{
	VkResult result;

	// Create the buffer handle
	VkBufferCreateInfo bufferCreateInfo = {};
	{
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.size = size;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	result = vkCreateBuffer(m_Device, &bufferCreateInfo, nullptr, &buffer);
	check_vk_result(result);

	// Create the memory backing up the buffer handle
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(m_Device, buffer, &memReqs);

	VkMemoryAllocateInfo memAlloc = {};
	{
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = memReqs.size;
		// Find a memory type index that fits the properties of the buffer
		memAlloc.memoryTypeIndex = GetVulkanMemoryType(memoryPropertyFlags, memReqs.memoryTypeBits);
	}

	// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
	VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
		allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
		allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		memAlloc.pNext = &allocFlagsInfo;
	}
	result = vkAllocateMemory(m_Device, &memAlloc, nullptr, &memory);
	check_vk_result(result);

	// If a pointer to the buffer data has been passed, map the buffer and copy over the data
	if (data != nullptr)
	{
		void* mapped;
		result = vkMapMemory(m_Device, memory, 0, size, 0, &mapped);
		check_vk_result(result);
		
		memcpy(mapped, data, size);
		// If host coherency hasn't been requested, do a manual flush to make writes visible
		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
		{
			VkMappedMemoryRange mappedRange = {};
			{
				mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				mappedRange.memory = memory;
				mappedRange.offset = 0;
				mappedRange.size = size;
			}
			vkFlushMappedMemoryRanges(m_Device, 1, &mappedRange);
		}
		vkUnmapMemory(m_Device, memory);
	}

	// Attach the memory to the buffer object
	result = vkBindBufferMemory(m_Device, buffer, memory, 0);
	check_vk_result(result);

	return VK_SUCCESS;
}

VkResult VulkanUtils::UpdateBuffer(VkDeviceMemory& memory, VkDeviceSize size, void* data)
{
	void* mapped;
	VkResult result = vkMapMemory(m_Device, memory, 0, size, 0, &mapped);
	check_vk_result(result);

	memcpy(mapped, data, size);

	vkUnmapMemory(m_Device, memory);

	return VK_SUCCESS;
}

uint32_t VulkanUtils::GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
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
