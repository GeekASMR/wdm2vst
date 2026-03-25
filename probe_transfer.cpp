#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "advapi32.lib")

DEFINE_GUID(KSCATEGORY_AUDIO_GUID,
    0x6994AD04, 0x93EF, 0x11D0, 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

// From original wdm2vst.dll analysis
#define IOCTL_ATTACH    0x9C402410
#define IOCTL_DETACH    0x9C402414
#define IOCTL_START     0x9C402420
#define IOCTL_STOP      0x9C402424
#define IOCTL_TRANSFER  0x9C402428
#define MAGIC           0x0131CA08

char* findFirstWaveDevice() {
    static char path[512];
    HDEVINFO hDevInfo = SetupDiGetClassDevsA(
        &KSCATEGORY_AUDIO_GUID, NULL, NULL,
        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    
    if (hDevInfo == INVALID_HANDLE_VALUE) return NULL;
    
    SP_DEVICE_INTERFACE_DATA ifData;
    ifData.cbSize = sizeof(ifData);
    
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &KSCATEGORY_AUDIO_GUID, i, &ifData); i++) {
        DWORD sz = 0;
        SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifData, NULL, 0, &sz, NULL);
        auto* det = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(sz);
        det->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        SP_DEVINFO_DATA dInfo; dInfo.cbSize = sizeof(dInfo);
        if (SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifData, det, sz, NULL, &dInfo)) {
            char hwId[512] = {};
            SetupDiGetDeviceRegistryPropertyA(hDevInfo, &dInfo, SPDRP_HARDWAREID, NULL, (BYTE*)hwId, sizeof(hwId), NULL);
            if (strstr(hwId, "WDM2VST") && strstr(det->DevicePath, "wave")) {
                strcpy(path, det->DevicePath);
                free(det);
                SetupDiDestroyDeviceInfoList(hDevInfo);
                return path;
            }
        }
        free(det);
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return NULL;
}

int main() {
    printf("=== WDM2VST IOCTL Transfer Probe ===\n\n");
    
    char* devPath = findFirstWaveDevice();
    if (!devPath) { printf("No WDM2VST wave device found!\n"); return 1; }
    printf("Device: %s\n", devPath);
    
    HANDLE hDev = CreateFileA(devPath, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDev == INVALID_HANDLE_VALUE) { printf("CreateFile failed: %d\n", GetLastError()); return 1; }
    printf("Opened OK\n");
    
    // Attach
    struct { DWORD a, b, c; } iobuf = { MAGIC, GetCurrentProcessId(), 0 };
    DWORD ret;
    if (!DeviceIoControl(hDev, IOCTL_ATTACH, &iobuf, 12, &iobuf, 12, &ret, NULL)) {
        printf("Attach failed: %d\n", GetLastError());
        CloseHandle(hDev);
        return 1;
    }
    printf("Attached OK (returned %d bytes)\n", ret);
    
    // Start
    iobuf = { MAGIC, GetCurrentProcessId(), 0 };
    if (!DeviceIoControl(hDev, IOCTL_START, &iobuf, 12, NULL, 0, &ret, NULL)) {
        printf("Start failed: %d\n", GetLastError());
    } else {
        printf("Start OK (returned %d bytes)\n", ret);
    }
    
    // Try Transfer with different buffer sizes
    int testSizes[] = { 12, 24, 48, 96, 192, 384, 392, 768, 964, 1356, 2048, 4096, 8192 };
    for (int si = 0; si < sizeof(testSizes)/sizeof(testSizes[0]); si++) {
        int bufSize = testSizes[si];
        BYTE* buf = (BYTE*)calloc(bufSize, 1);
        // Put magic at start
        *(DWORD*)buf = MAGIC;
        
        DWORD transferred = 0;
        BOOL r = DeviceIoControl(hDev, IOCTL_TRANSFER, buf, bufSize, buf, bufSize, &transferred, NULL);
        printf("\nTransfer(bufSize=%d): %s (err=%d, returned=%d bytes)\n", 
            bufSize, r ? "OK" : "FAIL", r ? 0 : GetLastError(), transferred);
        
        if (r && transferred > 0) {
            // Print first 64 bytes
            int show = transferred < 64 ? transferred : 64;
            printf("  Data: ");
            for (int j = 0; j < show; j++) printf("%02X ", buf[j]);
            printf("\n");
            
            // Check for non-zero data
            int nonzero = 0;
            for (DWORD j = 0; j < transferred; j++)
                if (buf[j] != 0) nonzero++;
            printf("  Non-zero bytes: %d / %d\n", nonzero, transferred);
        }
        free(buf);
    }
    
    // Detach
    iobuf = { MAGIC, GetCurrentProcessId(), 0 };
    DeviceIoControl(hDev, IOCTL_DETACH, &iobuf, 12, NULL, 0, &ret, NULL);
    printf("\nDetached.\n");
    
    CloseHandle(hDev);
    return 0;
}
