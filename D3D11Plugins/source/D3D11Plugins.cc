#include "GPUTiming.h"
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D11.h"

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces* s_UnityInterfaces = nullptr;
static IUnityGraphics* s_Graphics = nullptr;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;

static ID3D11Device* s_Device = nullptr;
static GPUTiming s_GPUTiming;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces) {
    s_UnityInterfaces = unityInterfaces;
    s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
    s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
    s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
    if (eventType == kUnityGfxDeviceEventInitialize) {
        s_DeviceType = s_Graphics->GetRenderer();

        if (s_DeviceType == kUnityGfxRendererD3D11) {
            IUnityGraphicsD3D11* d3d = s_UnityInterfaces->Get<IUnityGraphicsD3D11>();
            s_Device = d3d->GetDevice();
            s_GPUTiming.Initialize(s_Device);
        }
    }

    if (eventType == kUnityGfxDeviceEventShutdown) {
        s_GPUTiming.Release();
        s_Device = NULL;
        s_DeviceType = kUnityGfxRendererNull;
    }
}

static void UNITY_INTERFACE_API OnBeginFrameEvent(int eventID) {
    if (s_Device == nullptr) {
        return;
    }
    s_GPUTiming.BeginFrame();
}

static void UNITY_INTERFACE_API OnEndFrameEvent(int eventID) {
    if (s_Device == nullptr) {
        return;
    }
    s_GPUTiming.EndFrame();
}

static void UNITY_INTERFACE_API OnBeginTimerEvent(int eventID) {
    if (s_Device == nullptr) {
        return;
    }
    s_GPUTiming.BeginTimer(eventID);
}

static void UNITY_INTERFACE_API OnEndTimerEvent(int eventID) {
    if (s_Device == nullptr) {
        return;
    }
    s_GPUTiming.EndTimer(eventID);
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetBeginFrameEventFunction() {
    return OnBeginFrameEvent;
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetEndFrameEventFunction() {
    return OnEndFrameEvent;
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetBeginTimerEventFunction() {
    return OnBeginTimerEvent;
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetEndTimerEventFunction() {
    return OnEndTimerEvent;
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API CreateTimer() {
    if (s_Device == nullptr) {
        return -1;
    }
    return s_GPUTiming.CreateTimer();
}

extern "C" float UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetTimerDuration(int index) {
    if (s_Device == nullptr) {
        return 0.0f;
    }
    return s_GPUTiming.GetTimerDuration(index);
}

extern "C" float UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetFrameTimerDuration() {
    if (s_Device == nullptr) {
        return 0.0f;
    }
    return s_GPUTiming.GetFrameTimerDuration();
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ReleaseTimers() {
    s_GPUTiming.ReleaseTimers();
}
