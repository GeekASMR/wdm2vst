# WDM 虚拟驱动 IPC Ring Buffer 采样率问题报告

## 📌 问题概述

在将 ASMRTOP/VirtualAudio WDM 虚拟驱动与 Behringer UMC ASIO 驱动集成时，发现 
ring buffer 的写入速率**始终为 48000/s**，即使 Windows 声音面板设置为 44100Hz。

### ⚠️ 根因已确认（非 WDM 驱动 BUG）

经源码分析确认：**这是 Windows Audio Engine 的正常行为**，不是驱动代码问题。

`speakerwavtable.h` 中硬编码了 `SPEAKER_HOST_MIN_SAMPLE_RATE = 48000`，
导致 Windows 共享模式混音器始终以 ≥48000Hz 发送给驱动。驱动收到的 `pWfEx->nAvgBytesPerSec` 
是已经被 Windows 重采样后的 48000Hz 数据。

**采用方案 A：锁定 48000Hz，ASIO 侧做 SRC 补偿 USB 硬件时钟差异。**

### 测试环境

| 组件 | 版本/信息 |
|------|----------|
| WDM 虚拟驱动 | VirtualAudioDriver (公开版) |
| ASIO 驱动 | Behringer UMC ASIO v6 (Native Direct) |
| USB 声卡 | Behringer UMC (VID=0x1397 PID=0x0503) |
| 操作系统 | Windows 10 (19045) |
| DAW | Studio One |

### 集成架构

```
Windows 应用 → WDM 虚拟驱动 → SharedMemory Ring Buffer ← ASIO 驱动 → USB 硬件
                (PLAY ring)     (IpcAudioBuffer)          (读取)        (DMA 输出)
```

ASIO 驱动在 `onBufferSwitch` 回调中从 WDM 的 PLAY ring buffer 读取音频数据，
作为虚拟输入通道 (VRT IN 1-8) 呈现给 DAW。

### 测试工具

**ipc_monitor.exe** — 实时监控 ring buffer 读写状态

源码位置: `D:\Autigravity\UMCasio\tools\ipc_monitor.cpp`

功能:
- 连接所有 PLAY_0~3 和 CAP_0~3 共享内存段
- 每秒打印 writePos, readPos, avail
- 计算实际写入速率 (Wr) 和读取速率 (Rd) (样本/秒)
- 自动检测跨品牌名称 (AsmrtopWDM / VirtualAudioWDM)

编译方式:
```bash
cd D:\Autigravity\UMCasio\build
cmake --build . --config Release --target ipc_monitor
# 产出: build\bin\Release\ipc_monitor.exe
```

### 测试流程

1. 安装 VirtualAudioDriver (公开版)
2. 在 Windows 声音设置中将 Virtual 1/2 设置为指定采样率
3. 用 Windows 应用 (如浏览器/音乐播放器) 播放音频到 Virtual 1/2
4. 打开 Studio One 选择 UMC ASIO 驱动
5. 运行 `ipc_monitor.exe` 监控 ring buffer
6. 观察 Wr (写入速率) 和 Rd (读取速率) 是否匹配

---

## 🐛 核心问题

### 问题: WritePcmToRing 写入速率不跟随 endpoint 采样率设置

**现象**: 
Windows 声音设置中将 Virtual 1/2 的默认格式改为 **44100 Hz**，
但 ipc_monitor 监控显示 ring buffer 的写入速率仍然是 **48000 样本/秒**。

**证据** (ipc_monitor 输出):

```
设置: Virtual 1/2 → 44100 Hz, 32-bit

Time   | PLAY0_W      PLAY0_R      Avail    Wr       Rd
-------+------------------------------------------------
t=0    | W=32200080   R=32157352   A=42728  Wr=48000 Rd=46368  ← 写入48000, 不是44100!
t=1    | W=32248128   R=32203586   A=44542  Wr=48048 Rd=46234
t=2    | W=32296176   R=32249954   A=46222  Wr=48048 Rd=46368
t=3    | W=32344224   R=32296322   A=47902  Wr=48048 Rd=46368
...
t=19   | W=33112704   R=33037672   A=75032  Wr=48000 Rd=46368  ← Avail持续增长!
```

