#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"
#include "Walnut/Input/Input.h"

#include "Renderer.h"
#include "Camera.h"

#include "utils/VulkanUtils.h"

#include "ComputeShader.h"

#include "octree/Octree.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <filesystem>
#include <fstream>

class ExampleLayer : public Walnut::Layer
{
public:
	static uint32_t GetVulkanMemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
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

	ExampleLayer()
		: m_Camera(45.0f, 0.1f, 100.0f), m_ComputeShader(1600, 874)
	{
		{
			Sphere sphere;
			sphere.Position = { 0.0f, 0.0f, 0.0f };
			sphere.Radius = 0.5f;
			sphere.Albedo = { 1.0f, 0.0f, 1.0f };
			m_Scene.Spheres.push_back(sphere);
		}

		{
			Sphere sphere;
			sphere.Position = { 0.0f, 0.0f, -5.0f };
			sphere.Radius = 1.5f;
			sphere.Albedo = { 0.2f, 0.3f, 1.0f };
			m_Scene.Spheres.push_back(sphere);
		}
	}

	virtual void OnUpdate(float ts) override
	{
		m_Camera.OnUpdate(ts);

		if (Walnut::Input::IsKeyDown(Walnut::KeyCode::P))
		{
			std::cout << glm::to_string(m_Camera.GetPosition()) << " " << glm::to_string(m_Camera.GetDirection()) << std::endl;
		}
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Global Info");
		ImGui::Text("Last render: %.1ffps (%.3fms)", (1000 / m_LastRenderTime), m_LastRenderTime);
		if (ImGui::Button("Render"))
		{
			Render();
		}
		if (ImGui::Button("RunComputeShader"))
		{
			RunComputeShader();
		}
		ImGui::ColorEdit3("Compute Color", glm::value_ptr(m_Color));

		const glm::vec3& cameraPos = m_Camera.GetPosition();
		const glm::vec3& cameraRot = m_Camera.GetDirection();
		ImGui::Text("Camera Pos: %.1f; %.1f; %.1f", cameraPos.x, cameraPos.y, cameraPos.z);
		ImGui::Text("Camera Rot: %.1f; %.1f; %.1f", cameraRot.x, cameraRot.y, cameraRot.z);
		ImGui::End();

		ImGui::Begin("Render Settings");
		ImGui::Checkbox("Render Light", &m_RenderLight);
		ImGui::Checkbox("Render Normal", &m_RenderNormal);
		ImGui::End();

		ImGui::Begin("Scene");
		for (int i = 0; i < m_Scene.Spheres.size(); i++)
		{
			ImGui::PushID(i);

			Sphere& sphere = m_Scene.Spheres[i];
			ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
			ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
			ImGui::ColorEdit3("Albedo", glm::value_ptr(sphere.Albedo));

			ImGui::Separator();

			ImGui::PopID();
		}
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		m_ViewportWidth = static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
		m_ViewportHeight = static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);

