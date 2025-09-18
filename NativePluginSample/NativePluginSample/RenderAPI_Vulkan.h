#pragma once

#include <IUnityGraphics.h>
// This plugin does not link to the Vulkan loader, easier to support multiple APIs and systems that don't have Vulkan support
#define VK_NO_PROTOTYPES
#include <IUnityGraphicsVulkan.h>
#include "RenderAPI.h"

struct VulkanBuffer
{
	VkBuffer buffer;
	VkDeviceMemory deviceMemory;
	void* mapped;
	VkDeviceSize sizeInBytes;
	VkDeviceSize deviceMemorySize;
	VkMemoryPropertyFlags deviceMemoryFlags;
};

class RenderAPI_Vulkan : public RenderAPI
{
public:
	RenderAPI_Vulkan();
	virtual ~RenderAPI_Vulkan() {}

	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces);

	virtual bool GetUsesReverseZ() { return true; }

	virtual void DrawColoredTriangle();

private:
	void CreateTraingleBuffer();

	bool CreateVulkanBuffer(size_t sizeInBytes, VulkanBuffer* buffer, VkBufferUsageFlags usage);

	void ImmediateDestroyVulkanBuffer(const VulkanBuffer& buffer);

	IUnityGraphicsVulkan* m_UnityVulkan;
	UnityVulkanInstance m_Instance;

	VkPipelineLayout m_TrianglePipelineLayout;
	VkPipeline m_TrianglePipeline;
	VkRenderPass m_TrianglePipelineRenderPass;
	VulkanBuffer m_VertexBuffer;
};
