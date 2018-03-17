using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public static class D3D11PluginsNative {
    [DllImport("D3D11Plugins")]
    public static extern IntPtr GetBeginFrameEventFunction();
    [DllImport("D3D11Plugins")]
    public static extern IntPtr GetEndFrameEventFunction();
    [DllImport("D3D11Plugins")]
    public static extern IntPtr GetBeginTimerEventFunction();
    [DllImport("D3D11Plugins")]
    public static extern IntPtr GetEndTimerEventFunction();
    [DllImport("D3D11Plugins")]
    public static extern int CreateTimer();
    [DllImport("D3D11Plugins")]
    public static extern void ReleaseTimers();
    [DllImport("D3D11Plugins")]
    public static extern float GetTimerDuration(int index);
    [DllImport("D3D11Plugins")]
    public static extern float GetFrameTimerDuration();
}

public class D3D11GPUTimer {
    public void Begin() {
        if (internalId == -1 || beginWasCalled) {
            return;
        }
        beginWasCalled = true;
        GL.IssuePluginEvent(D3D11PluginsNative.GetBeginTimerEventFunction(), internalId);
    }

    public void End() {
        if (internalId == -1 || !beginWasCalled) {
            return;
        }
        beginWasCalled = false;
        GL.IssuePluginEvent(D3D11PluginsNative.GetEndTimerEventFunction(), internalId);
    }

    public float Duration {
        get {
            return D3D11PluginsNative.GetTimerDuration(internalId);
        }
    }

    public int internalId = -1;
    private bool beginWasCalled = false;
}

public class D3D11Plugins : MonoBehaviour {
    private static D3D11Plugins instance;
    private static Queue<D3D11GPUTimer> timerCreateQueue = new Queue<D3D11GPUTimer>();

    private static void CreateInstance() {
        if (instance != null) {
            return;
        }
        var gameObject = new GameObject("D3D11Plugins");
        instance = gameObject.AddComponent<D3D11Plugins>();
        GameObject.DontDestroyOnLoad(gameObject);
    }

    private void OnDestroy() {
        D3D11PluginsNative.ReleaseTimers();
    }

    private void LateUpdate() {
        GL.IssuePluginEvent(D3D11PluginsNative.GetEndFrameEventFunction(), 0);
        while (timerCreateQueue.Count > 0) {
            var timer = timerCreateQueue.Dequeue();
            timer.internalId = D3D11PluginsNative.CreateTimer();
        }
        GL.IssuePluginEvent(D3D11PluginsNative.GetBeginFrameEventFunction(), 0);
    }

    public static void Initialize() {
        CreateInstance();
    }

    public static D3D11GPUTimer CreateGPUTimer() {
        CreateInstance();
        var timer = new D3D11GPUTimer();
        timerCreateQueue.Enqueue(timer);
        return timer;
    }

    public static float FrameDuration {
        get {
            return D3D11PluginsNative.GetFrameTimerDuration();
        }
    }
}
