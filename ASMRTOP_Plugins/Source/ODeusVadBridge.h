#pragma once
/*
    ODeusVadBridge.h
    
    Pure ASIO Link Pro protocol bridge.
    Communicates with asiovadpro.sys via DeviceIoControl using the
    Link Pro IOCTL protocol (magic 0x0131CA08).
    
    Protocol (reverse-engineered & verified 2026-03-22/23):
    ─────────────────────────────────────────────────────
    Magic:     0x0131CA08 (ASIO Link Pro — suppresses anti-piracy tone)
    Method:    METHOD_BUFFERED
    Access:    FILE_ANY_ACCESS
    
    IOCTL Code    Function     Input     Output
    ─────────────────────────────────────────────────────
    0x9C402410    Attach       12 bytes  12 bytes
    0x9C402414    Detach       12 bytes  0 bytes
    0x9C402420    Start        12 bytes  0 bytes
    0x9C402424    Stop         12 bytes  0 bytes
    0x9C402428    Transfer     17740     17740
    
    Transfer buffer layout (17740 bytes total):
      [0x000..0x07F]  Header (128 bytes)
      [0x080..0x407F] Render block  (128 frames × 64ch × 16bit = 16384 bytes)
      [0x4080..0x454B] Capture block (driver writes here)
    
    4 groups of 16 channels = 64 total channels.
    Native sample rate: 48000 Hz.
    
    IMPORTANT: asiovadpro.sys does NOT support overlapped I/O.
    All DeviceIoControl calls MUST be synchronous. FILE_FLAG_OVERLAPPED
    or CancelIoEx will cause BSOD.
*/

#include <JuceHeader.h>

#ifdef JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>

#pragma comment(lib, "setupapi.lib")

// KSCATEGORY_AUDIO
DEFINE_GUID(GUID_KSCATEGORY_AUDIO_BRIDGE,
    0x6994AD04, 0x93EF, 0x11D0, 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);
#endif

namespace Asmrtop
{

// ── IOCTL codes ──
constexpr DWORD IOCTL_VAD_ATTACH         = 0x9C402410;
constexpr DWORD IOCTL_VAD_DETACH         = 0x9C402414;
constexpr DWORD IOCTL_VAD_START_TRANSFER = 0x9C402420;
constexpr DWORD IOCTL_VAD_STOP_TRANSFER  = 0x9C402424;
constexpr DWORD IOCTL_VAD_TRANSFER_DATA  = 0x9C402428;

// ── Link Pro protocol constants ──
static constexpr DWORD  VAD_MAGIC           = 0x0131CA08;
static constexpr DWORD  VAD_XFER_BUF_SIZE   = 17740;
static constexpr int    VAD_HEADER_SIZE      = 128;
static constexpr int    VAD_RENDER_OFFSET    = 128;
static constexpr int    VAD_CAPTURE_OFFSET   = 16512;
static constexpr int    VAD_FRAMES_PER_XFER  = 128;
static constexpr int    VAD_NUM_CHANNELS     = 64;   // 4 groups × 16ch
static constexpr int    VAD_SAMPLE_RATE      = 48000;
static constexpr int    VAD_BIT_DEPTH        = 16;
static constexpr int    VAD_AUDIO_DATA_SIZE  = 16384; // 128 × 64 × 2

// ── IOCTL buffer (12 bytes) ──
#pragma pack(push, 1)
struct VadIoctlBuffer
{
    DWORD magic;   // VAD_MAGIC
    DWORD param1;  // clientId / processTag
    DWORD param2;  // 0
};
#pragma pack(pop)

class ODeusVadBridge
{
public:
    ODeusVadBridge() = default;
    ~ODeusVadBridge() { close(); }
    
