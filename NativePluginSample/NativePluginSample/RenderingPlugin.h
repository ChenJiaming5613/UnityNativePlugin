#pragma once

#include <IUnityInterface.h>
#include <IUnityGraphics.h>
#include <IUnityLog.h>
#include "RenderAPI.h"

class RenderingPlugin
{
public:
	RenderingPlugin() = delete;

	static IUnityInterfaces* UnityInterfaces;
	static IUnityGraphics* UnityGraphics;
	static IUnityLog* UnityLog;
	static UnityGfxRenderer RHIType;
	static RenderAPI* CurrentAPI;
	static float Time;
};
