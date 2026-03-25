#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdio.h>

int main() {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    
    IMMDeviceEnumerator *pEnum = NULL;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    
    const wchar_t* renderNames[] = {L"Virtual Bridge 1", L"Virtual Bridge 2", L"Virtual Bridge 3", L"Virtual Bridge 4"};
    const wchar_t* captureNames[] = {L"Virtual Bridge Mic 1", L"Virtual Bridge Mic 2", L"Virtual Bridge Mic 3", L"Virtual Bridge Mic 4"};
    
    // === RENDER ===
    IMMDeviceCollection *pCol = NULL;
    pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &pCol);
    UINT count = 0;
    pCol->GetCount(&count);
    int ri = 0;
    
    for (UINT i = 0; i < count && ri < 4; i++) {
        IMMDevice *pDev = NULL;
        pCol->Item(i, &pDev);
        IPropertyStore *pStore = NULL;
        pDev->OpenPropertyStore(STGM_READ, &pStore);
        
        PROPVARIANT pvFriendly;
        PropVariantInit(&pvFriendly);
        pStore->GetValue(PKEY_Device_FriendlyName, &pvFriendly);
        
        if (pvFriendly.vt == VT_LPWSTR && wcsstr(pvFriendly.pwszVal, L"WDM2VST")) {
            pStore->Release();
            
            IPropertyStore *pStoreW = NULL;
            pDev->OpenPropertyStore(STGM_READWRITE, &pStoreW);
            if (pStoreW) {
                PROPVARIANT pvName;
                pvName.vt = VT_LPWSTR;
                
                pvName.pwszVal = SysAllocString(renderNames[ri]);
                pStoreW->SetValue(PKEY_Device_FriendlyName, pvName);
                pStoreW->SetValue(PKEY_DeviceInterface_FriendlyName, pvName);
                pStoreW->SetValue(PKEY_Device_DeviceDesc, pvName);
                pStoreW->Commit();
                SysFreeString(pvName.pwszVal);
                pStoreW->Release();
                
                wprintf(L"Render: %s -> %s\n", pvFriendly.pwszVal, renderNames[ri]);
                ri++;
            } else {
                wprintf(L"FAILED to open write store for: %s\n", pvFriendly.pwszVal);
            }
        } else {
            pStore->Release();
        }
        PropVariantClear(&pvFriendly);
        pDev->Release();
    }
    pCol->Release();
    
    // === CAPTURE ===
    IMMDeviceCollection *pCapCol = NULL;
    pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &pCapCol);
    UINT capCount = 0;
    pCapCol->GetCount(&capCount);
    int ci = 0;
    
    for (UINT i = 0; i < capCount && ci < 4; i++) {
        IMMDevice *pDev = NULL;
        pCapCol->Item(i, &pDev);
        IPropertyStore *pStore = NULL;
        pDev->OpenPropertyStore(STGM_READ, &pStore);
        
        PROPVARIANT pvFriendly;
        PropVariantInit(&pvFriendly);
        pStore->GetValue(PKEY_Device_FriendlyName, &pvFriendly);
        
        if (pvFriendly.vt == VT_LPWSTR && wcsstr(pvFriendly.pwszVal, L"WDM2VST")) {
            pStore->Release();
            
            IPropertyStore *pStoreW = NULL;
            pDev->OpenPropertyStore(STGM_READWRITE, &pStoreW);
            if (pStoreW) {
                PROPVARIANT pvName;
                pvName.vt = VT_LPWSTR;
                
                pvName.pwszVal = SysAllocString(captureNames[ci]);
                pStoreW->SetValue(PKEY_Device_FriendlyName, pvName);
                pStoreW->SetValue(PKEY_DeviceInterface_FriendlyName, pvName);
                pStoreW->SetValue(PKEY_Device_DeviceDesc, pvName);
                pStoreW->Commit();
                SysFreeString(pvName.pwszVal);
                pStoreW->Release();
                
                wprintf(L"Capture: %s -> %s\n", pvFriendly.pwszVal, captureNames[ci]);
                ci++;
            }
        } else {
            pStore->Release();
        }
        PropVariantClear(&pvFriendly);
        pDev->Release();
    }
    pCapCol->Release();
    pEnum->Release();
    
    printf("\nDone! Renamed %d render + %d capture devices.\n", ri, ci);
    printf("Please close and reopen mmsys.cpl to see changes.\n");
    CoUninitialize();
    return 0;
}