    // ── Open device ──
    bool open(int deviceIndex = 0)
    {
#ifdef JUCE_WINDOWS
        if (hDevice != INVALID_HANDLE_VALUE)
            return true;
        
        juce::String symLink = findDevice(deviceIndex);
        if (symLink.isEmpty())
        {
            lastError = "Link Pro device #" + juce::String(deviceIndex) + " not found.";
            return false;
        }
        
        // Synchronous mode only — asiovadpro.sys does NOT support overlapped I/O
        hDevice = CreateFileA(
            symLink.toRawUTF8(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING,
            0,    // Synchronous
            NULL);
        
        if (hDevice == INVALID_HANDLE_VALUE)
        {
            lastError = "CreateFile failed: " + juce::String((int)GetLastError());
            return false;
        }
        
        devicePath = symLink;
        DBG("ODeusVadBridge: Opened device: " + devicePath);
        return true;
#else
        return false;
#endif
    }
    
    // ── Attach ──
    bool attach()
    {
#ifdef JUCE_WINDOWS
        if (hDevice == INVALID_HANDLE_VALUE) return false;
        
        clientId = (DWORD)GetCurrentProcessId();
        if (clientId == 0) clientId = 1;
        
        VadIoctlBuffer io = { VAD_MAGIC, clientId, 0 };
        DWORD ret = 0;
        BOOL ok = DeviceIoControl(hDevice, IOCTL_VAD_ATTACH,
            &io, sizeof(io), &io, sizeof(io), &ret, NULL);
        
        if (!ok || ret < 4)
        {
            lastError = "Attach failed: " + juce::String((int)GetLastError());
            return false;
        }
        
        xferBuf.assign(VAD_XFER_BUF_SIZE, 0);
        attached = true;
        DBG("ODeusVadBridge: Attached (Link Pro)");
        return true;
#else
        return false;
#endif
    }
    
    // ── Start transfer ──
    bool startTransfer()
    {
#ifdef JUCE_WINDOWS
        if (hDevice == INVALID_HANDLE_VALUE || !attached) return false;
        
        VadIoctlBuffer io = { VAD_MAGIC, clientId, 0 };
        DWORD ret = 0;
        BOOL ok = DeviceIoControl(hDevice, IOCTL_VAD_START_TRANSFER,
            &io, sizeof(io), NULL, 0, &ret, NULL);
        
        if (!ok)
        {
            lastError = "Start failed: " + juce::String((int)GetLastError());
            return false;
        }
        
        transferring = true;
        DBG("ODeusVadBridge: Transfer started");
        return true;
#else
        return false;
#endif
    }
    
    // ── Stop transfer ──
    bool stopTransfer()
    {
#ifdef JUCE_WINDOWS
        if (hDevice == INVALID_HANDLE_VALUE) return false;
        
        VadIoctlBuffer io = { VAD_MAGIC, clientId, 0 };
        DWORD ret = 0;
        DeviceIoControl(hDevice, IOCTL_VAD_STOP_TRANSFER,
            &io, sizeof(io), NULL, 0, &ret, NULL);
        transferring = false;
        DBG("ODeusVadBridge: Transfer stopped");
        return true;
#else
        return false;
#endif
    }
    
    // ── Detach ──
    bool detach()
    {
#ifdef JUCE_WINDOWS
        if (hDevice == INVALID_HANDLE_VALUE || !attached) return false;
        
        VadIoctlBuffer io = { VAD_MAGIC, clientId, 0 };
        DWORD ret = 0;
        DeviceIoControl(hDevice, IOCTL_VAD_DETACH,
            &io, sizeof(io), NULL, 0, &ret, NULL);
        attached = false;
        DBG("ODeusVadBridge: Detached");
        return true;
#else
        return false;
#endif
    }
    
    // ── Read captured audio (driver → plugin) ──
    // Returns number of frames read, or 0 on failure.
    int readCapture(float* outL, float* outR)
    {
        if (!transferring || xferBuf.size() != VAD_XFER_BUF_SIZE) return 0;
        
        // Prepare header
        std::fill(xferBuf.begin(), xferBuf.end(), 0);
        *(DWORD*)(xferBuf.data() + 0)  = VAD_HEADER_SIZE;   // 128
        *(DWORD*)(xferBuf.data() + 4)  = VAD_AUDIO_DATA_SIZE; // 16384
        *(DWORD*)(xferBuf.data() + 8)  = 0;
        *(DWORD*)(xferBuf.data() + 12) = 1;
        *(DWORD*)(xferBuf.data() + 28) = VAD_SAMPLE_RATE;    // 48000
        
        // Transfer
        DWORD transferred = 0;
        BOOL ok = DeviceIoControl(hDevice, IOCTL_VAD_TRANSFER_DATA,
            xferBuf.data(), VAD_XFER_BUF_SIZE,
            xferBuf.data(), VAD_XFER_BUF_SIZE,
            &transferred, NULL);
        
        if (!ok) return 0;
        
        // Extract stereo from capture block (ch0=L, ch1=R from 64-channel interleaved)
        // Try capture offset first, fall back to render offset
        SHORT* pcmCapture = (SHORT*)(xferBuf.data() + VAD_CAPTURE_OFFSET);
        SHORT* pcmRender  = (SHORT*)(xferBuf.data() + VAD_RENDER_OFFSET);
        
        // Check which block has audio
        int nzCapture = 0, nzRender = 0;
        for (int i = 0; i < VAD_FRAMES_PER_XFER * VAD_NUM_CHANNELS && i < 1024; ++i)
        {
            if (pcmCapture[i] != 0) nzCapture++;
            if (pcmRender[i] != 0) nzRender++;
        }
        SHORT* pcm = (nzCapture > 0) ? pcmCapture : pcmRender;
        
        // Convert to float (stereo: ch0 + ch1)
        for (int i = 0; i < VAD_FRAMES_PER_XFER; ++i)
        {
            if (outL) outL[i] = (float)pcm[i * VAD_NUM_CHANNELS + 0] / 32768.0f;
            if (outR) outR[i] = (float)pcm[i * VAD_NUM_CHANNELS + 1] / 32768.0f;
        }
        
        return VAD_FRAMES_PER_XFER;
    }
    
    // ── Write render audio (plugin → driver) ──
    int writeRender(const float* inL, const float* inR)
    {
        if (!transferring || xferBuf.size() != VAD_XFER_BUF_SIZE) return 0;
        
        // Prepare header
        std::fill(xferBuf.begin(), xferBuf.end(), 0);
        *(DWORD*)(xferBuf.data() + 0)  = VAD_HEADER_SIZE;
        *(DWORD*)(xferBuf.data() + 4)  = VAD_AUDIO_DATA_SIZE;
        *(DWORD*)(xferBuf.data() + 8)  = 0;
        *(DWORD*)(xferBuf.data() + 12) = 1;
        *(DWORD*)(xferBuf.data() + 28) = VAD_SAMPLE_RATE;
        
        // Pack stereo into 64-channel interleaved render block
        SHORT* pcm = (SHORT*)(xferBuf.data() + VAD_RENDER_OFFSET);
        for (int i = 0; i < VAD_FRAMES_PER_XFER; ++i)
        {
            float lVal = juce::jlimit(-1.0f, 1.0f, inL ? inL[i] : 0.0f);
            float rVal = juce::jlimit(-1.0f, 1.0f, inR ? inR[i] : 0.0f);
            pcm[i * VAD_NUM_CHANNELS + 0] = (SHORT)(lVal * 32767.0f);
            pcm[i * VAD_NUM_CHANNELS + 1] = (SHORT)(rVal * 32767.0f);
        }
        
        // Transfer
        DWORD transferred = 0;
        BOOL ok = DeviceIoControl(hDevice, IOCTL_VAD_TRANSFER_DATA,
            xferBuf.data(), VAD_XFER_BUF_SIZE,
            xferBuf.data(), VAD_XFER_BUF_SIZE,
            &transferred, NULL);
        
        return ok ? VAD_FRAMES_PER_XFER : 0;
    }
    
    // ── Close everything ──
    void close()
    {
#ifdef JUCE_WINDOWS
        if (transferring) stopTransfer();
        if (attached) detach();
        if (hDevice != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hDevice);
            hDevice = INVALID_HANDLE_VALUE;
        }
#endif
    }
    
