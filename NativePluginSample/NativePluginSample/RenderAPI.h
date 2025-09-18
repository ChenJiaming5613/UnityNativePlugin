#pragma once

#include <IUnityGraphics.h>

class RenderAPI
{
public:
	virtual ~RenderAPI() {}

	// Process general event like initialization, shutdown, device loss/reset etc.
	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) = 0;

	// Is the API using "reversed" (1.0 at near plane, 0.0 at far plane) depth buffer?
	// Reversed Z is used on modern platforms, and improves depth buffer precision.
	virtual bool GetUsesReverseZ() = 0;

	virtual void DrawColoredTriangle() = 0;
};

// Create a graphics API implementation instance for the given API type.
RenderAPI* CreateRenderAPI(UnityGfxRenderer apiType);
