using System;
using System.Runtime.InteropServices;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

public class TestRendererFeature : ScriptableRendererFeature
{
    private TestRenderPass _testRenderPass;
    
    public override void Create()
    {
        _testRenderPass = new TestRenderPass
        {
            renderPassEvent = RenderPassEvent.AfterRenderingOpaques
        };
    }

    public override void AddRenderPasses(ScriptableRenderer renderer, ref RenderingData renderingData)
    {
        renderer.EnqueuePass(_testRenderPass);
    }
}

public class TestRenderPass : ScriptableRenderPass
{
    [DllImport("NativePluginSample")]
    private static extern void SetTimeFromUnity(float t);
    
    [DllImport("NativePluginSample")]
    private static extern IntPtr GetRenderEventFunc();

    public TestRenderPass()
    {
        SetTimeFromUnity(1.23f);
    }
    
    public override void Execute(ScriptableRenderContext context, ref RenderingData renderingData)
    {
        var cmd = CommandBufferPool.Get();
        cmd.IssuePluginEvent(GetRenderEventFunc(), 1);
        context.ExecuteCommandBuffer(cmd);
        CommandBufferPool.Release(cmd);
    }
}
