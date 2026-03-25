/*
 * ipc_monitor.cpp - 监控 ASMRTOP IPC ring buffer 状态
 * 
 * 每秒打印 writePos, readPos, avail 变化
 * 用于诊断虚拟通道速度问题
 */
#include <windows.h>
#include <iostream>
#include <atomic>
#include <cstdint>

#define IPC_RING_SIZE 131072

struct IpcAudioBuffer {
    std::atomic<uint32_t> writePos;
    std::atomic<uint32_t> readPos;
    float ringL[IPC_RING_SIZE];
    float ringR[IPC_RING_SIZE];
};

IpcAudioBuffer* openIpc(const char* direction, int id) {
    const char* brands[] = { "AsmrtopWDM", "VirtualAudioWDM" };
    const char* prefixes[] = { "Global\\", "" };
    char name[256];
    
    for (auto brand : brands) {
        for (auto prefix : prefixes) {
            snprintf(name, sizeof(name), "%s%s_%s_%d", prefix, brand, direction, id);
            HANDLE h = OpenFileMappingA(FILE_MAP_READ, FALSE, name);
            if (h) {
                auto* buf = (IpcAudioBuffer*)MapViewOfFile(h, FILE_MAP_READ, 0, 0, sizeof(IpcAudioBuffer));
                if (buf) {
                    printf("  Connected: %s\n", name);
                    return buf;
                }
                CloseHandle(h);
            }
        }
    }
    return nullptr;
}

int main() {
    printf("=== IPC Ring Buffer Monitor ===\n\n");
    
    // Try to open all channels
    IpcAudioBuffer* play[4] = {};
    IpcAudioBuffer* cap[4] = {};
    
    for (int i = 0; i < 4; i++) {
        printf("PLAY_%d: ", i);
        play[i] = openIpc("PLAY", i);
        if (!play[i]) printf("  Not found\n");
    }
    for (int i = 0; i < 4; i++) {
        printf("CAP_%d:  ", i);
        cap[i] = openIpc("CAP", i);
        if (!cap[i]) printf("  Not found\n");
    }
    
    printf("\nMonitoring... (Ctrl+C to stop)\n");
    printf("%-6s | %-12s %-12s %-8s %-10s | %-12s %-12s %-8s\n",
           "Time", "PLAY0_W", "PLAY0_R", "Avail", "Rate(w/s)", "CAP0_W", "CAP0_R", "Avail");
    printf("-------+----------------------------------------------+----------------------------------\n");
    
    uint32_t prevPlayW = play[0] ? play[0]->writePos.load() : 0;
    uint32_t prevPlayR = play[0] ? play[0]->readPos.load() : 0;
    uint32_t prevCapW = cap[0] ? cap[0]->writePos.load() : 0;
    
    for (int sec = 0; ; sec++) {
        Sleep(1000);
        
        if (play[0]) {
            uint32_t w = play[0]->writePos.load();
            uint32_t r = play[0]->readPos.load();
            int32_t avail = (int32_t)(w - r);
            uint32_t writeRate = w - prevPlayW;
            uint32_t readRate = r - prevPlayR;
            
            printf("t=%-4d | W=%-10u R=%-10u A=%-6d Wr=%u Rd=%u", 
                   sec, w, r, avail, writeRate, readRate);
            prevPlayW = w;
            prevPlayR = r;
        } else {
            printf("t=%-4d | PLAY0 N/A                           ", sec);
        }
        
        if (cap[0]) {
            uint32_t w = cap[0]->writePos.load();
            uint32_t r = cap[0]->readPos.load();
            int32_t avail = (int32_t)(w - r);
            printf(" | W=%-10u R=%-10u A=%d", w, r, avail);
        }
        printf("\n");
    }
    return 0;
}
