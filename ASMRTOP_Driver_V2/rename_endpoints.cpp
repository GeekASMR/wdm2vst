// rename_endpoints.cpp - Small utility to rename audio endpoints
// Compile: cl.exe /EHsc rename_endpoints.cpp ole32.lib propsys.lib
// This does the same thing as right-clicking an endpoint in Sound panel and renaming it.

#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propsys.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ole32.lib")

// PKEY_Device_FriendlyName = {a45c254e-df1c-4efd-8020-67d146a850e0},14
// PKEY_DeviceInterface_FriendlyName = {026e516e-b814-414b-83cd-856d6fef4822},2

int wmain(int argc, wchar_t* argv[])
{
    CoInitialize(NULL);

    IMMDeviceEnumerator* pEnum = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
                                  __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (FAILED(hr)) { wprintf(L"Failed to create enumerator: 0x%08X\n", hr); return 1; }

    // Process both render and capture
    EDataFlow flows[] = { eRender, eCapture };
    const wchar_t* flowNames[] = { L"Render", L"Capture" };

    // Expected names - passed via command line or hardcoded
    // Format: rename_endpoints.exe "DeviceName" "Spk1" "Spk2" "Spk3" "Spk4" "Mic1" "Mic2" "Mic3" "Mic4"
    const wchar_t* deviceMatch = (argc > 1) ? argv[1] : L"Virtual Audio Router";
    const wchar_t* spkNames[4];
    const wchar_t* micNames[4];
    
    if (argc >= 10) {
        for (int i = 0; i < 4; i++) {
            spkNames[i] = argv[2 + i];
            micNames[i] = argv[6 + i];
        }
    } else {
        spkNames[0] = L"Virtual 1/2"; spkNames[1] = L"Virtual 3/4";
        spkNames[2] = L"Virtual 5/6"; spkNames[3] = L"Virtual 7/8";
        micNames[0] = L"Mic 1/2"; micNames[1] = L"Mic 3/4";
        micNames[2] = L"Mic 5/6"; micNames[3] = L"Mic 7/8";
    }

    for (int f = 0; f < 2; f++) {
        IMMDeviceCollection* pColl = NULL;
        hr = pEnum->EnumAudioEndpoints(flows[f], DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &pColl);
        if (FAILED(hr)) continue;

        UINT count = 0;
        pColl->GetCount(&count);
        
        int nameIdx = 0;
        const wchar_t** names = (f == 0) ? spkNames : micNames;

        for (UINT i = 0; i < count; i++) {
            IMMDevice* pDev = NULL;
            pColl->Item(i, &pDev);
            if (!pDev) continue;

            IPropertyStore* pStore = NULL;
            pDev->OpenPropertyStore(STGM_READ, &pStore);
            if (!pStore) { pDev->Release(); continue; }

            // Check if this is our device
            PROPVARIANT pvName;
            PropVariantInit(&pvName);
            pStore->GetValue(PKEY_Device_FriendlyName, &pvName);
            
            bool isOurs = false;
            if (pvName.vt == VT_LPWSTR && wcsstr(pvName.pwszVal, deviceMatch)) {
                isOurs = true;
            }
            PropVariantClear(&pvName);
            pStore->Release();

            if (isOurs && nameIdx < 4) {
                // Open with write access
                IPropertyStore* pStoreW = NULL;
                pDev->OpenPropertyStore(STGM_READWRITE, &pStoreW);
                if (pStoreW) {
                    // Build new friendly name: "CustomName (DeviceName)"
                    wchar_t newName[256];
                    swprintf_s(newName, L"%s (%s)", names[nameIdx], deviceMatch);

                    PROPVARIANT pvNew;
                    PropVariantInit(&pvNew);
                    pvNew.vt = VT_LPWSTR;
                    pvNew.pwszVal = (LPWSTR)CoTaskMemAlloc((wcslen(newName) + 1) * sizeof(wchar_t));
                    wcscpy_s(pvNew.pwszVal, wcslen(newName) + 1, newName);

                    hr = pStoreW->SetValue(PKEY_Device_FriendlyName, pvNew);
                    if (SUCCEEDED(hr)) {
                        // Also set the interface friendly name (the short name)
                        PROPVARIANT pvShort;
                        PropVariantInit(&pvShort);
                        pvShort.vt = VT_LPWSTR;
                        pvShort.pwszVal = (LPWSTR)CoTaskMemAlloc((wcslen(names[nameIdx]) + 1) * sizeof(wchar_t));
                        wcscpy_s(pvShort.pwszVal, wcslen(names[nameIdx]) + 1, names[nameIdx]);
                        
                        // {026e516e-b814-414b-83cd-856d6fef4822},2
                        static const PROPERTYKEY PKEY_DeviceInterface_FriendlyName2 = 
                            {{0x026e516e, 0xb814, 0x414b, {0x83, 0xcd, 0x85, 0x6d, 0x6f, 0xef, 0x48, 0x22}}, 2};
                        pStoreW->SetValue(PKEY_DeviceInterface_FriendlyName2, pvShort);
                        pStoreW->Commit();
                        
                        wprintf(L"  OK: %s -> %s\n", flowNames[f], newName);
                        PropVariantClear(&pvShort);
                    } else {
                        wprintf(L"  FAIL: SetValue hr=0x%08X\n", hr);
                    }
                    PropVariantClear(&pvNew);
                    pStoreW->Release();
                }
                nameIdx++;
            }
            pDev->Release();
        }
        pColl->Release();
    }

    pEnum->Release();
    CoUninitialize();
    wprintf(L"\nDone.\n");
    return 0;
}
