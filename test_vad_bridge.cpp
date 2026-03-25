#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <initguid.h>
#include <math.h>
#pragma comment(lib, "setupapi.lib")

DEFINE_GUID(GUID_KS_A, 0x6994AD04, 0x93EF, 0x11D0, 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

#define IOCTL_ATTACH   0x9C402410
#define IOCTL_DETACH   0x9C402414
#define IOCTL_START    0x9C402420
#define IOCTL_TRANSFER 0x9C402428
#define MAGIC          0x0131CA08

struct IoctlBuf { DWORD magic, p1, p2; };

HANDLE findAndOpen() {
    HDEVINFO h = SetupDiGetClassDevsA(&GUID_KS_A, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (h == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA ifd; ifd.cbSize = sizeof(ifd);
    HANDLE result = INVALID_HANDLE_VALUE;
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(h, NULL, &GUID_KS_A, i, &ifd); i++) {
        DWORD sz = 0;
        SetupDiGetDeviceInterfaceDetailA(h, &ifd, NULL, 0, &sz, NULL);
        auto* d = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(sz);
        d->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        if (SetupDiGetDeviceInterfaceDetailA(h, &ifd, d, sz, NULL, NULL)) {
            if (strstr(d->DevicePath, "0004") || strstr(d->DevicePath, "0005")) {
                result = CreateFileA(d->DevicePath, GENERIC_READ|GENERIC_WRITE, 3, NULL, OPEN_EXISTING, 0, NULL);
                if (result != INVALID_HANDLE_VALUE) {
                    printf("Opened: %s\n", d->DevicePath);
                    free(d); break;
                }
            }
        }
        free(d);
    }
    SetupDiDestroyDeviceInfoList(h);
    return result;
}

int main() {
    printf("=== Full ASIOVADPRO Transfer Test ===\n\n");
    
    HANDLE dev = findAndOpen();
    if (dev == INVALID_HANDLE_VALUE) { printf("FAIL: no device\n"); return 1; }
    
    // 1. Attach
    IoctlBuf in = {MAGIC, 0, 0}, out = {};
    DWORD ret = 0;
    BOOL ok = DeviceIoControl(dev, IOCTL_ATTACH, &in, 12, &out, 12, &ret, NULL);
    printf("[Attach] %s ret=%d magic=0x%X ver=%d status=%d\n", ok?"OK":"FAIL", ret, out.magic, out.p1, out.p2);
    if (!ok) { CloseHandle(dev); return 1; }
    
    // 2. Start Transfer - try different input sizes
    printf("\n--- Testing Start Transfer ---\n");
    
    // Try with 12-byte magic buffer
    in = {MAGIC, 0, 0};
    ret = 0;
    ok = DeviceIoControl(dev, IOCTL_START, &in, 12, &out, 12, &ret, NULL);
    printf("[Start 12B] %s err=%d ret=%d\n", ok?"OK":"FAIL", GetLastError(), ret);
    if (ok && ret > 0) {
        printf("  Output: 0x%X %d %d\n", out.magic, out.p1, out.p2);
    }
    
    // 3. Transfer Data - try various buffer sizes
    printf("\n--- Testing Transfer Data ---\n");
    
    // Try sizes matching common audio formats:
    // 256 frames * 8 channels * 2 bytes (16-bit) = 4096
    // 256 frames * 2 channels * 2 bytes (16-bit) = 1024
    // 256 frames * 8 channels * 4 bytes (32-bit) = 8192
    int testSizes[] = {12, 48, 256, 512, 1024, 2048, 4096, 8192, 16384};
    
    for (int sz : testSizes) {
        BYTE* buf = (BYTE*)calloc(sz, 1);
        // Put magic at the start
        *(DWORD*)buf = MAGIC;
        
        ret = 0;
        ok = DeviceIoControl(dev, IOCTL_TRANSFER, buf, sz, buf, sz, &ret, NULL);
        DWORD err = GetLastError();
        
        printf("[Transfer %5dB] %s err=%d ret=%d", sz, ok?"OK":"FAIL", err, ret);
        if (ok && ret > 0) {
            printf(" first16:");
            for (int j = 0; j < 16 && j < (int)ret; j++) printf(" %02X", buf[j]);
            // Check if returned data is non-zero (actual audio)
            bool hasData = false;
            for (DWORD j = 0; j < ret; j++) {
                if (buf[j] != 0) { hasData = true; break; }
            }
            printf(" data=%s", hasData ? "YES" : "zeros");
        }
        printf("\n");
        free(buf);
    }
    
    // 4. Try transfer with a sine wave (render direction)
    printf("\n--- Sine Wave Render Test ---\n");
    {
        // 8ch * 16bit * 256 frames = 4096 bytes, with magic header
        int numChannels = 8;
        int numFrames = 256;
        int headerSize = 12; // magic + params
        int pcmSize = numFrames * numChannels * 2; // 16-bit
        int totalSize = headerSize + pcmSize;
        
        BYTE* buf = (BYTE*)calloc(totalSize, 1);
        *(DWORD*)(buf) = MAGIC;
        *(DWORD*)(buf + 4) = numFrames;
        *(DWORD*)(buf + 8) = numChannels;
        
        // Fill with 440Hz sine on channel 0
        SHORT* pcm = (SHORT*)(buf + headerSize);
        for (int i = 0; i < numFrames; i++) {
            float val = sinf(2.0f * 3.14159f * 440.0f * i / 44100.0f);
            pcm[i * numChannels] = (SHORT)(val * 32767.0f);
        }
        
        ret = 0;
        ok = DeviceIoControl(dev, IOCTL_TRANSFER, buf, totalSize, buf, totalSize, &ret, NULL);
        printf("[Render %dB] %s err=%d ret=%d\n", totalSize, ok?"OK":"FAIL", GetLastError(), ret);
        
        // Also try without header
        pcm = (SHORT*)calloc(pcmSize, 1);
        for (int i = 0; i < numFrames; i++) {
            float val = sinf(2.0f * 3.14159f * 440.0f * i / 44100.0f);
            ((SHORT*)pcm)[i * numChannels] = (SHORT)(val * 32767.0f);
        }
        ret = 0;
        ok = DeviceIoControl(dev, IOCTL_TRANSFER, pcm, pcmSize, pcm, pcmSize, &ret, NULL);
        printf("[Render raw %dB] %s err=%d ret=%d\n", pcmSize, ok?"OK":"FAIL", GetLastError(), ret);
        free(pcm);
        free(buf);
    }
    
    // 5. Detach
    printf("\n");
    in = {MAGIC, 0, 0};
    ok = DeviceIoControl(dev, IOCTL_DETACH, &in, 12, NULL, 0, &ret, NULL);
    printf("[Detach] %s err=%d\n", ok?"OK":"FAIL", GetLastError());
    
    CloseHandle(dev);
    printf("\n=== Done ===\n");
    return 0;
}
