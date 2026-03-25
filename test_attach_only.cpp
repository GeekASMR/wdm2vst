// test_attach_only.cpp - 只测试 Open + Attach + Detach + Close
// 不显示设备列表，直接使用硬编码的 wave1 路径
#include <windows.h>
#include <stdio.h>

#define IOCTL_ATTACH   0x9C402410
#define IOCTL_DETACH   0x9C402414
#define IOCTL_START    0x9C402420
#define IOCTL_TRANSFER 0x9C402428
#define MAGIC          0x0131CA08
#define XFER_SIZE      0x188

struct Buf { DWORD magic, p1, p2; };

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    
    // wave1 endpoint path on this machine
    const char* path = "\\\\?\\root#media#0003#{6994ad04-93ef-11d0-a3cc-00a0c9223196}\\wave1";
    if (argc > 1) path = argv[1];
    
    DWORD pid = GetCurrentProcessId();
    printf("PID=%d\n", pid);
    
    // 1. Open
    printf("[1] Opening %s ...\n", path);
    HANDLE h = CreateFileA(path, GENERIC_READ|GENERIC_WRITE, 
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        printf("  FAIL err=%d\n", GetLastError());
        return 1;
    }
    printf("  OK handle=%p\n", h);
    
    // 2. Attach
    printf("[2] Attach (magic=0x%08X clientId=%d mode=0)...\n", MAGIC, pid);
    Buf in = {MAGIC, pid, 0}, out = {};
    DWORD ret = 0;
    BOOL ok = DeviceIoControl(h, IOCTL_ATTACH, &in, 12, &out, 12, &ret, NULL);
    printf("  Result: %s err=%d bytes=%d\n", ok?"OK":"FAIL", GetLastError(), ret);
    if (ok && ret >= 12)
        printf("  Output: magic=0x%08X version=%d status=%d\n", out.magic, out.p1, out.p2);
    
    if (!ok) { CloseHandle(h); return 1; }
    
    // 3. Start Transfer
    printf("[3] Start Transfer...\n");
    Buf start_in = {MAGIC, pid, 0};
    ret = 0;
    ok = DeviceIoControl(h, IOCTL_START, &start_in, 12, NULL, 0, &ret, NULL);
    printf("  Result: %s err=%d\n", ok?"OK":"FAIL", GetLastError());
    
    // 4. One transfer
    if (ok) {
        printf("[4] Transfer (buf=%d bytes)...\n", XFER_SIZE);
        BYTE buf[XFER_SIZE] = {};
        ret = 0;
        ok = DeviceIoControl(h, IOCTL_TRANSFER, buf, XFER_SIZE, buf, XFER_SIZE, &ret, NULL);
        printf("  Result: %s err=%d bytes=%d\n", ok?"OK":"FAIL", GetLastError(), ret);
        if (ok) {
            printf("  First 16 bytes: ");
            for (int i = 0; i < 16 && i < (int)ret; i++) printf("%02X ", buf[i]);
            printf("\n");
        }
    }
    
    // 5. Detach
    printf("[5] Detach...\n");
    Buf det = {MAGIC, pid, 0};
    ret = 0;
    DeviceIoControl(h, IOCTL_DETACH, &det, 12, NULL, 0, &ret, NULL);
    printf("  err=%d\n", GetLastError());
    
    CloseHandle(h);
    printf("[6] Done.\n");
    return 0;
}
