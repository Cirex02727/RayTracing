#include "ComputeShader.h"

#include "Walnut/Application.h"

#include "utils/VulkanUtils.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>

ComputeShader::ComputeShader(uint32_t width, uint32_t height)
{
	m_OutImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	float* data = new float[width * height];
	memset(data, 1, width * height * sizeof(float));
	m_OutImage->SetData(data);

	Device = Walnut::Application::GetDevice();

	prepareStorageBuffers();
	setupDescriptorSetLayout();
	preparePipelines();
	setupDescriptorPool();
	setupDescriptorSet();
	prepareCompute();
	buildComputeCommandBuffer();

	Render();
}

ComputeShader::~ComputeShader()
{
	vkDestroyPipeline(Device, Pipeline, nullptr);
	vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(Device, DescriptorSetLayout, nullptr);

	vkDestroyFence(Device, Fence, nullptr);
	vkFreeMemory(Device, InBufferMemory, nullptr);
}

void ComputeShader::Render()
{
	vkWaitForFences(Device, 1, &Fence, VK_TRUE, UINT64_MAX);
	vkResetFences(Device, 1, &Fence);

	VkResult res = vkGetFenceStatus(Device, Fence);

	VkSubmitInfo computeSubmitInfo{};
	{
		computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &CommandBuffer;
	}

	VkResult err = vkQueueSubmit(Queue, 1, &computeSubmitInfo, Fence);
	check_vk_result(err);

	res = vkGetFenceStatus(Device, Fence);
}

void ComputeShader::UpdateBuffers(glm::vec3& color)
{
	VulkanUtils::UpdateBuffer(InBufferMemory,
		sizeof(glm::vec3),
		glm::value_ptr(color));
}

void ComputeShader::prepareStorageBuffers()
{
	glm::vec3 default_color { 0.1f, 0.1f, 0.1f };
	VulkanUtils::CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(glm::vec3),
		InBuffer,
		InBufferMemory,
		glm::value_ptr(default_color));
}

void ComputeShader::setupDescriptorSetLayout()
{
	VkResult err;

	// Create Bindings
	std::vector<VkDescriptorSetLayoutBinding> bindings = {
		{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  1, VK_SHADER_STAGE_COMPUTE_BIT },
		{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT }
	};

	VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutBinding = {};
	DescriptorSetLayoutBinding.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	DescriptorSetLayoutBinding.pBindings = bindings.data();
	DescriptorSetLayoutBinding.bindingCount = bindings.size();

	// Create Descriptors
	err = vkCreateDescriptorSetLayout(Device, &DescriptorSetLayoutBinding, nullptr, &DescriptorSetLayout);
	check_vk_result(err);

	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{};
	{
		PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		PipelineLayoutCreateInfo.pSetLayouts = &DescriptorSetLayout;
		PipelineLayoutCreateInfo.setLayoutCount = 1;
	}

	err = vkCreatePipelineLayout(Device, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout);
	check_vk_result(err);
}

void ComputeShader::preparePipelines()
{
	VkResult err;

	// Load Compute Shader
	std::ifstream file("./shaders/test.comp.svp", std::ios::ate | std::ios::binary);
	size_t fileSize = (size_t)file.tellg();

	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	VkShaderModuleCreateInfo ShaderModuleCreateInfo = {};
	{
		ShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ShaderModuleCreateInfo.pNext = nullptr;

		ShaderModuleCreateInfo.codeSize = buffer.size() * sizeof(uint32_t);
		ShaderModuleCreateInfo.pCode = buffer.data();
	}


	// Create Pipeline
	VkShaderModule ShaderModule;
	err = vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &ShaderModule);
	check_vk_result(err);

	VkPipelineShaderStageCreateInfo PipelineShaderCreateInfo = {};
	{
		PipelineShaderCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		PipelineShaderCreateInfo.module = ShaderModule;
		PipelineShaderCreateInfo.pName = "main";
	}

	VkComputePipelineCreateInfo ComputePipelineCreateInfo{};
	{
		ComputePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		ComputePipelineCreateInfo.stage = PipelineShaderCreateInfo;
		ComputePipelineCreateInfo.layout = PipelineLayout;
		ComputePipelineCreateInfo.flags = 0;
	}

	err = vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1, &ComputePipelineCreateInfo, nullptr, &Pipeline);
	check_vk_result(err);
}

