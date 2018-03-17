#pragma once

#include <d3d11.h>
#include <vector>

// GPUTimer
struct GPUTimer {
    bool Create(ID3D11Device* device) {
        D3D11_QUERY_DESC queryDesc = {D3D11_QUERY_TIMESTAMP, 0};
        for (int i = 0; i < 2; ++i) {
            if (FAILED(device->CreateQuery(&queryDesc, &m_StartQuery[i]))) {
                return false;
            }
            if (FAILED(device->CreateQuery(&queryDesc, &m_EndQuery[i]))) {
                return false;
            }
        }
        return true;
    }

    void Release() {
        for (int i = 0; i < 2; ++i) {
            if (m_StartQuery[i] != nullptr) {
                m_StartQuery[i]->Release();
                m_StartQuery[i] = nullptr;
            }
            if (m_EndQuery[i] != nullptr) {
                m_EndQuery[i]->Release();
                m_EndQuery[i] = nullptr;
            }
        }
    }

    void Begin(ID3D11DeviceContext* deviceContext, int frameIndex) {
        deviceContext->End(m_StartQuery[frameIndex]);
    }

    void End(ID3D11DeviceContext* deviceContext, int frameIndex) {
        deviceContext->End(m_EndQuery[frameIndex]);
    }

    void Update(ID3D11DeviceContext* deviceContext, int frameIndex, uint64_t frequency) {
        uint64_t timestampStart = 0;
        if (deviceContext->GetData(m_StartQuery[frameIndex], &timestampStart, sizeof(uint64_t), 0) != S_OK) {
            return;
        }
        uint64_t timestampEnd = 0;
        if (deviceContext->GetData(m_EndQuery[frameIndex], &timestampEnd, sizeof(uint64_t), 0) != S_OK) {
            return;
        }
        m_Time = (float)((double)(timestampEnd - timestampStart) / (double)frequency);
    }

    float GetDuration() const {
        return m_Time;
    }

    ID3D11Query* m_StartQuery[2] = {};
    ID3D11Query* m_EndQuery[2] = {};
    float m_Time = -1.0f;
};

// GPUDisjoint
struct GPUDisjoint {
    bool Create(ID3D11Device* device) {
        D3D11_QUERY_DESC queryDesc = {D3D11_QUERY_TIMESTAMP_DISJOINT, 0};
        for (int i = 0; i < 2; ++i) {
            if (FAILED(device->CreateQuery(&queryDesc, &m_Query[i]))) {
                return false;
            }
        }
        return true;
    }

    void Release() {
        for (int i = 0; i < 2; ++i) {
            if (m_Query[i] != nullptr) {
                m_Query[i]->Release();
                m_Query[i] = nullptr;
            }
        }
    }

    void Begin(ID3D11DeviceContext* deviceContext, int frameIndex) {
        deviceContext->Begin(m_Query[frameIndex]);
    }

    void End(ID3D11DeviceContext* deviceContext, int frameIndex) {
        deviceContext->End(m_Query[frameIndex]);
    }

    bool Update(ID3D11DeviceContext* deviceContext, int frameIndex, uint64_t* frequency) {
        if (frequency == nullptr) {
            return false;
        }
        // Wait for query
        while (deviceContext->GetData(m_Query[frameIndex], NULL, 0, 0) == S_FALSE) {
            Sleep(1);
        }
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timestampDisjoint = {};
        // Get data
        auto result = deviceContext->GetData(m_Query[frameIndex], &timestampDisjoint, sizeof(timestampDisjoint), 0);
        if (result != S_OK) {
            return false;
        }
        // Check if frame is disjointed
        if (timestampDisjoint.Disjoint) {
            return false;
        }
        *frequency = timestampDisjoint.Frequency;
        return true;
    }

    ID3D11Query* m_Query[2] = {};
};

// GPUTiming
struct GPUTiming {
    bool Initialize(ID3D11Device* device) {
        m_Device = device;
        // Get context
        m_Device->GetImmediateContext(&m_DeviceContext);
        // Create disjoint query
        if (!m_GPUDisjoint.Create(m_Device)) {
            return false;
        }
        // Get default frame timer
        frameTimer.Create(m_Device);
        return true;
    }

    void ReleaseTimers() {
        EndFrame();
        // Release timers
        for (auto& gpuTimer : m_GPUTimers) {
            gpuTimer.Release();
        }
        m_GPUTimers.clear();
    }

    void Release() {
        ReleaseTimers();
        // Release frame timer
        frameTimer.Release();
        // Release disjoint
        m_GPUDisjoint.Release();
        // Release context
        if (m_DeviceContext != nullptr) {
            m_DeviceContext->Release();
            m_DeviceContext = nullptr;
        }
    }

    int CreateTimer() {
        GPUTimer timer;
        if (!timer.Create(m_Device)) {
            return -1;
        }
        m_GPUTimers.push_back(timer);
        return (int)m_GPUTimers.size() - 1;
    }

    void BeginFrame() {
        if (m_BeginFrameCalled) {
            return;
        }
        m_BeginFrameCalled = true;
        m_GPUDisjoint.Begin(m_DeviceContext, m_FrameIndex);
        BeginTimer(0);
    }

    void EndFrame() {
        if (!m_BeginFrameCalled) {
            return;
        }
        m_BeginFrameCalled = false;
        EndTimer(0);
        m_GPUDisjoint.End(m_DeviceContext, m_FrameIndex);
        // Update frame
        ++m_FrameCounter;
        int activeFrameIndex = m_FrameIndex;
        m_FrameIndex = !m_FrameIndex;
        if (m_FrameCounter < 2) {
            return;
        }
        // Check if disjointed and quit
        uint64_t frequency = 0;
        if (!m_GPUDisjoint.Update(m_DeviceContext, activeFrameIndex, &frequency)) {
            return;
        }
        for (auto& gpuTimer : m_GPUTimers) {
            gpuTimer.Update(m_DeviceContext, activeFrameIndex, frequency);
        }
    }

    void BeginTimer(int index) {
        if (index < 0 || index >= (int)m_GPUTimers.size()) {
            return;
        }
        m_GPUTimers[index].Begin(m_DeviceContext, m_FrameIndex);
    }

    void EndTimer(int index) {
        if (index < 0 || index >= (int)m_GPUTimers.size()) {
            return;
        }
        m_GPUTimers[index].End(m_DeviceContext, m_FrameIndex);
    }

    float GetTimerDuration(int index) {
        if (index < 0 || index >= (int)m_GPUTimers.size()) {
            return -1.0f;
        }
        return m_GPUTimers[index].GetDuration();
    }

    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_DeviceContext = nullptr;
    GPUDisjoint m_GPUDisjoint;
    GPUTimer frameTimer;
    std::vector<GPUTimer> m_GPUTimers;
    bool m_BeginFrameCalled = false;
    int m_FrameIndex = 0;
    uint64_t m_FrameCounter = 0;
};
