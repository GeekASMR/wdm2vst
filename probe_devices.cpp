#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#pragma comment(lib, "setupapi.lib")

// KSCATEGORY_AUDIO
DEFINE_GUID(KSCATEGORY_AUDIO_GUID,
    0x6994AD04, 0x93EF, 0x11D0, 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

int main() {
    HDEVINFO hDevInfo = SetupDiGetClassDevsA(
        &KSCATEGORY_AUDIO_GUID, NULL, NULL,
        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        printf("SetupDiGetClassDevs FAILED: %d\n", GetLastError());
        return 1;
    }
    
    SP_DEVICE_INTERFACE_DATA ifData;
    ifData.cbSize = sizeof(ifData);
    
    int wdmCount = 0;
    int totalCount = 0;
    
    printf("=== Enumerating KSCATEGORY_AUDIO interfaces ===\n\n");
    
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &KSCATEGORY_AUDIO_GUID, i, &ifData); i++) {
        DWORD detailSize = 0;
        SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifData, NULL, 0, &detailSize, NULL);
        
        auto* detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(detailSize);
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        
        SP_DEVINFO_DATA devInfo;
        devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
        
        if (SetupDiGetDeviceInterfaceDetailA(hDevInfo, &ifData, detail, detailSize, NULL, &devInfo)) {
            char hwId[512] = {};
            DWORD hwType = 0, hwSize = 0;
            SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfo, SPDRP_HARDWAREID,
                &hwType, (BYTE*)hwId, sizeof(hwId), &hwSize);
            
            char desc[256] = {};
            DWORD descType = 0, descSize = 0;
            SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfo, SPDRP_FRIENDLYNAME,
                &descType, (BYTE*)desc, sizeof(desc), &descSize);
            if (descSize == 0)
                SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfo, SPDRP_DEVICEDESC,
                    &descType, (BYTE*)desc, sizeof(desc), &descSize);
            
            totalCount++;
            
            // Check if WDM2VST related
            bool isWdm = false;
            if (strstr(hwId, "WDM2VST") || strstr(hwId, "wdm2vst") ||
                strstr(desc, "WDM2VST") || strstr(desc, "Virtual Bridge") || 
                strstr(desc, "ASMR Bridge") || strstr(desc, "wdm2vst")) {
                isWdm = true;
                wdmCount++;
            }
            
            if (isWdm) {
                printf("[WDM2VST #%d] DevInst=%d\n", wdmCount, devInfo.DevInst);
                printf("  HwID: %s\n", hwId);
                printf("  Name: %s\n", desc);
                printf("  Path: %s\n\n", detail->DevicePath);
            }
        }
        free(detail);
    }
    
    printf("=== Summary ===\n");
    printf("Total KSCATEGORY_AUDIO interfaces: %d\n", totalCount);
    printf("WDM2VST matches: %d\n", wdmCount);
    
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return 0;
}
