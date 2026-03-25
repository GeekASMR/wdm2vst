#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#pragma comment(lib, "setupapi.lib")

DEFINE_GUID(KSCATEGORY_AUDIO_GUID,
    0x6994AD04, 0x93EF, 0x11D0, 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

int main() {
    printf("=== WDM2VST Shared Memory Probe ===\n\n");
    
    // 1. Try to open the shared memory
    const char* shmName = "Global\\{Wdm2VST88EDC269-54DF-428C-BA96-F2DA06BC7BB1}";
    printf("[1] Opening shared memory: %s\n", shmName);
    
    HANDLE hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, shmName);
    if (hMap != NULL) {
        printf("  SUCCESS! Handle=%p\n", hMap);
        
        // Map it
        void* pBuf = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        if (pBuf) {
            // Query the size
            MEMORY_BASIC_INFORMATION mbi;
            VirtualQuery(pBuf, &mbi, sizeof(mbi));
            printf("  Mapped size: %llu bytes\n", (unsigned long long)mbi.RegionSize);
            
            // Dump first 512 bytes as hex
            printf("\n  First 512 bytes (hex dump):\n");
            unsigned char* p = (unsigned char*)pBuf;
            for (int row = 0; row < 32; row++) {
                printf("  %04X: ", row * 16);
                for (int col = 0; col < 16; col++) {
                    printf("%02X ", p[row * 16 + col]);
                }
                printf(" | ");
                for (int col = 0; col < 16; col++) {
                    unsigned char c = p[row * 16 + col];
                    printf("%c", (c >= 32 && c < 127) ? c : '.');
                }
                printf("\n");
            }
            
            // Look for WAVEFORMATEX-like structures (nChannels, nSamplesPerSec at typical values)
            printf("\n  Scanning for WAVEFORMATEX...\n");
            DWORD* dwords = (DWORD*)pBuf;
            for (int i = 0; i < (int)(mbi.RegionSize / 4) && i < 256; i++) {
                // Look for common sample rates
                if (dwords[i] == 44100 || dwords[i] == 48000 || dwords[i] == 96000) {
                    printf("  [DWORD %d, offset 0x%X] = %u (possible sample rate)\n", i, i*4, dwords[i]);
                }
                // Look for channel counts
                if (dwords[i] == 2 || dwords[i] == 8) {
                    WORD* words = (WORD*)&dwords[i];
                    if (words[0] == 2 || words[0] == 8) {
                        printf("  [WORD at offset 0x%X] = %u (possible channel count)\n", i*4, words[0]);
                    }
                }
            }
            
            // Look for ring buffer pointers (large contiguous regions)
            printf("\n  Looking for buffer structure indicators...\n");
            for (int i = 0; i < 64; i++) {
                printf("  [DWORD %d, offset 0x%03X] = 0x%08X (%u)\n", i, i*4, dwords[i], dwords[i]);
            }
            
            UnmapViewOfFile(pBuf);
        } else {
            printf("  MapViewOfFile failed: %d\n", GetLastError());
        }
        CloseHandle(hMap);
    } else {
        printf("  FAILED: %d (probably not yet created by driver)\n", GetLastError());
    }
    
    // 2. Check registry settings
    printf("\n[2] Registry: Software\\ODeusAudio\\Wdm2Vst\n");
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\ODeusAudio\\Wdm2Vst", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD numDevices = 0, numChannels = 0, valSize = sizeof(DWORD);
        RegQueryValueExA(hKey, "NumDevices", NULL, NULL, (BYTE*)&numDevices, &valSize);
        valSize = sizeof(DWORD);
        RegQueryValueExA(hKey, "NumChannels", NULL, NULL, (BYTE*)&numChannels, &valSize);
        printf("  NumDevices=%d, NumChannels=%d\n", numDevices, numChannels);
        
        // Enumerate all values
        char valName[256];
        DWORD valNameLen, valType, valData;
        DWORD datSize;
        for (DWORD i = 0; ; i++) {
            valNameLen = sizeof(valName);
            datSize = sizeof(valData);
            if (RegEnumValueA(hKey, i, valName, &valNameLen, NULL, &valType, (BYTE*)&valData, &datSize) != ERROR_SUCCESS)
                break;
            if (valType == REG_DWORD)
                printf("  %s = %d (0x%X)\n", valName, valData, valData);
        }
        RegCloseKey(hKey);
    } else {
        printf("  Not found\n");
    }
    
    // 3. Find WDM2VST device interfaces
    printf("\n[3] WDM2VST Device Interfaces:\n");
    HDEVINFO hDevInfo = SetupDiGetClassDevsA(
        &KSCATEGORY_AUDIO_GUID, NULL, NULL,
        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    
    if (hDevInfo != INVALID_HANDLE_VALUE) {
        SP_DEVICE_INTERFACE_DATA ifData;
        ifData.cbSize = sizeof(ifData);
        int found = 0;
        
        for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &KSCATEGORY_AUDIO_GUID, i, &ifData); i++) {
            DWORD detailSize = 0;
            SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifData, NULL, 0, &detailSize, NULL);
            auto* detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(detailSize);
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
            SP_DEVINFO_DATA devInfo;
            devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
            
            if (SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifData, detail, detailSize, NULL, &devInfo)) {
                char hwId[512] = {};
                SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfo, SPDRP_HARDWAREID, NULL, (BYTE*)hwId, sizeof(hwId), NULL);
                
                if (strstr(hwId, "WDM2VST") || strstr(hwId, "wdm2vst")) {
                    found++;
                    char desc[256] = {};
                    SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfo, SPDRP_FRIENDLYNAME, NULL, (BYTE*)desc, sizeof(desc), NULL);
                    if (!desc[0])
                        SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfo, SPDRP_DEVICEDESC, NULL, (BYTE*)desc, sizeof(desc), NULL);
                    
                    printf("  [%d] DevInst=%d  Name=%s\n", found, devInfo.DevInst, desc);
                    printf("      HwID: %s\n", hwId);
                    printf("      Path: %s\n", detail->DevicePath);
                    
                    // Try to open this device
                    HANDLE hDev = CreateFileA(detail->DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, 0, NULL);
                    if (hDev != INVALID_HANDLE_VALUE) {
                        printf("      CreateFile: SUCCESS\n");
                        
                        // Try attach with different magic values
                        struct { DWORD magic; DWORD param1; DWORD param2; } input, output;
                        DWORD bytesReturned;
                        
                        // Try magic 0x0131CA08
                        memset(&output, 0, sizeof(output));
                        input.magic = 0x0131CA08;
                        input.param1 = GetCurrentProcessId();
                        input.param2 = 0;
                        BOOL r = DeviceIoControl(hDev, 0x9C402410, &input, sizeof(input), &output, sizeof(output), &bytesReturned, NULL);
                        printf("      Attach(magic=0x0131CA08): %s (err=%d, returned=%d bytes)\n", 
                            r ? "OK" : "FAIL", r ? 0 : GetLastError(), bytesReturned);
                        if (r) {
                            printf("      Response: magic=0x%08X ver=%d status=%d\n", output.magic, output.param1, output.param2);
                            // Detach
                            DeviceIoControl(hDev, 0x9C402414, &input, sizeof(input), NULL, 0, &bytesReturned, NULL);
                        }
                        
                        // Try magic 0 (no magic)
                        memset(&output, 0, sizeof(output));
                        input.magic = 0;
                        input.param1 = GetCurrentProcessId();
                        input.param2 = 0;
                        r = DeviceIoControl(hDev, 0x9C402410, &input, sizeof(input), &output, sizeof(output), &bytesReturned, NULL);
                        printf("      Attach(magic=0): %s (err=%d, returned=%d bytes)\n", 
                            r ? "OK" : "FAIL", r ? 0 : GetLastError(), bytesReturned);
                        if (r) {
                            printf("      Response: magic=0x%08X ver=%d status=%d\n", output.magic, output.param1, output.param2);
                            DeviceIoControl(hDev, 0x9C402414, &input, sizeof(input), NULL, 0, &bytesReturned, NULL);
                        }
                        
                        CloseHandle(hDev);
                    } else {
                        printf("      CreateFile: FAILED (err=%d)\n", GetLastError());
                    }
                }
            }
            free(detail);
        }
        printf("  Total WDM2VST interfaces: %d\n", found);
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    
    printf("\nDone.\n");
    return 0;
}