		auto image = m_Renderer.GetFinalImage();
		// auto image = m_ComputeShader.GetOutImage();
		if(image)
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() },
				ImVec2(0, 1), ImVec2(1, 0));

		ImGui::End();
		ImGui::PopStyleVar();

		Render();
	}

	void Render()
	{
		Walnut::Timer timer;

		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_Scene, m_Camera, m_RenderLight, m_RenderNormal);

		// m_ComputeShader.Render();
		// m_ComputeShader.UpdateBuffers(m_Color);

		m_LastRenderTime = timer.ElapsedMillis();
	}

	void RunComputeShader()
	{
		VkDevice device = Walnut::Application::GetDevice();

		const uint32_t NumElements = 10;
		const uint32_t BufferSize = NumElements * sizeof(int32_t);
		VkResult err;

		VkBuffer InBuffer, OutBuffer;
		VkDeviceMemory InBufferMemory, OutBufferMemory;
		size_t InAlignedSize, OutAlignedSize;

		// Create the Input Buffer
		{
			VkBufferCreateInfo buffer_info = {};
			{
				buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				buffer_info.size = BufferSize;
				buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}
			err = vkCreateBuffer(device, &buffer_info, nullptr, &InBuffer);
			check_vk_result(err);

			VkMemoryRequirements req;
			vkGetBufferMemoryRequirements(device, InBuffer, &req);
			InAlignedSize = req.size;

			VkMemoryAllocateInfo alloc_info = {};
			{
				alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				alloc_info.allocationSize = req.size;
				alloc_info.memoryTypeIndex = GetVulkanMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
			}
			err = vkAllocateMemory(device, &alloc_info, nullptr, &InBufferMemory);
			check_vk_result(err);

			err = vkBindBufferMemory(device, InBuffer, InBufferMemory, 0);
			check_vk_result(err);
		}

		// Create the Output Buffer
		{
			VkBufferCreateInfo buffer_info = {};
			{
				buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				buffer_info.size = BufferSize;
				buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}
			err = vkCreateBuffer(device, &buffer_info, nullptr, &OutBuffer);
			check_vk_result(err);

			VkMemoryRequirements req;
			vkGetBufferMemoryRequirements(device, OutBuffer, &req);
			OutAlignedSize = req.size;

			VkMemoryAllocateInfo alloc_info = {};
			{
				alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				alloc_info.allocationSize = req.size;
				alloc_info.memoryTypeIndex = GetVulkanMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
			}
			err = vkAllocateMemory(device, &alloc_info, nullptr, &OutBufferMemory);
			check_vk_result(err);

			err = vkBindBufferMemory(device, OutBuffer, OutBufferMemory, 0);
			check_vk_result(err);
		}

		// Upload to Buffer
		{
			int32_t* map = NULL;
			err = vkMapMemory(device, InBufferMemory, 0, InAlignedSize, 0, (void**)(&map));
			check_vk_result(err);

			for (int32_t i = 0; i < NumElements; ++i)
			{
				map[i] = i;
			}

			VkMappedMemoryRange range[1] = {};
			range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = InBufferMemory;
			range[0].size = InAlignedSize;
			err = vkFlushMappedMemoryRanges(device, 1, range);
			check_vk_result(err);

			vkUnmapMemory(device, InBufferMemory);
		}

		// Load Compute Shader
		std::ifstream file("./shaders/test_buffers.comp.svp", std::ios::ate | std::ios::binary);
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

		VkShaderModule ShaderModule;
		err = vkCreateShaderModule(device, &ShaderModuleCreateInfo, nullptr, &ShaderModule);
		check_vk_result(err);


		// Create Bindings
		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutBinding = {};

		VkDescriptorSetLayoutBinding binding1 = {};
		{
			binding1.binding = 0;
			binding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding1.descriptorCount = 1;
			binding1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		}

		VkDescriptorSetLayoutBinding binding2 = {};
		{
			binding2.binding = 1;
			binding2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding2.descriptorCount = 1;
			binding2.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		}

		std::vector<VkDescriptorSetLayoutBinding> bindings = { binding1, binding2 };

		DescriptorSetLayoutBinding.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		DescriptorSetLayoutBinding.pBindings = bindings.data();
		DescriptorSetLayoutBinding.bindingCount = static_cast<uint32_t>(bindings.size());

		// Create Descriptos
		VkDescriptorSetLayout DescriptorSetLayout = {};
		err = vkCreateDescriptorSetLayout(device, &DescriptorSetLayoutBinding, nullptr, &DescriptorSetLayout);
		check_vk_result(err);

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{};
		{
			PipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			PipelineLayoutCreateInfo.pSetLayouts = &DescriptorSetLayout;
			PipelineLayoutCreateInfo.setLayoutCount = 1;
		}


		VkPipelineLayout PipelineLayout;
		err = vkCreatePipelineLayout(device, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout);
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

		VkPipeline Pipeline;
		err = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &ComputePipelineCreateInfo, nullptr, &Pipeline);
		check_vk_result(err);

		VkDescriptorPoolSize DescriptorPoolSize = {};
		{
			DescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			DescriptorPoolSize.descriptorCount = 2;
		}

		VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo = {};
		{
			DescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			DescriptorPoolCreateInfo.poolSizeCount = 2;
			DescriptorPoolCreateInfo.pPoolSizes = &DescriptorPoolSize;
			DescriptorPoolCreateInfo.maxSets = 2;
		}

		VkDescriptorPool DescriptorPool = {};
		err = vkCreateDescriptorPool(device, &DescriptorPoolCreateInfo, nullptr, &DescriptorPool);
		check_vk_result(err);


		VkDescriptorSetAllocateInfo DescriptorSetAllocInfo = {};
		{
			DescriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			DescriptorSetAllocInfo.descriptorPool = DescriptorPool;
			DescriptorSetAllocInfo.pSetLayouts = &DescriptorSetLayout;
			DescriptorSetAllocInfo.descriptorSetCount = 1;
		};

		VkDescriptorSet DescriptorSet = {};
		err = vkAllocateDescriptorSets(device, &DescriptorSetAllocInfo, &DescriptorSet);
		check_vk_result(err);

		VkDescriptorBufferInfo InBufferInfo = {};
		{
			InBufferInfo.buffer = InBuffer;
			InBufferInfo.offset = 0;
			InBufferInfo.range = NumElements * sizeof(int32_t);
		}
		VkDescriptorBufferInfo OutBufferInfo = {};
		{
			OutBufferInfo.buffer = OutBuffer;
			OutBufferInfo.offset = 0;
			OutBufferInfo.range = NumElements * sizeof(int32_t);
		}

		VkWriteDescriptorSet WriteDescriptorSet1 = {};
		{
			WriteDescriptorSet1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSet1.dstSet = DescriptorSet;
			WriteDescriptorSet1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			WriteDescriptorSet1.dstBinding = 0;
			WriteDescriptorSet1.pBufferInfo = &InBufferInfo;
			WriteDescriptorSet1.descriptorCount = 1;
		}

		VkWriteDescriptorSet WriteDescriptorSet2 = {};
		{
			WriteDescriptorSet2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			WriteDescriptorSet2.dstSet = DescriptorSet;
			WriteDescriptorSet2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			WriteDescriptorSet2.dstBinding = 1;
			WriteDescriptorSet2.pBufferInfo = &OutBufferInfo;
			WriteDescriptorSet2.descriptorCount = 1;
		}

		const std::vector<VkWriteDescriptorSet> WriteDescriptorSets = { WriteDescriptorSet1, WriteDescriptorSet2 };

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(WriteDescriptorSets.size()),
			WriteDescriptorSets.data(), 0, nullptr);


		// Free Memory
		Walnut::Application::SubmitResourceFree([shaderModule = ShaderModule,
			inBufferMemory = InBufferMemory,outBufferMemory = OutBufferMemory,
			inBuffer = InBuffer, outBuffer = OutBuffer]()
			{
				VkDevice device = Walnut::Application::GetDevice();

				vkDestroyShaderModule(device, shaderModule, nullptr);
				vkFreeMemory(device, inBufferMemory, nullptr);
				vkFreeMemory(device, outBufferMemory, nullptr);
				vkDestroyBuffer(device, inBuffer, nullptr);
				vkDestroyBuffer(device, outBuffer, nullptr);
			});


		// Flush Command Buffer
		VkCommandBuffer command_buffer = Walnut::Application::GetCommandBuffer(true);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, Pipeline);
		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, PipelineLayout,
			0, 1, &DescriptorSet, 0, nullptr);
		vkCmdDispatch(command_buffer, NumElements, 1, 1);

		Walnut::Application::FlushCommandBuffer(command_buffer);


		// Check Data
		{
			int32_t* map = NULL;
			err = vkMapMemory(device, InBufferMemory, 0, InAlignedSize, 0, (void**)(&map));
			check_vk_result(err);

			for (int32_t i = 0; i < NumElements; ++i)
			{
				std::cout << map[i] << " ";
			}
			std::cout << std::endl;

			vkUnmapMemory(device, InBufferMemory);
		}

		{
			int32_t* map = NULL;
			err = vkMapMemory(device, OutBufferMemory, 0, OutAlignedSize, 0, (void**)(&map));
			check_vk_result(err);

			for (int32_t i = 0; i < NumElements; ++i)
			{
				std::cout << map[i] << " ";
			}
			std::cout << std::endl;

			vkUnmapMemory(device, OutBufferMemory);
		}

		// Free Memory
		Walnut::Application::SubmitResourceFree([pipeline = Pipeline, pipelineLayout = PipelineLayout,
			setLayout = DescriptorSetLayout, descriptorPool = DescriptorPool, descriptorSet = DescriptorSet]()
			{
				VkDevice device = Walnut::Application::GetDevice();

				vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
				vkDestroyPipeline(device, pipeline, nullptr);
				vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
				vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
				vkDestroyDescriptorPool(device, descriptorPool, nullptr);
			});
	}

private:
	Renderer m_Renderer;
	Camera m_Camera;
	Scene m_Scene;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	ComputeShader m_ComputeShader;

	glm::vec3 m_Color { 0.8f, 0.2, 0.3 };

	float m_LastRenderTime = 0;

	bool m_RenderLight = true, m_RenderNormal = false;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Walnut Example";

	Walnut::Application* app = new Walnut::Application(spec);

	VulkanUtils::Init();

	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}