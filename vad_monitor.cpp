// vad_monitor.cpp - ASIOVADPRO IOCTL audio monitor
// Continuously transfers and shows audio levels in real-time
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <setupapi.h>
#include <initguid.h>
#pragma comment(lib, "setupapi.lib")

DEFINE_GUID(GUID_KS, 0x6994AD04, 0x93EF, 0x11D0, 
    0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

#define IOCTL_ATTACH   0x9C402410
#define IOCTL_DETACH   0x9C402414
#define IOCTL_START    0x9C402420
#define IOCTL_TRANSFER 0x9C402428
#define MAGIC          0x0131CA08
#define XFER_SIZE      0x188  // 392 bytes
#define NUM_CH         8
#define NUM_FRAMES     24

struct Buf { DWORD magic, p1, p2; };

// Find ASIOVADPRO wave endpoint
char* findWaveEndpoint(int waveNum)
{
    static char path[512];
    char target[32];
    sprintf(target, "wave%d", waveNum);
    
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
            if (strstr(hw, "ASIOVADPRO") && strstr(p->DevicePath, target)) {
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

// Draw a VU meter bar
void drawMeter(const char* label, float dbFS, float peak)
{
    int barLen = 40;
    // Map -60dB..0dB to 0..barLen
    int pos = (int)((dbFS + 60.0f) / 60.0f * barLen);
    if (pos < 0) pos = 0;
    if (pos > barLen) pos = barLen;
    
    int peakPos = (int)((peak + 60.0f) / 60.0f * barLen);
    if (peakPos < 0) peakPos = 0;
    if (peakPos > barLen) peakPos = barLen;
    
    printf("%s [", label);
    for (int i = 0; i < barLen; i++) {
        if (i == peakPos && peak > -59.0f)
            printf("|");
        else if (i < pos)
            printf("=");
        else
            printf(" ");
    }
    printf("] %6.1f dB", dbFS);
}

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    
    printf("=== ASIOVADPRO IOCTL Audio Monitor ===\n\n");
    
    // Find wave1
    char* path = findWaveEndpoint(1);
    if (!path) {
        printf("ERROR: ASIOVADPRO wave1 not found!\n");
        return 1;
    }
    printf("Device: %s\n", path);
    
    DWORD pid = GetCurrentProcessId();
    printf("PID: %d\n\n", pid);
    
    // Open
    HANDLE h = CreateFileA(path, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        printf("ERROR: CreateFile failed (%d)\n", GetLastError());
        return 1;
    }
    printf("OK: Device opened\n");
    
    // Attach
    Buf in = {MAGIC, pid, 0}, out = {};
    DWORD ret = 0;
    if (!DeviceIoControl(h, IOCTL_ATTACH, &in, 12, &out, 12, &ret, NULL)) {
        printf("ERROR: Attach failed (%d)\n", GetLastError());
        CloseHandle(h);
        return 1;
    }
    printf("OK: Attached (driver v%d)\n", out.p1);
    
    // Start Transfer
    Buf start = {MAGIC, pid, 0};
    if (!DeviceIoControl(h, IOCTL_START, &start, 12, NULL, 0, &ret, NULL)) {
        printf("ERROR: Start failed (%d)\n", GetLastError());
        CloseHandle(h);
        return 1;
    }
    printf("OK: Transfer started\n");
    printf("\nMonitoring audio (Ctrl+C to stop)...\n");
    printf("8 channels x 24 frames per transfer\n\n");
    
    // Peak hold for each channel
    float peakHold[NUM_CH] = {};
    for (int i = 0; i < NUM_CH; i++) peakHold[i] = -60.0f;
    
    int transferCount = 0;
    int errorCount = 0;
    BYTE buf[XFER_SIZE];
    
    while (1) {
        memset(buf, 0, XFER_SIZE);
        ret = 0;
        
        BOOL ok = DeviceIoControl(h, IOCTL_TRANSFER,
            buf, XFER_SIZE, buf, XFER_SIZE, &ret, NULL);
        
        if (!ok) {
            errorCount++;
            if (errorCount > 100) {
                printf("\nToo many errors, stopping.\n");
                break;
            }
            Sleep(10);
            continue;
        }
        
        errorCount = 0;
        transferCount++;
        
        // Parse PCM: 8 bytes header + 24 frames * 8 channels * 16bit
        SHORT* pcm = (SHORT*)(buf + 8);
        
        // Calculate RMS per channel
        float rms[NUM_CH] = {};
        SHORT maxSample[NUM_CH] = {};
        
        for (int ch = 0; ch < NUM_CH; ch++) {
            double sum = 0;
            for (int f = 0; f < NUM_FRAMES; f++) {
                SHORT s = pcm[f * NUM_CH + ch];
                sum += (double)s * s;
                if (abs(s) > abs(maxSample[ch])) maxSample[ch] = s;
            }
            rms[ch] = (float)sqrt(sum / NUM_FRAMES);
        }
        
        // Update display every ~50 transfers (roughly 25 times/sec at 44.1kHz)
        if (transferCount % 50 == 0) {
            // Move cursor up 8 lines (for 8 channels) + 1 status line
            if (transferCount > 50) printf("\033[9A");
            
            for (int ch = 0; ch < NUM_CH; ch++) {
                float dbFS = -60.0f;
                if (rms[ch] > 0) dbFS = 20.0f * log10f(rms[ch] / 32768.0f);
                if (dbFS < -60.0f) dbFS = -60.0f;
                
                // Peak hold with decay
                if (dbFS > peakHold[ch]) peakHold[ch] = dbFS;
                else peakHold[ch] -= 0.3f; // slow decay
                if (peakHold[ch] < -60.0f) peakHold[ch] = -60.0f;
                
                char label[8];
                sprintf(label, "Ch%d", ch + 1);
                drawMeter(label, dbFS, peakHold[ch]);
                printf("  peak=%6d\n", maxSample[ch]);
            }
            printf("Transfers: %d  Errors: %d        \n", transferCount, errorCount);
        }
    }
    
    // Detach
    Buf det = {MAGIC, pid, 0};
    DeviceIoControl(h, IOCTL_DETACH, &det, 12, NULL, 0, &ret, NULL);
    CloseHandle(h);
    printf("\nDetached and closed.\n");
    return 0;
}