**关键数据**:
- `Wr` 始终 ≈ **48000**/s (而非配置的 44100)
- ASIO 侧 DMA 以 44100Hz 回调, 即使做了 SRC 补偿 (8.84%), 
  `Rd` 只能到 ~46368/s, 不够追上 48000/s 的写入
- `Avail` 每秒增长 ~1632 样本, ring buffer 持续积水

**后果**:
1. ⚠️ 音频播放速度变慢 (因为消费速度 < 生产速度)
2. ⚠️ 声音出现杂音/电音, 且**播放时间越久越严重**
3. ⚠️ ring buffer 最终溢出, 导致音频跳帧

### 对比: 正常工作时的数据

当采样率匹配时 (均为 48000Hz), ring buffer 完全稳定:

```
Time   | PLAY0_W      PLAY0_R      Avail    Wr       Rd
-------+------------------------------------------------
t=17   | W=40153056   R=40152798   A=258    Wr=48048 Rd=47992
t=18   | W=40201008   R=40200883   A=125    Wr=47952 Rd=48085
t=19   | W=40249104   R=40248947   A=157    Wr=48096 Rd=48064
t=20   | W=40297152   R=40297010   A=142    Wr=48048 Rd=48063
```

- `Avail` 稳定在 100~260 范围
- `Wr ≈ Rd ≈ 48000`
- 声音正常, 无杂音

---

## 📋 建议修复方向

### WDM 驱动侧 (VirtualAudioDriver)

1. **检查 `minwavertstream.cpp` 中的 `WritePcmToRing` 调用链**:
   - 确认当 Windows 共享模式格式切换到 44100Hz 后,
     PortCls/WASAPI 传入的 PCM 数据速率是否真的变了
   - 还是 WDM 驱动始终内部处理为 48000Hz?

2. **检查 endpoint 格式协商逻辑**:
   - `IMiniportWaveRT::NewStream` 是否正确响应格式变化?
   - Stream 的 `SetFormat` 是否传递到实际的采样处理?

3. **可能的根因**:
   - WDM 驱动的 DPC 定时器周期可能硬编码为 48000Hz
   - 或者 PortCls 的共享模式混音器在驱动之前做了重采样
   - Ring buffer 写入逻辑可能没有考虑不同采样率

### ASIO 驱动侧 (BehringerASIO)

已实现的:
- ✅ 从 PLAY ring buffer 读取虚拟通道音频
- ✅ Hermite 4点插值 SRC (待采样率问题解决后作为保险)
- ✅ 自适应速率追踪 (来自 ASMRTOP_Plugins 参考)
- ✅ ipc_monitor 诊断工具

待解决:
- ⚠️ `setSampleRate(48000)` 没有真正改变 USB 硬件时钟
  (硬件始终运行在 44100Hz, 这是 ASIO 驱动的独立问题)

---

## 🛠️ 复现步骤

1. 安装 VirtualAudioDriver 公开版
2. Windows 声音 → Virtual 1/2 → 高级 → 设置为 **2通道, 32位, 44100Hz**
3. 编译运行 ipc_monitor:
   ```
   cd D:\Autigravity\UMCasio\build
   cmake --build . --config Release --target ipc_monitor
   .\bin\Release\ipc_monitor.exe
   ```
4. 在 Windows 播放音频到 Virtual 1/2
5. 观察 `Wr` 列 — 应该是 44100 但实际显示 48000

---

## 📁 相关文件

| 文件 | 说明 |
|------|------|
| `D:\Autigravity\UMCasio\tools\ipc_monitor.cpp` | Ring buffer 监控工具源码 |
| `D:\Autigravity\UMCasio\src\bridge\AsmrtopIPC.h` | ASIO侧 IPC 读写实现 (含 Hermite SRC) |
| `D:\Autigravity\UMCasio\src\driver\BehringerASIO.cpp` | ASIO 驱动主文件 |
| `D:\Autigravity\wdm2vst\ASMRTOP_Plugins\Source\AsmrtopIPC.h` | 参考: 插件侧 IPC 实现 |
| `D:\Autigravity\wdm2vst\ASMRTOP_Plugins\Source\W2V_PluginProcessor.cpp` | 参考: 自适应读取逻辑 |