void ComputeShader::setupDescriptorPool()
{
	VkResult err;

	std::vector<VkDescriptorPoolSize> DescriptorPoolSizes = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
	};

	VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
	{
		DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		DescriptorPoolCreateInfo.poolSizeCount = DescriptorPoolSizes.size();
		DescriptorPoolCreateInfo.pPoolSizes = DescriptorPoolSizes.data();
		DescriptorPoolCreateInfo.maxSets = 3;
	}

	err = vkCreateDescriptorPool(Device, &DescriptorPoolCreateInfo, nullptr, &DescriptorPool);
	check_vk_result(err);
}

void ComputeShader::setupDescriptorSet()
{
	VkResult err;

	// Allocate Descriptor Set
	VkDescriptorSetAllocateInfo DescriptorSetAllocInfo = {};
	{
		DescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		DescriptorSetAllocInfo.descriptorPool = DescriptorPool;
		DescriptorSetAllocInfo.pSetLayouts = &DescriptorSetLayout;
		DescriptorSetAllocInfo.descriptorSetCount = 1;
	};

	err = vkAllocateDescriptorSets(Device, &DescriptorSetAllocInfo, &DescriptorSet);
	check_vk_result(err);


	// Upload Descriptor Set
	VkDescriptorImageInfo InImageInfo = {};
	m_OutImage->GetDescriptorImageInfo(InImageInfo);

	VkWriteDescriptorSet WriteDescriptorSet1 = {};
	{
		WriteDescriptorSet1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSet1.dstSet = DescriptorSet;
		WriteDescriptorSet1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		WriteDescriptorSet1.dstBinding = 0;
		WriteDescriptorSet1.pImageInfo = &InImageInfo;
		WriteDescriptorSet1.descriptorCount = 1;
	}

	VkDescriptorBufferInfo InBufferInfo = {};
	{
		InBufferInfo.buffer = InBuffer;
		InBufferInfo.offset = 0;
		InBufferInfo.range = InAlignedSize;
	}

	VkWriteDescriptorSet WriteDescriptorSet2 = {};
	{
		WriteDescriptorSet2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		WriteDescriptorSet2.dstSet = DescriptorSet;
		WriteDescriptorSet2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		WriteDescriptorSet2.dstBinding = 1;
		WriteDescriptorSet2.pBufferInfo = &InBufferInfo;
		WriteDescriptorSet2.descriptorCount = 1;
	}

	const std::vector<VkWriteDescriptorSet> WriteDescriptorSets = { WriteDescriptorSet1, WriteDescriptorSet2 };

	vkUpdateDescriptorSets(Device, WriteDescriptorSets.size(),
		WriteDescriptorSets.data(), 0, nullptr);
}

void ComputeShader::prepareCompute()
{
	VkPhysicalDevice physicalDevice = Walnut::Application::GetPhysicalDevice();

	uint32_t count;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, NULL);
	VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*) malloc(sizeof(VkQueueFamilyProperties) * count);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queues);
	for (uint32_t i = 0; i < count; i++)
		if (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			QueueFamily = i;
			break;
		}
	free(queues);


	VkDeviceQueueCreateInfo queueCreateInfo = {};
	{
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = NULL;
		queueCreateInfo.queueFamilyIndex = QueueFamily;
		queueCreateInfo.queueCount = 1;
	}
	vkGetDeviceQueue(Device, QueueFamily, 0, &Queue);


	// Separate command pool as queue family for compute may be different than graphics
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = QueueFamily;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkResult err = vkCreateCommandPool(Device, &cmdPoolInfo, nullptr, &CommandPool);
	check_vk_result(err);


	VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = CommandPool;
	cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocateInfo.commandBufferCount = 1;

	err = vkAllocateCommandBuffers(Device, &cmdBufAllocateInfo, &CommandBuffer);
	check_vk_result(err);


	// Fence for compute CB sync
	VkFenceCreateInfo fenceCreateInfo{};
	{
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}
	err = vkCreateFence(Device, &fenceCreateInfo, nullptr, &Fence);
	check_vk_result(err);
}

void ComputeShader::buildComputeCommandBuffer()
{
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	VkResult err = vkBeginCommandBuffer(CommandBuffer, &begin_info);
	check_vk_result(err);

	// Image Barrier
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageMemoryBarrier.image = m_OutImage->GetImage();
	imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	
	vkCmdPipelineBarrier(CommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
	

	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
	vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout, 0, 1, &DescriptorSet, 0, 0);

	vkCmdDispatch(CommandBuffer, m_OutImage->GetWidth() / 16, m_OutImage->GetHeight() / 16, 1);


	err = vkEndCommandBuffer(CommandBuffer);
	check_vk_result(err);
}
