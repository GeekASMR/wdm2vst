#include <windows.h>
#include <iostream>
#include <string>
#include <atomic>

struct IpcAudioBuffer {
    std::atomic<uint32_t> writePos;
    std::atomic<uint32_t> readPos;
    float ringL[131072];
    float ringR[131072];
};

int main() {
    HANDLE hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "Global\\AsmrtopWDM_0");
    if(hMap) {
        IpcAudioBuffer* pBuf = (IpcAudioBuffer*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sizeof(IpcAudioBuffer));
        if (pBuf) {
            std::cout << "Monitoring writePos... Play audio now!" << std::endl;
            for (int i=0; i<30; ++i) {
                std::cout << "writePos: " << pBuf->writePos << std::endl;
                Sleep(200);
            }
        }
        CloseHandle(hMap);
    } else {
        std::cout << "Failed Global\\AsmrtopWDM_0 error: " << GetLastError() << std::endl;
    }
    return 0;
}
