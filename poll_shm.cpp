#include <windows.h>
#include <stdio.h>

int main() {
    printf("Polling for WDM2VST shared memory...\n");
    printf("Play audio to a Virtual Bridge device, then press Enter.\n");
    
    const char* names[] = {
        "Global\\{Wdm2VST88EDC269-54DF-428C-BA96-F2DA06BC7BB1}",
        "Global\\Wdm2VST",
        "Global\\ODeusWdm2Vst",
        "Local\\{Wdm2VST88EDC269-54DF-428C-BA96-F2DA06BC7BB1}",
        "{Wdm2VST88EDC269-54DF-428C-BA96-F2DA06BC7BB1}",
    };
    
    for (int attempt = 0; attempt < 10; attempt++) {
        printf("\n--- Attempt %d ---\n", attempt + 1);
        for (int i = 0; i < 5; i++) {
            HANDLE h = OpenFileMappingA(FILE_MAP_READ, FALSE, names[i]);
            if (h) {
                printf("  FOUND: %s (handle=%p)\n", names[i], h);
                
                void* p = MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0);
                if (p) {
                    MEMORY_BASIC_INFORMATION mbi;
                    VirtualQuery(p, &mbi, sizeof(mbi));
                    printf("  Size: %llu bytes\n", (unsigned long long)mbi.RegionSize);
                    
                    // Dump first 256 bytes
                    unsigned char* buf = (unsigned char*)p;
                    for (int r = 0; r < 16; r++) {
                        printf("  %04X: ", r*16);
                        for (int c = 0; c < 16; c++)
                            printf("%02X ", buf[r*16+c]);
                        printf("\n");
                    }
                    UnmapViewOfFile(p);
                }
                CloseHandle(h);
            } else {
                printf("  miss: %s (err=%d)\n", names[i], GetLastError());
            }
        }
        
        // Also check if the original wdm2vst.dll is loaded by checking for its shared memory
        // Try alternate naming patterns
        char altName[256];
        for (int dev = 0; dev < 4; dev++) {
            sprintf(altName, "Global\\{Wdm2VST88EDC269-54DF-428C-BA96-F2DA06BC7BB1}_%d", dev);
            HANDLE h = OpenFileMappingA(FILE_MAP_READ, FALSE, altName);
            if (h) {
                printf("  FOUND: %s\n", altName);
                CloseHandle(h);
            }
            sprintf(altName, "Global\\Wdm2Vst_%d", dev);
            h = OpenFileMappingA(FILE_MAP_READ, FALSE, altName);
            if (h) {
                printf("  FOUND: %s\n", altName);
                CloseHandle(h);
            }
        }
        
        Sleep(2000);
    }
    return 0;
}
