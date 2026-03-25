// test_transfer_dump.cpp - Dump IOCTL_TRANSFER buffer to find where PCM data lives
// Does ONE safe transfer and prints hex dump of the entire 0x54C buffer
#include <windows.h>
#include <stdio.h>
#include <setupapi.h>
#include <initguid.h>
#pragma comment(lib, "setupapi.lib")

DEFINE_GUID(GUID_KS, 0x6994AD04, 0x93EF, 0x11D0,
    0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

#define IOCTL_ATTACH   0x9C402410
#define IOCTL_START    0x9C402420
#define IOCTL_TRANSFER 0x9C402428
#define IOCTL_DETACH   0x9C402414
#define MAGIC          0x0131CA08
#define BUF_SIZE       0x54C

struct Buf12 { DWORD magic, p1, p2; };

char* findDevice() {
    static char path[512];
    HDEVINFO h = SetupDiGetClassDevsA(&GUID_KS, NULL, NULL, DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);
    SP_DEVICE_INTERFACE_DATA d; d.cbSize = sizeof(d);
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(h, NULL, &GUID_KS, i, &d); i++) {
        DWORD sz = 0;
        SetupDiGetDeviceInterfaceDetailA(h, &d, NULL, 0, &sz, NULL);
        auto* p = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(sz);
        p->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        SP_DEVINFO_DATA di; di.cbSize = sizeof(di);
        if (SetupDiGetDeviceInterfaceDetailA(h, &d, p, sz, NULL, &di)) {
            char hw[256] = {};
            SetupDiGetDeviceRegistryPropertyA(h, &di, SPDRP_HARDWAREID, NULL, (BYTE*)hw, 256, NULL);
            if (strstr(hw, "ASIOVADPRO") && strstr(p->DevicePath, "wave1")) {
                strcpy(path, p->DevicePath);
                free(p);
                SetupDiDestroyDeviceInfoList(h);
                return path;
            }
        }
        free(p);
    }
    SetupDiDestroyDeviceInfoList(h);
    return NULL;
}

void hexdump(const BYTE* data, int len, int offset) {
    for (int i = 0; i < len; i += 16) {
        printf("  %04X: ", offset + i);
        for (int j = 0; j < 16 && (i+j) < len; j++)
            printf("%02X ", data[i+j]);
        for (int j = (len - i < 16 ? len - i : 16); j < 16; j++)
            printf("   ");
        printf(" |");
        for (int j = 0; j < 16 && (i+j) < len; j++) {
            char c = data[i+j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("|\n");
    }
}

int main() {
    printf("=== IOCTL_TRANSFER Buffer Dump (0x54C bytes) ===\n\n");
    
    char* path = findDevice();
    if (!path) { printf("ERROR: Device not found\n"); return 1; }
    printf("Device: %s\n", path);
    
    HANDLE h = CreateFileA(path, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) { printf("ERROR: CreateFile (%d)\n", GetLastError()); return 1; }
    
    // Attach
    Buf12 in = {MAGIC, (DWORD)GetCurrentProcessId(), 0}, out = {};
    DWORD ret = 0;
    if (!DeviceIoControl(h, IOCTL_ATTACH, &in, 12, &out, 12, &ret, NULL)) {
        printf("ERROR: Attach (%d)\n", GetLastError()); CloseHandle(h); return 1;
    }
    printf("Attached: version=%d status=%d\n", out.p1, out.p2);
    
    // Start
    Buf12 start = {MAGIC, (DWORD)GetCurrentProcessId(), 0};
    if (!DeviceIoControl(h, IOCTL_START, &start, 12, NULL, 0, &ret, NULL)) {
        printf("ERROR: Start (%d)\n", GetLastError()); CloseHandle(h); return 1;
    }
    printf("Started\n\n");
    
    // Do a few transfers to let the driver warm up
    BYTE buf[BUF_SIZE];
    for (int warm = 0; warm < 10; warm++) {
        memset(buf, 0, BUF_SIZE);
        DeviceIoControl(h, IOCTL_TRANSFER, buf, BUF_SIZE, buf, BUF_SIZE, &ret, NULL);
        Sleep(5);
    }
    
    // Now do the actual transfer we want to inspect
    memset(buf, 0xCC, BUF_SIZE);  // Fill with 0xCC sentinel to see what driver overwrites
    ret = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_TRANSFER, buf, BUF_SIZE, buf, BUF_SIZE, &ret, NULL);
    printf("Transfer result: %s, bytesReturned=%d (0x%X)\n\n", ok ? "OK" : "FAILED", ret, ret);
    
    if (ok) {
        // Check which regions were modified (no longer 0xCC)
        printf("--- Block A [0x000..0x187] (Render/Input region) ---\n");
        hexdump(buf, 0x188, 0);
        
        printf("\n--- Block B [0x188..0x54B] (Capture/Output region) ---\n");
        hexdump(buf + 0x188, 0x3C4, 0x188);
        
        // Find any non-zero SHORT samples in Block A
        printf("\n--- PCM scan Block A (offset 8, 24 frames x 8ch) ---\n");
        SHORT* pcmA = (SHORT*)(buf + 8);
        int nonzeroA = 0;
        for (int i = 0; i < 24*8; i++) if (pcmA[i] != 0 && (BYTE)pcmA[i] != 0xCC) nonzeroA++;
        printf("Non-zero samples in Block A: %d / %d\n", nonzeroA, 24*8);
        
        // Find any non-zero SHORT samples in Block B  
        printf("\n--- PCM scan Block B (offset 0x190, first 384 bytes) ---\n");
        SHORT* pcmB = (SHORT*)(buf + 0x188 + 8);
        int nonzeroB = 0;
        for (int i = 0; i < 24*8; i++) if (pcmB[i] != 0 && (BYTE)pcmB[i] != 0xCC) nonzeroB++;
        printf("Non-zero samples in Block B: %d / %d\n", nonzeroB, 24*8);
        
        // Check bytesReturned vs buffer size
        printf("\nbytesReturned = 0x%X (%d)\n", ret, ret);
        printf("Expected = 0x%X (%d)\n", BUF_SIZE, BUF_SIZE);
    }
    
    // Detach
    Buf12 det = {MAGIC, (DWORD)GetCurrentProcessId(), 0};
    DeviceIoControl(h, IOCTL_DETACH, &det, 12, NULL, 0, &ret, NULL);
    CloseHandle(h);
    printf("\nDetached and closed.\n");
    return 0;
}
