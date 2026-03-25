#include <windows.h>
#include <iostream>
#include <string>

int main() {
    for(int i=0; i<10; ++i) {
        std::string name = "Global\\AsmrtopWDM_" + std::to_string(i);
        HANDLE hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, name.c_str());
        if(hMap) {
            std::cout << "Opened " << name << std::endl;
            CloseHandle(hMap);
        } else {
            std::cout << "Failed " << name << " error: " << GetLastError() << std::endl;
        }
    }
    return 0;
}
