# UnityNativePlugin

## Environment

- Visual Studio 2022
- Unity 2022.3.50f1 URP
- Vulkan SDK 1.4.321.0
- x64 C++20

Set Environment Variables:
- `UNITY_NATIVE_PLUGIN_API`: `C:\Program Files\Unity\Hub\Editor\2022.3.50f1\Editor\Data\PluginAPI`
  
## Use Plugin in Unity

- Create Directory: `Assets/Plugins/x86_64/`
- Copy dll to `Assets/Plugins/x86_64/NativePluginSample.dll`
- Switch Backend API to `Vulkan` (Default is `DX11`) & Restart Unity Editor
- Disable MSAA (Default) in URP-HighFidelity Pipeline (**ToDo**)
- Create some opaque objects (such as cube) (**ToDo**)
- Then you will see a colored triangle in the center of the screen :)!
