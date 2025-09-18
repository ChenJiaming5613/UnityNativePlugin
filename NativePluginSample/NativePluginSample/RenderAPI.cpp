#include "RenderAPI.h"
#include "PlatformBase.h"

RenderAPI* CreateRenderAPI(UnityGfxRenderer apiType)
{
#if SUPPORT_VULKAN
	if (apiType == kUnityGfxRendererVulkan)
	{
		extern RenderAPI* CreateRenderAPI_Vulkan();
		return CreateRenderAPI_Vulkan();
	}
#endif // if SUPPORT_VULKAN

	// Unknown or unsupported graphics API
	return nullptr;
}
