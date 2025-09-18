#include "RenderAPI_Vulkan.h"
#include <cmath>
#include "RenderingPlugin.h"
#include "Shader.h"

#define UNITY_USED_VULKAN_API_FUNCTIONS(apply) \
	apply(vkCreateInstance); \
	apply(vkCmdBeginRenderPass); \
	apply(vkCreateBuffer); \
	apply(vkGetPhysicalDeviceMemoryProperties); \
	apply(vkGetBufferMemoryRequirements); \
	apply(vkMapMemory); \
	apply(vkBindBufferMemory); \
	apply(vkAllocateMemory); \
	apply(vkDestroyBuffer); \
	apply(vkFreeMemory); \
	apply(vkUnmapMemory); \
	apply(vkQueueWaitIdle); \
	apply(vkDeviceWaitIdle); \
	apply(vkCmdCopyBufferToImage); \
	apply(vkFlushMappedMemoryRanges); \
	apply(vkCreatePipelineLayout); \
	apply(vkCreateShaderModule); \
	apply(vkDestroyShaderModule); \
	apply(vkCreateGraphicsPipelines); \
	apply(vkCmdBindPipeline); \
	apply(vkCmdDraw); \
	apply(vkCmdPushConstants); \
	apply(vkCmdBindVertexBuffers); \
	apply(vkDestroyPipeline); \
	apply(vkDestroyPipelineLayout);

#define VULKAN_DEFINE_API_FUNCPTR(func) static PFN_##func func
VULKAN_DEFINE_API_FUNCPTR(vkGetInstanceProcAddr);
UNITY_USED_VULKAN_API_FUNCTIONS(VULKAN_DEFINE_API_FUNCPTR);
#undef VULKAN_DEFINE_API_FUNCPTR

static void LoadVulkanAPI(PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkInstance instance)
{
	if (!vkGetInstanceProcAddr && getInstanceProcAddr)
		vkGetInstanceProcAddr = getInstanceProcAddr;

	if (!vkCreateInstance)
		vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");

#define LOAD_VULKAN_FUNC(fn) if (!fn) fn = (PFN_##fn)vkGetInstanceProcAddr(instance, #fn)
	UNITY_USED_VULKAN_API_FUNCTIONS(LOAD_VULKAN_FUNC);
#undef LOAD_VULKAN_FUNC
}

static int FindMemoryTypeIndex(VkPhysicalDeviceMemoryProperties const& physicalDeviceMemoryProperties, VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlags memoryPropertyFlags)
{
	uint32_t memoryTypeBits = memoryRequirements.memoryTypeBits;

	// Search memtypes to find first index with those properties
	for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < VK_MAX_MEMORY_TYPES; ++memoryTypeIndex)
	{
		if ((memoryTypeBits & 1) == 1)
		{
			// Type is available, does it match user properties?
			if ((physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
				return memoryTypeIndex;
		}
		memoryTypeBits >>= 1;
	}

	return -1;
}

static VkPipelineLayout CreateTrianglePipelineLayout(VkDevice device)
{
	VkPushConstantRange pushConstantRange;
	pushConstantRange.offset = 0;
	pushConstantRange.size = 64; // single matrix
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

	VkPipelineLayout pipelineLayout;
	return vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) == VK_SUCCESS ? pipelineLayout : VK_NULL_HANDLE;
}

static VkPipeline CreateTrianglePipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipelineCache pipelineCache)
{
	if (pipelineLayout == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;
	if (device == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;
	if (renderPass == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;

	bool success = true;

	VkPipelineShaderStageCreateInfo shaderStages[2] = {};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].pName = "main";
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].pName = "main";

	if (success)
	{
		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = sizeof(Shader::vertexShaderSpirv);
		moduleCreateInfo.pCode = Shader::vertexShaderSpirv;
		success = vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderStages[0].module) == VK_SUCCESS;
	}

	if (success)
	{
		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = sizeof(Shader::fragmentShaderSpirv);
		moduleCreateInfo.pCode = Shader::fragmentShaderSpirv;
		success = vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderStages[1].module) == VK_SUCCESS;
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	VkPipeline pipeline = VK_NULL_HANDLE;
	if (success)
	{
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.renderPass = renderPass;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.depthClampEnable = VK_FALSE;
		rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		rasterizationState.depthBiasEnable = VK_FALSE;
		rasterizationState.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState blendAttachmentState[1] = {};
		blendAttachmentState[0].colorWriteMask = 0xf;
		blendAttachmentState[0].blendEnable = VK_FALSE;
		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = blendAttachmentState;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		const VkDynamicState dynamicStateEnables[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.pDynamicStates = dynamicStateEnables;
		dynamicState.dynamicStateCount = sizeof(dynamicStateEnables) / sizeof(*dynamicStateEnables);

		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.stencilTestEnable = VK_FALSE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // Unity/Vulkan uses reverse Z
		depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
		depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
		depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
		depthStencilState.front = depthStencilState.back;

		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleState.pSampleMask = nullptr;

		// Vertex:
		// float3 vpos;
		// byte4 vcol;
		VkVertexInputBindingDescription vertexInputBinding = {};
		vertexInputBinding.binding = 0;
		vertexInputBinding.stride = 16;
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vertexInputAttributes[2];
		vertexInputAttributes[0].binding = 0;
		vertexInputAttributes[0].location = 0;
		vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributes[0].offset = 0;
		vertexInputAttributes[1].binding = 0;
		vertexInputAttributes[1].location = 1;
		vertexInputAttributes[1].format = VK_FORMAT_R8G8B8A8_UNORM;
		vertexInputAttributes[1].offset = 12;

		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = 1;
		vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputState.vertexAttributeDescriptionCount = 2;
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes;

		pipelineCreateInfo.stageCount = sizeof(shaderStages) / sizeof(*shaderStages);
		pipelineCreateInfo.pStages = shaderStages;
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;

		success = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, NULL, &pipeline) == VK_SUCCESS;
	}

	if (shaderStages[0].module != VK_NULL_HANDLE)
		vkDestroyShaderModule(device, shaderStages[0].module, NULL);
	if (shaderStages[1].module != VK_NULL_HANDLE)
		vkDestroyShaderModule(device, shaderStages[1].module, NULL);

	return success ? pipeline : VK_NULL_HANDLE;
}

