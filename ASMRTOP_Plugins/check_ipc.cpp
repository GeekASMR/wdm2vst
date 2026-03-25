#include <windows.h>  
#include <iostream>  
#include <atomic>  
struct IpcAudioBuffer { std::atomic<uint32_t> writePos; std::atomic<uint32_t> readPos; float ringL[131072]; float ringR[131072]; }; int main() { HANDLE hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "Global\\AsmrtopWDM_0"); if(!hMap) { std::cout << "Failed to open Global\\AsmrtopWDM_0, error: " << GetLastError() << std::endl; return 1; } std::cout << "Successfully opened mapping!" << std::endl; IpcAudioBuffer* pBuf = (IpcAudioBuffer*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sizeof(IpcAudioBuffer)); if(!pBuf) { std::cout << "Failed to map view" << std::endl; return 1;} std::cout << "writePos: " << pBuf->writePos << ", readPos: " << pBuf->readPos << std::endl; return 0; }  
