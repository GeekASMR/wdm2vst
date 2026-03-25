#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <initguid.h>
DEFINE_GUID(G, 0x6994AD04, 0x93EF, 0x11D0, 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);
int main() {
    HDEVINFO h = SetupDiGetClassDevsA(&G, NULL, NULL, DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);
    SP_DEVICE_INTERFACE_DATA d; d.cbSize = sizeof(d);
    for (DWORD i=0; SetupDiEnumDeviceInterfaces(h,NULL,&G,i,&d); i++) {
        DWORD sz=0; SetupDiGetDeviceInterfaceDetailA(h,&d,NULL,0,&sz,NULL);
        auto* p = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(sz);
        p->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        SP_DEVINFO_DATA di; di.cbSize=sizeof(di);
        if (SetupDiGetDeviceInterfaceDetailA(h,&d,p,sz,NULL,&di)) {
            char hw[256]={}; SetupDiGetDeviceRegistryPropertyA(h,&di,SPDRP_HARDWAREID,NULL,(BYTE*)hw,256,NULL);
            if (strstr(hw,"ASIOVADPRO") && strstr(p->DevicePath,"wave1"))
                printf("PATH: %s\n", p->DevicePath);
        }
        free(p);
    }
    SetupDiDestroyDeviceInfoList(h);
}
