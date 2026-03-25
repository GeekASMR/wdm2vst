#pragma once
#include <JuceHeader.h>
#include "PluginBranding.h"

#ifdef JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Asmrtop
{

constexpr int IPC_RING_SIZE = 131072;
constexpr int IPC_RING_MASK = IPC_RING_SIZE - 1;

struct IpcAudioBuffer
{
    std::atomic<uint32_t> writePos;
    std::atomic<uint32_t> readPos;
    float ringL[IPC_RING_SIZE];
    float ringR[IPC_RING_SIZE];
};

class SharedMemoryBridge
{
public:
    SharedMemoryBridge(const juce::String& baseName, const juce::String& direction, int channelId)
        : m_baseName(baseName), m_direction(direction), m_channelId(channelId)
    {
        connect();
    }

    void connect()
    {
#ifdef JUCE_WINDOWS
        if (pBuf) return;

        juce::String primaryName = m_baseName + "_" + m_direction + "_" + juce::String(m_channelId);
        juce::String altBaseName = (m_baseName == "AsmrtopWDM") ? "VirtualAudioWDM" : "AsmrtopWDM";
        juce::String fallbackName = altBaseName + "_" + m_direction + "_" + juce::String(m_channelId);
        
        // Phase 1: Try to OPEN an existing mapping (driver-created) for both brand names
        for (auto& nameBase : { primaryName, fallbackName })
        {
            if (pBuf) break;
            for (auto& prefix : { juce::String("Global\\"), juce::String("Local\\") })
            {
                juce::String mappingName = prefix + nameBase;
                
                hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mappingName.toRawUTF8());
                if (hMapFile != NULL)
                {
                    pBuf = (IpcAudioBuffer*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(IpcAudioBuffer));
                    if (pBuf) break; // Successfully attached
                    CloseHandle(hMapFile);
                    hMapFile = NULL;
                }
            }
        }

        // Phase 2: Removed! We no longer unilaterally create the FileMapping.
        // By relying purely on OpenFileMappingA, we force the Kernel Driver 
        // to be the exclusive owner/creator of the memory mapped section.
        // This permanently fixes the ZwOpenSection STATUS_INVALID_PARAMETER bug
        // that caused "No sound" when VST was selected before the browser starts.
        if (!pBuf) {
            // Do absolutely nothing. The 300ms heartbeat will retry Opening it!
        }
#endif
    }

    ~SharedMemoryBridge()
    {
#ifdef JUCE_WINDOWS
        if (pBuf) UnmapViewOfFile(pBuf);
        if (hMapFile) CloseHandle(hMapFile);
#endif
    }

    bool isConnected() const { return pBuf != nullptr; }

    // Probe if a kernel driver is installed for this brand.
    // Checks ALL channels (0-3) and both directions (PLAY/REC) to detect the driver.
    // Returns the display name for the requested channelId, or empty string if driver not found.
    static juce::String getIpcChannelName(const juce::String& direction, int channelId)
    {
#ifdef JUCE_WINDOWS
        const char* playNamesAsmr[4] = { "ASMR Audio 1/2", "ASMR Audio 3/4", "ASMR Audio 5/6", "ASMR Audio 7/8" };
        const char* recNamesAsmr[4]  = { "ASMR Mic 1/2", "ASMR Mic 3/4", "ASMR Mic 5/6", "ASMR Mic 7/8" };
        const char* playNamesPub[4]  = { "Virtual 1/2", "Virtual 3/4", "Virtual 5/6", "Virtual 7/8" };
        const char* recNamesPub[4]   = { "Mic 1/2", "Mic 3/4", "Mic 5/6", "Mic 7/8" };

        juce::String baseName = juce::String(PLUGIN_IPC_BASE_NAME);
        if (baseName == "AsmrtopWDM") {
            const char** names = (direction == "PLAY") ? playNamesAsmr : recNamesAsmr;
            return juce::String(names[channelId]);
        } else {
            const char** names = (direction == "PLAY") ? playNamesPub : recNamesPub;
            return juce::String(names[channelId]);
        }
#endif
        return {};
    }
    IpcAudioBuffer* getBuffer() { return pBuf; }

    // Writing to Shared Memory
    void write(const float* inL, const float* inR, int numSamples)
    {
        if (!pBuf) return;
        uint32_t w = pBuf->writePos.load(std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            pBuf->ringL[(w + i) & IPC_RING_MASK] = inL ? inL[i] : 0.0f;
            pBuf->ringR[(w + i) & IPC_RING_MASK] = inR ? inR[i] : 0.0f;
        }
        pBuf->writePos.store(w + numSamples, std::memory_order_release);
    }

    // Reading from Shared Memory (Zero latency identical block sync)
    // Always reads exactly the newest samples sent by writer
    void readLastSamples(float* outL, float* outR, int numSamples)
    {
        if (!pBuf)
        {
            if (outL) juce::FloatVectorOperations::clear(outL, numSamples);
            if (outR) juce::FloatVectorOperations::clear(outR, numSamples);
            return;
        }

        uint32_t w = pBuf->writePos.load(std::memory_order_acquire);
        uint32_t r = pBuf->readPos.load(std::memory_order_relaxed);
        int32_t available = (int32_t)(w - r);

        if (available < 0 || available > IPC_RING_SIZE) {
            r = w;
            available = 0;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            if (available > 0)
            {
                if (outL) outL[i] = pBuf->ringL[r & IPC_RING_MASK];
                if (outR) outR[i] = pBuf->ringR[r & IPC_RING_MASK];
                r++;
                available--;
            }
            else
            {
                if (outL) outL[i] = 0.0f;
                if (outR) outR[i] = 0.0f;
            }
        }
        pBuf->readPos.store(r, std::memory_order_release);
    }

private:
    IpcAudioBuffer* pBuf = nullptr;
    juce::String m_baseName;
    juce::String m_direction;
    int m_channelId;
    
#ifdef JUCE_WINDOWS
    HANDLE hMapFile = NULL;
#endif
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedMemoryBridge)
};

}