//=============================================================================================================================================================

RenderAPI_Vulkan::RenderAPI_Vulkan()
	:m_UnityVulkan(nullptr), m_Instance{}, m_TrianglePipelineLayout(VK_NULL_HANDLE), m_TrianglePipeline(VK_NULL_HANDLE), m_TrianglePipelineRenderPass(VK_NULL_HANDLE), m_VertexBuffer{}
{
}

void RenderAPI_Vulkan::ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
{
	switch (type)
	{
	case kUnityGfxDeviceEventInitialize:
		m_UnityVulkan = interfaces->Get<IUnityGraphicsVulkan>();
		m_Instance = m_UnityVulkan->Instance();

		// Make sure Vulkan API functions are loaded
		LoadVulkanAPI(m_Instance.getInstanceProcAddr, m_Instance.instance);

		UnityVulkanPluginEventConfig config_1;
		config_1.graphicsQueueAccess = kUnityVulkanGraphicsQueueAccess_DontCare;
		config_1.renderPassPrecondition = kUnityVulkanRenderPass_EnsureInside;
		config_1.flags = kUnityVulkanEventConfigFlag_EnsurePreviousFrameSubmission | kUnityVulkanEventConfigFlag_ModifiesCommandBuffersState;
		m_UnityVulkan->ConfigureEvent(1, &config_1);

		// alternative way to intercept API
		//m_UnityVulkan->InterceptVulkanAPI("vkCmdBeginRenderPass", (PFN_vkVoidFunction)Hook_vkCmdBeginRenderPass);

		CreateTraingleBuffer();

		break;
	case kUnityGfxDeviceEventShutdown:

		ImmediateDestroyVulkanBuffer(m_VertexBuffer);

		if (m_Instance.device != VK_NULL_HANDLE)
		{
			//GarbageCollect(true); // TODO
			if (m_TrianglePipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(m_Instance.device, m_TrianglePipeline, nullptr);
				m_TrianglePipeline = VK_NULL_HANDLE;
			}
			if (m_TrianglePipelineLayout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(m_Instance.device, m_TrianglePipelineLayout, nullptr);
				m_TrianglePipelineLayout = VK_NULL_HANDLE;
			}
		}

		m_UnityVulkan = nullptr;
		m_TrianglePipelineRenderPass = VK_NULL_HANDLE;
		m_Instance = UnityVulkanInstance();

		break;
	}
}

