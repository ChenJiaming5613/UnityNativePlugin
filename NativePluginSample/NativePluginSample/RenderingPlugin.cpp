#include <assert.h>
#include <format>
#include "RenderingPlugin.h"
#include "PlatformBase.h"

IUnityInterfaces* RenderingPlugin::UnityInterfaces = nullptr;
IUnityGraphics* RenderingPlugin::UnityGraphics = nullptr;
IUnityLog* RenderingPlugin::UnityLog = nullptr;
UnityGfxRenderer RenderingPlugin::RHIType = kUnityGfxRendererNull;
RenderAPI* RenderingPlugin::CurrentAPI = nullptr;
float RenderingPlugin::Time;

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static void UNITY_INTERFACE_API OnRenderEvent(int eventID);

extern "C" void	UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
	RenderingPlugin::UnityInterfaces = unityInterfaces;
	RenderingPlugin::UnityLog = RenderingPlugin::UnityInterfaces->Get<IUnityLog>();
	RenderingPlugin::UnityGraphics = RenderingPlugin::UnityInterfaces->Get<IUnityGraphics>();
	RenderingPlugin::UnityGraphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

	// Run OnGraphicsDeviceEvent(initialize) manually on plugin load
	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
	RenderingPlugin::UnityGraphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
	RenderingPlugin::UnityLog = nullptr;
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return OnRenderEvent;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTimeFromUnity(float t)
{ 
	RenderingPlugin::Time = t;
	UNITY_LOG(RenderingPlugin::UnityLog, std::format("SetTimeFromUnity: {}", t).c_str());
}
 
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	// Create graphics API implementation upon initialization
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		assert(RenderingPlugin::CurrentAPI == nullptr);
		RenderingPlugin::RHIType = RenderingPlugin::UnityGraphics->GetRenderer();
		RenderingPlugin::CurrentAPI = CreateRenderAPI(RenderingPlugin::RHIType);
	}

	// Let the implementation process the device related events
	if (RenderingPlugin::CurrentAPI)
	{
		RenderingPlugin::CurrentAPI->ProcessDeviceEvent(eventType, RenderingPlugin::UnityInterfaces);
	}

	// Cleanup graphics API implementation upon shutdown
	if (eventType == kUnityGfxDeviceEventShutdown)
	{
		delete RenderingPlugin::CurrentAPI;
		RenderingPlugin::CurrentAPI = nullptr;
		RenderingPlugin::RHIType = kUnityGfxRendererNull;
	}
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
	// Unknown / unsupported graphics device type? Do nothing
	if (RenderingPlugin::CurrentAPI == NULL)
		return;

	if (eventID == 1)
	{
		RenderingPlugin::CurrentAPI->DrawColoredTriangle();
	}
}