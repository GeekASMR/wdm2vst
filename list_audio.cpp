#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdio.h>

int main() {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    IMMDeviceEnumerator *pEnum = NULL;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    
    IMMDeviceCollection *pCol = NULL;
    pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &pCol);
    
    UINT count = 0;
    pCol->GetCount(&count);
    wprintf(L"=== RENDER DEVICES (%u) ===\n", count);
    
    for (UINT i = 0; i < count; i++) {
        IMMDevice *pDev = NULL;
        pCol->Item(i, &pDev);
        
        LPWSTR id = NULL;
        pDev->GetId(&id);
        
        IPropertyStore *pStore = NULL;
        pDev->OpenPropertyStore(STGM_READ, &pStore);
        
        PROPVARIANT pvDesc, pvFriendly, pvIfFriendly;
        PropVariantInit(&pvDesc);
        PropVariantInit(&pvFriendly);
        PropVariantInit(&pvIfFriendly);
        
        pStore->GetValue(PKEY_Device_DeviceDesc, &pvDesc);
        pStore->GetValue(PKEY_Device_FriendlyName, &pvFriendly);
        pStore->GetValue(PKEY_DeviceInterface_FriendlyName, &pvIfFriendly);
        
        wprintf(L"[%u] ID: %s\n", i, id);
        if (pvDesc.vt == VT_LPWSTR) wprintf(L"    DeviceDesc: %s\n", pvDesc.pwszVal);
        if (pvFriendly.vt == VT_LPWSTR) wprintf(L"    FriendlyName: %s\n", pvFriendly.pwszVal);
        if (pvIfFriendly.vt == VT_LPWSTR) wprintf(L"    IfFriendlyName: %s\n", pvIfFriendly.pwszVal);
        
        PropVariantClear(&pvDesc);
        PropVariantClear(&pvFriendly);
        PropVariantClear(&pvIfFriendly);
        CoTaskMemFree(id);
        pStore->Release();
        pDev->Release();
    }
    pCol->Release();
    pEnum->Release();
    CoUninitialize();
    return 0;
}