void RenderAPI_Vulkan::DrawColoredTriangle()
{
	UnityVulkanRecordingState recordingState;
	if (!m_UnityVulkan->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
		return;

	// Unity does not destroy render passes, so this is safe regarding ABA-problem
	if (recordingState.renderPass != m_TrianglePipelineRenderPass)
	{
		if (m_TrianglePipelineLayout == VK_NULL_HANDLE)
			m_TrianglePipelineLayout = CreateTrianglePipelineLayout(m_Instance.device);

		m_TrianglePipeline = CreateTrianglePipeline(m_Instance.device, m_TrianglePipelineLayout, recordingState.renderPass, VK_NULL_HANDLE);
		m_TrianglePipelineRenderPass = recordingState.renderPass;
	}

	if (m_TrianglePipeline != VK_NULL_HANDLE && m_TrianglePipelineLayout != VK_NULL_HANDLE)
	{
		// Transformation matrix: rotate around Z axis based on time.
		float phi = RenderingPlugin::Time; // time set externally from Unity script
		float cosPhi = cosf(phi);
		float sinPhi = sinf(phi);
		float depth = 0.7f;
		float finalDepth = GetUsesReverseZ() ? 1.0f - depth : depth;
		float worldMatrix[16] = {
			cosPhi,-sinPhi,0,0,
			sinPhi,cosPhi,0,0,
			0,0,1,0,
			0,0,finalDepth,1,
		};

		//SafeDestroy(recordingState.currentFrameNumber, buffer);
		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(recordingState.commandBuffer, 0, 1, &m_VertexBuffer.buffer, &offset);
		vkCmdPushConstants(recordingState.commandBuffer, m_TrianglePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, (const void*)worldMatrix);
		vkCmdBindPipeline(recordingState.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_TrianglePipeline);
		vkCmdDraw(recordingState.commandBuffer, 1 * 3, 1, 0, 0);
	}
}

void RenderAPI_Vulkan::CreateTraingleBuffer()
{
	// Draw a colored triangle. Note that colors will come out differently
	// in D3D and OpenGL, for example, since they expect color bytes
	// in different ordering.
	struct MyVertex
	{
		float x, y, z;
		unsigned int color;
	};
	MyVertex verts[3] =
	{
		{ -0.5f, -0.25f,  0, 0xFFff0000 },
		{ 0.5f, -0.25f,  0, 0xFF00ff00 },
		{ 0,     0.5f ,  0, 0xFF0000ff },
	};

	if (!CreateVulkanBuffer(16 * 3 * 1, &m_VertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
		return;

	memcpy(m_VertexBuffer.mapped, verts, static_cast<size_t>(m_VertexBuffer.sizeInBytes));
	if (!(m_VertexBuffer.deviceMemoryFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
	{
		VkMappedMemoryRange range;
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.pNext = NULL;
		range.memory = m_VertexBuffer.deviceMemory;
		range.offset = 0;
		range.size = m_VertexBuffer.deviceMemorySize;
		vkFlushMappedMemoryRanges(m_Instance.device, 1, &range);
	}

	UNITY_LOG(RenderingPlugin::UnityLog, "Created Vertex Buffer");
}

bool RenderAPI_Vulkan::CreateVulkanBuffer(size_t sizeInBytes, VulkanBuffer* buffer, VkBufferUsageFlags usage)
{
	if (sizeInBytes == 0)
		return false;

	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.pQueueFamilyIndices = &m_Instance.queueFamilyIndex;
	bufferCreateInfo.queueFamilyIndexCount = 1;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = sizeInBytes;

	*buffer = VulkanBuffer();

	if (vkCreateBuffer(m_Instance.device, &bufferCreateInfo, NULL, &buffer->buffer) != VK_SUCCESS)
		return false;

	VkPhysicalDeviceMemoryProperties physicalDeviceProperties;
	vkGetPhysicalDeviceMemoryProperties(m_Instance.physicalDevice, &physicalDeviceProperties);

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_Instance.device, buffer->buffer, &memoryRequirements);

	const int memoryTypeIndex = FindMemoryTypeIndex(physicalDeviceProperties, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	if (memoryTypeIndex < 0)
	{
		ImmediateDestroyVulkanBuffer(*buffer);
		return false;
	}

	VkMemoryAllocateInfo memoryAllocateInfo;
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = NULL;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;

	if (vkAllocateMemory(m_Instance.device, &memoryAllocateInfo, NULL, &buffer->deviceMemory) != VK_SUCCESS)
	{
		ImmediateDestroyVulkanBuffer(*buffer);
		return false;
	}

	if (vkMapMemory(m_Instance.device, buffer->deviceMemory, 0, VK_WHOLE_SIZE, 0, &buffer->mapped) != VK_SUCCESS)
	{
		ImmediateDestroyVulkanBuffer(*buffer);
		return false;
	}

	if (vkBindBufferMemory(m_Instance.device, buffer->buffer, buffer->deviceMemory, 0) != VK_SUCCESS)
	{
		ImmediateDestroyVulkanBuffer(*buffer);
		return false;
	}

	buffer->sizeInBytes = sizeInBytes;
	buffer->deviceMemoryFlags = physicalDeviceProperties.memoryTypes[memoryTypeIndex].propertyFlags;
	buffer->deviceMemorySize = memoryAllocateInfo.allocationSize;

	return true;
}

void RenderAPI_Vulkan::ImmediateDestroyVulkanBuffer(const VulkanBuffer& buffer)
{
	if (buffer.buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(m_Instance.device, buffer.buffer, NULL);

	if (buffer.mapped && buffer.deviceMemory != VK_NULL_HANDLE)
		vkUnmapMemory(m_Instance.device, buffer.deviceMemory);

	if (buffer.deviceMemory != VK_NULL_HANDLE)
		vkFreeMemory(m_Instance.device, buffer.deviceMemory, NULL);
}

RenderAPI* CreateRenderAPI_Vulkan()
{
	return new RenderAPI_Vulkan();
}