    // ── Queries ──
    bool isOpen() const { return hDevice != INVALID_HANDLE_VALUE; }
    bool isAttached() const { return attached; }
    bool isTransferring() const { return transferring; }
    juce::String getLastError() const { return lastError; }
    juce::String getDevicePath() const { return devicePath; }
    
    static bool isDriverInstalled() { return !findDevice(0).isEmpty(); }
    
    static juce::StringArray findAllDevicePaths()
    {
        juce::StringArray paths;
        for (auto& dev : enumerateDevices())
            paths.add(dev.interfacePath);
        return paths;
    }

private:
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    bool attached = false;
    bool transferring = false;
    DWORD clientId = 0;
    juce::String lastError;
    juce::String devicePath;
    std::vector<BYTE> xferBuf;
    
    // ── Device discovery ──
    struct DeviceInfo {
        juce::String interfacePath;
        juce::String friendlyName;
        juce::String hwId;
    };
    
    static std::vector<DeviceInfo> enumerateDevices()
    {
        std::vector<DeviceInfo> results;
#ifdef JUCE_WINDOWS
        HDEVINFO hDevInfo = SetupDiGetClassDevsA(
            &GUID_KSCATEGORY_AUDIO_BRIDGE, NULL, NULL,
            DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
        
        if (hDevInfo == INVALID_HANDLE_VALUE) return results;
        
        SP_DEVICE_INTERFACE_DATA ifData;
        ifData.cbSize = sizeof(ifData);
        
        // Collect all matching wave interfaces
        struct RawEntry {
            juce::String path, friendlyName, hwId;
            int devInst;
            bool isWave;
        };
        std::vector<RawEntry> raw;
        
        for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL,
            &GUID_KSCATEGORY_AUDIO_BRIDGE, i, &ifData); i++)
        {
            DWORD sz = 0;
            SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifData, NULL, 0, &sz, NULL);
            if (sz == 0) continue;
            
