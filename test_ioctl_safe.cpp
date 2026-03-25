/*
    test_ioctl_safe.cpp
    
    安全的 IOCTL 协议测试工具。
    基于 asiovadpro.sys 反汇编分析，使用正确的缓冲区格式。
    
    每一步都有确认提示，遇到问题立刻停止。
    
    发现的协议参数:
      - Transfer 缓冲区: 0x188 (392) 字节 = 8字节header + 24帧 × 8通道 × 16bit
      - Attach param1: 客户端ID (非零! 驱动用0表示slot空闲)
      - Attach param2: 模式 (0=双slot, 1=slot1, 2=slot2)
      - 驱动检查: 所有 NumDevices 个 session slot 必须非空才处理IOCTL
*/

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <initguid.h>
#pragma comment(lib, "setupapi.lib")

DEFINE_GUID(GUID_KS_AUDIO, 0x6994AD04, 0x93EF, 0x11D0, 
    0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

#define IOCTL_ATTACH   0x9C402410
#define IOCTL_DETACH   0x9C402414
#define IOCTL_START    0x9C402420
#define IOCTL_TRANSFER 0x9C402428
#define MAGIC          0x0131CA08

#define XFER_BUF_SIZE  0x188    // 392 bytes - exact driver expectation
#define XFER_HEADER    8
#define XFER_PCM       384     // 24 frames * 8 channels * 2 bytes
#define NUM_CH         8
#define NUM_FRAMES     24

struct IoctlBuf { DWORD magic, p1, p2; };

// 枚举所有 KSCATEGORY_AUDIO 设备
void listAllDevices()
{
    printf("\n=== Enumerating KSCATEGORY_AUDIO devices ===\n\n");
    
    HDEVINFO h = SetupDiGetClassDevsA(&GUID_KS_AUDIO, NULL, NULL, 
        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (h == INVALID_HANDLE_VALUE) { printf("FAIL: SetupDi\n"); return; }
    
    SP_DEVICE_INTERFACE_DATA ifd;
    ifd.cbSize = sizeof(ifd);
    
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(h, NULL, &GUID_KS_AUDIO, i, &ifd); i++)
    {
        DWORD sz = 0;
        SetupDiGetDeviceInterfaceDetailA(h, &ifd, NULL, 0, &sz, NULL);
        auto* d = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(sz);
        d->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        
        SP_DEVINFO_DATA devInfo;
        devInfo.cbSize = sizeof(devInfo);
        
        if (SetupDiGetDeviceInterfaceDetailA(h, &ifd, d, sz, NULL, &devInfo))
        {
            // Get hardware ID
            char hwId[512] = {};
            SetupDiGetDeviceRegistryPropertyA(h, &devInfo, SPDRP_HARDWAREID,
                NULL, (BYTE*)hwId, sizeof(hwId), NULL);
            
            // Get friendly name
            char name[256] = {};
            DWORD nameSize = 0;
            SetupDiGetDeviceRegistryPropertyA(h, &devInfo, SPDRP_FRIENDLYNAME,
                NULL, (BYTE*)name, sizeof(name), &nameSize);
            if (nameSize == 0)
                SetupDiGetDeviceRegistryPropertyA(h, &devInfo, SPDRP_DEVICEDESC,
                    NULL, (BYTE*)name, sizeof(name), NULL);
            
            bool isVad = (strstr(hwId, "asiovadpro") || strstr(hwId, "wdm2vst") ||
                         strstr(name, "ASIOVADPRO") || strstr(name, "Virtual Audio"));
            
            printf("[%2d] %s%s\n", i, isVad ? "*** " : "    ", name);
            printf("     HwID: %s\n", hwId);
            printf("     Path: %.80s...\n\n", d->DevicePath);
        }
        free(d);
    }
    SetupDiDestroyDeviceInfoList(h);
}

// 尝试打开特定设备
HANDLE openDevice(int index)
{
    HDEVINFO h = SetupDiGetClassDevsA(&GUID_KS_AUDIO, NULL, NULL,
        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (h == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
    
    SP_DEVICE_INTERFACE_DATA ifd;
    ifd.cbSize = sizeof(ifd);
    
    HANDLE result = INVALID_HANDLE_VALUE;
    
    if (SetupDiEnumDeviceInterfaces(h, NULL, &GUID_KS_AUDIO, index, &ifd))
    {
        DWORD sz = 0;
        SetupDiGetDeviceInterfaceDetailA(h, &ifd, NULL, 0, &sz, NULL);
        auto* d = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(sz);
        d->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        
        if (SetupDiGetDeviceInterfaceDetailA(h, &ifd, d, sz, NULL, NULL))
        {
            printf("Opening: %s\n", d->DevicePath);
            
            // SYNCHRONOUS mode - asiovadpro.sys requires this
            result = CreateFileA(d->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, 0, NULL);
            
            if (result == INVALID_HANDLE_VALUE)
                printf("  FAIL: CreateFile error %d\n", GetLastError());
            else
                printf("  OK: handle=0x%p\n", result);
        }
        free(d);
    }
    
    SetupDiDestroyDeviceInfoList(h);
    return result;
}

// 检查注册表配置
void checkRegistry()
{
    printf("\n=== Registry Configuration ===\n\n");
    
    HKEY key;
    LONG r = RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
        "SOFTWARE\\ODeusAudio\\AsioLinkPro", 0, KEY_READ, &key);
    
    if (r != ERROR_SUCCESS) {
        printf("MISSING: HKLM\\SOFTWARE\\ODeusAudio\\AsioLinkPro\n");
        printf("  This is REQUIRED for asiovadpro.sys to function!\n");
        return;
    }
    
    DWORD numDev = 0, numCh = 0, sz = sizeof(DWORD);
    RegQueryValueExA(key, "NumDevices", NULL, NULL, (BYTE*)&numDev, &sz);
    sz = sizeof(DWORD);
    RegQueryValueExA(key, "NumChannels", NULL, NULL, (BYTE*)&numCh, &sz);
    
    printf("NumDevices  = %d\n", numDev);
    printf("NumChannels = %d\n", numCh);
    printf("\nDriver expects %d session slots at [devext+0x528]\n", numDev);
    printf("Transfer buffer: %d frames x %d channels x 16bit = %d bytes + 8 header = %d total\n",
        NUM_FRAMES, numCh, NUM_FRAMES * numCh * 2, NUM_FRAMES * numCh * 2 + XFER_HEADER);
    
    if (numCh != 8) {
        printf("\n*** WARNING: NumChannels=%d, but our code assumes 8! ***\n", numCh);
        printf("*** Transfer buffer size may need adjustment! ***\n");
    }
    
    RegCloseKey(key);
}

// 测试 Attach IOCTL
bool testAttach(HANDLE dev, DWORD clientId)
{
    printf("\n=== Test Attach (clientId=%d, param2=0) ===\n", clientId);
    
    IoctlBuf in = { MAGIC, clientId, 0 };
    IoctlBuf out = {};
    DWORD ret = 0;
    
    BOOL ok = DeviceIoControl(dev, IOCTL_ATTACH, &in, 12, &out, 12, &ret, NULL);
    DWORD err = GetLastError();
    
    printf("  Result: %s (err=%d, bytes=%d)\n", ok ? "OK" : "FAIL", err, ret);
    if (ok && ret >= 12) {
        printf("  Output: magic=0x%08X version=%d status=%d\n", out.magic, out.p1, out.p2);
        if (out.magic == MAGIC)
            printf("  Magic: MATCH\n");
        else
            printf("  Magic: MISMATCH (expected 0x%08X)\n", MAGIC);
        
        if (out.p2 == 0)
            printf("  Status: OK (slot available)\n");
        else
            printf("  Status: BUSY or ERROR (%d)\n", out.p2);
    }
    
    return ok && out.magic == MAGIC && out.p2 == 0;
}

// 测试 Detach IOCTL
void testDetach(HANDLE dev, DWORD clientId)
{
    printf("\n=== Test Detach (clientId=%d) ===\n", clientId);
    
    IoctlBuf in = { MAGIC, clientId, 0 };
    DWORD ret = 0;
    BOOL ok = DeviceIoControl(dev, IOCTL_DETACH, &in, 12, NULL, 0, &ret, NULL);
    printf("  Result: %s (err=%d)\n", ok ? "OK" : "FAIL", GetLastError());
}

// 测试 Start Transfer IOCTL
bool testStartTransfer(HANDLE dev, DWORD clientId)
{
    printf("\n=== Test Start Transfer ===\n");
    
    IoctlBuf in = { MAGIC, clientId, 0 };
    DWORD ret = 0;
    BOOL ok = DeviceIoControl(dev, IOCTL_START, &in, 12, NULL, 0, &ret, NULL);
    printf("  Result: %s (err=%d, bytes=%d)\n", ok ? "OK" : "FAIL", GetLastError(), ret);
    return ok;
}

// 测试 Transfer IOCTL (正确的 0x188 缓冲区)
void testTransfer(HANDLE dev, int count)
{
    printf("\n=== Test Transfer (0x188 buffer, %d iterations) ===\n", count);
    
    BYTE buf[XFER_BUF_SIZE] = {};
    
    for (int t = 0; t < count; t++)
    {
        memset(buf, 0, XFER_BUF_SIZE);
        
        DWORD ret = 0;
        BOOL ok = DeviceIoControl(dev, IOCTL_TRANSFER, 
            buf, XFER_BUF_SIZE, buf, XFER_BUF_SIZE, &ret, NULL);
        
        if (!ok) {
            printf("  [%d] FAIL err=%d\n", t, GetLastError());
            break;
        }
        
        // Check header
        DWORD hdr_lo = *(DWORD*)buf;
        DWORD hdr_hi = *(DWORD*)(buf + 4);
        
        // Check if PCM data has any non-zero values
        bool hasAudio = false;
        SHORT* pcm = (SHORT*)(buf + XFER_HEADER);
        SHORT maxVal = 0;
        for (int i = 0; i < NUM_FRAMES * NUM_CH; i++) {
            if (pcm[i] != 0) hasAudio = true;
            if (abs(pcm[i]) > abs(maxVal)) maxVal = pcm[i];
        }
        
        if (t < 5 || t == count - 1 || hasAudio) {
            printf("  [%d] OK ret=%d hdr=0x%08X:%08X audio=%s peak=%d\n",
                t, ret, hdr_hi, hdr_lo, hasAudio ? "YES" : "silent", maxVal);
            
            if (t == 0) {
                printf("       First 32 bytes: ");
                for (int j = 0; j < 32; j++) printf("%02X ", buf[j]);
                printf("\n");
            }
        }
    }
}

int main()
{
    printf("=========================================\n");
    printf("  ASIOVADPRO IOCTL Protocol Test Tool\n");
    printf("  (Safe mode - step by step)\n");
    printf("=========================================\n");
    
    // Step 1: Check registry
    checkRegistry();
    
    // Step 2: List devices
    listAllDevices();
    
    printf("Enter device index to test (-1 to quit): ");
    int idx;
    scanf("%d", &idx);
    if (idx < 0) return 0;
    
    // Step 3: Open device
    HANDLE dev = openDevice(idx);
    if (dev == INVALID_HANDLE_VALUE) {
        printf("Cannot open device. Try a different index.\n");
        return 1;
    }
    
    DWORD clientId = GetCurrentProcessId();
    printf("\nUsing clientId = %d (PID)\n", clientId);
    
    // Step 4: Attach
    printf("\nPress ENTER to test Attach (or 'q' to skip)...");
    char c = getchar(); c = getchar(); // consume newline
    if (c != 'q') {
        if (testAttach(dev, clientId)) {
            // Step 5: Start Transfer
            printf("\nPress ENTER to test Start Transfer...");
            getchar();
            if (testStartTransfer(dev, clientId)) {
                // Step 6: Transfer data
                printf("\nHow many transfer iterations? (1-100): ");
                int n = 5;
                scanf("%d", &n);
                if (n < 1) n = 1;
                if (n > 100) n = 100;
                
                testTransfer(dev, n);
            }
            
            // Step 7: Detach
            printf("\nPress ENTER to detach...");
            getchar(); getchar();
            testDetach(dev, clientId);
        }
    }
    
    CloseHandle(dev);
    printf("\n=== Done ===\n");
    return 0;
}