            auto* detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(sz);
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
            SP_DEVINFO_DATA diData;
            diData.cbSize = sizeof(SP_DEVINFO_DATA);
            
            if (SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifData, detail, sz, NULL, &diData))
            {
                juce::String path(detail->DevicePath);
                
                char hwBuf[512] = {};
                DWORD t = 0, s = 0;
                SetupDiGetDeviceRegistryPropertyA(hDevInfo, &diData, SPDRP_HARDWAREID,
                    &t, (BYTE*)hwBuf, sizeof(hwBuf), &s);
                juce::String hwId(hwBuf);
                
                char descBuf[256] = {};
                s = 0;
                SetupDiGetDeviceRegistryPropertyA(hDevInfo, &diData, SPDRP_FRIENDLYNAME,
                    &t, (BYTE*)descBuf, sizeof(descBuf), &s);
                if (s == 0)
                    SetupDiGetDeviceRegistryPropertyA(hDevInfo, &diData, SPDRP_DEVICEDESC,
                        &t, (BYTE*)descBuf, sizeof(descBuf), &s);
                juce::String name(descBuf);
                
                bool match = hwId.containsIgnoreCase("asiovadpro") ||
                             hwId.containsIgnoreCase("wdm2vst") ||
                             hwId.containsIgnoreCase("asiolink") ||
                             hwId.containsIgnoreCase("virtualaudio") ||
                             name.containsIgnoreCase("virtual audio") ||
                             name.containsIgnoreCase("asiovadpro") ||
                             name.containsIgnoreCase("wdm2vst") ||
                             name.containsIgnoreCase("Virtual Bridge") ||
                             name.containsIgnoreCase("ASMR Bridge");
                
                if (match)
                    raw.push_back({ path, name, hwId, (int)diData.DevInst,
                                    path.containsIgnoreCase("wave") });
            }
            free(detail);
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
        
        // Deduplicate by DevInst, prefer wave interfaces
        std::vector<int> seenDev;
        for (auto& e : raw)
        {
            int idx = -1;
            for (int j = 0; j < (int)seenDev.size(); j++)
                if (seenDev[j] == e.devInst) { idx = j; break; }
            
            if (idx < 0)
            {
                seenDev.push_back(e.devInst);
                results.push_back({ e.path, e.friendlyName, e.hwId });
            }
            else if (e.isWave)
            {
                results[idx].interfacePath = e.path;
            }
        }
#endif
        return results;
    }
    
    static juce::String findDevice(int index = 0)
    {
        auto devs = enumerateDevices();
        if (index >= 0 && index < (int)devs.size())
            return devs[index].interfacePath;
        return {};
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ODeusVadBridge)
};

} // namespace Asmrtop
