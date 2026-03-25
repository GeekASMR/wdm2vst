# VAD 驱动调试工作报告

> 时间: 2026-03-21 17:23 ~ 18:54
> 对话 ID: c60b047e-535d-4099-ab91-83edd72ac74a

---

## 一、问题背景

用户报告 VST 插件中 VAD 设备不显示。系统中 ASIO Link Pro 驱动存在重复安装问题。

## 二、已完成的代码修复（VST 插件设备发现逻辑）✅

### 根因

`VadBridgeMode::probeDevices()` 和 `ODeusVadBridge::findAsioVadProDevice()` 通过检查**设备接口路径**中是否包含 `"asiovadpro"` 或 `"wdm2vst"` 来过滤设备。

但实际的设备接口路径是通用格式：
```
\\?\root#media#0003#{6994ad04-93ef-11d0-a3cc-00a0c9223196}\wave1
\\?\root#media#0004#{6994ad04-93ef-11d0-a3cc-00a0c9223196}\wave1
```

路径中**不包含** "asiovadpro" 或 "wdm2vst"，导致过滤永远不匹配。

### 修复方案

改为通过 `SetupDiGetDeviceRegistryPropertyA` 获取设备的**硬件 ID** (`SPDRP_HARDWAREID`) 和**友好名称** (`SPDRP_FRIENDLYNAME`) 来识别设备：
- 硬件 ID: `*ASIOVADPRO`, `*WDM2VST`
- 友好名称: `ASIOVADPRO Driver`, `WDM2VST Driver`

### 修改文件

1. **`ASMRTOP_Plugins/Source/VadBridgeMode.h`** — `probeDevices()` 函数
   - 传递 `SP_DEVINFO_DATA` 从 `SetupDiGetDeviceInterfaceDetailA`
   - 用 `SPDRP_HARDWAREID` / `SPDRP_FRIENDLYNAME` 代替路径字符串匹配
   - 增加 `seenDevInstIds` 去重逻辑（同一设备有多个接口端点 wave1/wave2/topology 等）
   - 用 `VAD_MAGIC_SIGNATURE` 和 `IOCTL_VAD_ATTACH` 常量代替硬编码值

2. **`ASMRTOP_Plugins/Source/ODeusVadBridge.h`** — `findAsioVadProDevice()` 函数
   - 同样改为通过硬件 ID 和友好名称识别设备
   - 增加 DBG 日志输出

### 编译部署

4 个 VST3 插件已全部编译（Release）并部署到 `C:\Program Files\Common Files\VST3\`：
- ASMRTOP WDM2VST.vst3
- ASMRTOP VST2WDM.vst3
- ASMRTOP Send.vst3
- ASMRTOP Receive.vst3

编译命令：
```
cmake --build d:\wdm2vst\ASMRTOP_Plugins\build --config Release --target ASMRTOP_WDM2VST_VST3 ASMRTOP_VST2WDM_VST3 ASMRTOP_SEND_VST3 ASMRTOP_RECV_VST3
```

---

## 三、驱动卸载（已完成 ✅）

### 卸载前状态
| 设备 | InstanceId | 硬件ID | 状态 |
|---|---|---|---|
| WDM2VST Driver | ROOT\MEDIA\0003 | *WDM2VST | OK |
| ASIOVADPRO Driver | ROOT\MEDIA\0004 | *ASIOVADPRO | OK |
| ASIOVADPRO Driver | ROOT\MEDIA\0005 | *ASIOVADPRO | OK (重复!) |

驱动包：oem65.inf (asiovadpro), oem30.inf (wdm2vst)

### 卸载步骤（需要管理员权限，当前终端已是管理员）
```powershell
# 移除设备节点
pnputil /remove-device "ROOT\MEDIA\0005" /subtree    # 成功
pnputil /remove-device "ROOT\MEDIA\0004" /subtree    # 需要重启标记
pnputil /remove-device "ROOT\MEDIA\0003" /subtree    # 成功

# 删除驱动包
pnputil /delete-driver oem65.inf /uninstall /force   # 成功
pnputil /delete-driver oem30.inf /uninstall /force   # 成功

# 删除服务注册表
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\asiovadpro" /f
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\wdm2vst" /f

# 删除驱动文件
Remove-Item 'C:\Windows\System32\drivers\asiovadpro.sys' -Force
Remove-Item 'C:\Windows\System32\drivers\wdm2vst.sys' -Force
```

注意：`Start-Process -Verb RunAs` 在此环境中不会正确传递管理员权限给子进程，需要直接在当前终端运行命令（终端已是管理员）。

---

## 四、驱动重装（部分完成 ⚠️）

### 已成功的步骤

1. **安装驱动包到驱动商店**:
   ```
   pnputil /add-driver "D:\link\asiolink_unpacked\x64\asiovadpro.inf" /install
   ```
   注册为 `oem30.inf`

2. **解决服务冲突**: 
   旧服务在 SCM 缓存中残留（reg delete 不清 SCM 内存缓存），导致 devcon install 失败（Error 2: 找不到文件）。
   ```
   sc.exe delete asiovadpro    # 从 SCM 清除
   sc.exe create asiovadpro type= kernel start= demand error= normal binpath= "System32\DRIVERS\asiovadpro.sys" DisplayName= "ASIOVADPRO"
   ```

3. **安装设备**:
   ```powershell
   & 'C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe' install 'D:\link\asiolink_unpacked\x64\asiovadpro.inf' '*ASIOVADPRO'
   # 输出: Drivers installed successfully.
   ```

### 遇到蓝屏及修复方案（2026-03-22 更新）

**发生蓝屏 (0x0000003b SYSTEM_SERVICE_EXCEPTION)**
由于此前强制卸载和仅挂载 `.sys` 文件，系统底层虽然加载了 `asiovadpro.sys`，但核心注册表参数（如 `NumDevices` 和 `NumChannels` 从 `HKLM\SOFTWARE\ODeusAudio\AsioLinkPro` 读取的环境）缺失或损坏。此状态导致驱动 DriverEntry 产生 `STATUS_INVALID_PARAMETER` 失败，且在处理音频系统请求时触发系统级内存越界访问，引起蓝屏重启。

**已执行并已完成的修复操作**：
1. **清理旧节点与服务**：
   ```powershell
   # 执行过以下命令进行垃圾清退
   devcon.exe remove '*ASIOVADPRO'     # 系统标记为 Removed on reboot
   pnputil /delete-driver <oem.inf>    # 清理相关 OEM 驱动库
   sc.exe delete asiovadpro            # 清除冲突的服务进程
   # 同步清理注册表内相关残留项与残留系统文件
   ```
2. **使用官方源文件复原**：
   已找到并启动源文件 `D:\link\ASIOLinkPro24CE.exe` 进行原生安装。此安装能自动写入必要注册表逻辑和外围库，让 `.sys` 环境完整。
3. **重启落实生效**：
   安装完成后必须由用户重启计算机，以清除刚被锁定并标记“Removed on reboot”引发故障的旧版驱动系统节点，正式激活新的健康驱动。

### 下一步跟进工作（重启后操作）

1. 重启之后，进入宿主体与系统声音面板，检查 `ASIOVADPRO Driver` 是否满血复活且稳定。
2. 打开宿主软件及前文刚刚编译出的 `ASMRTOP WDM2VST.vst3` 等插件，验证设备枚举与捕获不再报错。
3. ASIO 侧稳固后，同等方法推进我们专属开发的 `WDM2VST` 驱动安装。

---

## 五、系统关键路径参考

| 项目 | 路径/值 |
|---|---|
| 项目根目录 | `d:\wdm2vst` |
| VST 插件源码 | `d:\wdm2vst\ASMRTOP_Plugins\Source\` |
| 编译输出 | `d:\wdm2vst\ASMRTOP_Plugins\build\` |
| CMake 构建 | `d:\wdm2vst\ASMRTOP_Plugins\build\ASMRTOP_AUDIO_PLUGINS.sln` |
| VST3 部署目录 | `C:\Program Files\Common Files\VST3\` |
| ASIO Link Pro 安装包 | `D:\link\asiolink_unpacked\` (部分被卸载程序删除) |
| 驱动商店备份 | `C:\Windows\System32\DriverStore\FileRepository\asiovadpro.inf_amd64_b6974654a52821c5\` |
| devcon 工具 | `C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe` |
| 应用程序注册表 | `HKLM:\SOFTWARE\ODeusAudio\AsioLinkPro` (驱动 DriverEntry 读取) |
| 服务注册表 | `HKLM:\SYSTEM\CurrentControlSet\Services\asiovadpro` |
| 设备枚举注册表 | `HKLM:\SYSTEM\CurrentControlSet\Enum\ROOT\MEDIA\0003` |
| 安装日志 | `C:\Windows\INF\setupapi.dev.log` |
| 测试签名 | 已开启 (bcdedit testsigning Yes) |
| 驱动硬件 ID | `*ASIOVADPRO` (asiovadpro), `*WDM2VST` (wdm2vst) |
| GUID (KSCATEGORY_AUDIO) | {6994AD04-93EF-11D0-A3CC-00A0C9223196} |
| asiovadpro.sys MD5 | 8DE02193E178E94E2EF9E5B3449B0472 |
| asiovadpro.sys 大小 | 42984 bytes (signed by John Shield, expired 2016) |

## 六、其他驱动相关信息

### 系统中其他 VAD 驱动包
- `oem31.inf` → asmrtop_vad.inf (自己的 VAD 驱动, ASMRTOP_VAD_TestCert)
- `oem58.inf` → asmrtop_vad.inf (重复, ASMRTOP_VAD_TestCert)
- 这些是独立于 ASIO Link Pro 的，暂未处理

### ASIOVADPRO INF 结构
- 定义了 4 组 Wave/Topology 接口 (Wave1~4, Topology1~4)
- 每组支持 Render + Capture
- 使用 `AlsoInstall=ks.registration(ks.inf),wdmaudio.registration(wdmaudio.inf)`
- OEM 格式: 8 通道 16-bit 44100Hz

### asiovadpro.sys 中发现的关键字符串
```
ASIOVADPRO Adapter Driver
O Deus Audio
NumDevices
NumChannels
\Registry\Machine\Software\ODeusAudio\AsioLinkPro
```

---

## 七、IOCTL 模式修复（2026-03-22 会话）

> 对话 ID: 7bd897c3-196c-401f-bab4-1acc0af4cb54
> 时间: 2026-03-22 04:00 ~ 12:58

### 目标
修复 ASMRTOP VST 插件的 IOCTL 传输模式（VadBridgeMode），使其能安全地与 asiovadpro.sys 驱动通信。

### 蓝屏根因分析

通过反汇编 `asiolink.dll` 和 `asiovadpro.sys`，定位了蓝屏原因：

| 项目 | 我们的代码 (vad_monitor/ODeusVadBridge) | asiolink.dll (正常) |
|------|-------|---------|
| TRANSFER 缓冲区大小 | `0x188` (392字节) | `[buf+4] + 0x54C` (≥1356字节) |
| 驱动 memcpy 写入 | 0x3C4 字节到 offset 0x188 | 同上 |
| 结果 | 越界 → BSOD | 正常 |

**根本原因**: METHOD_BUFFERED IOCTL 的系统缓冲区只分配了 max(nIn, nOut) = 0x188 字节，但驱动在 offset 0x188 处写入 0x3C4 字节 → PAGE_FAULT_IN_NONPAGED_AREA (0x50)

### 缓冲区布局（逆向工程确认）

```
IOCTL_TRANSFER (0x9C402428) 缓冲区:
  Block A [0x000..0x187] = 0x188 bytes — Render 数据 (输入)
    [0..7] 头部 + [8..391] 24帧×8通道×16bit PCM
  Block B [0x188..0x54B] = 0x3C4 bytes — Capture 数据 (输出)
    驱动填充内部 DEVICE_EXTENSION 状态 (非直接PCM)
  总计: 0x54C (1356) bytes 最小
```

### 诊断工具运行结果 (test_transfer_dump.exe)

- Block A 在 TRANSFER 后完全未被驱动修改
- Block B 包含内核指针、"Io  " / "IoNm" 标记、WAVEFORMATEX 片段
- `bytesReturned = 0x54C` (1356) ✅ — 大小匹配
- **结论**: IOCTL_TRANSFER 不直接传输 PCM 音频，而是同步驱动内部状态。实际音频数据可能通过驱动内部的共享内存/环形缓冲区交换

### 修改的文件清单

#### 1. `ODeusVadBridge.h` — IOCTL 缓冲区大小修正
- `VAD_TRANSFER_BUF_SIZE`: `0x188` → `0x54C` (**防止蓝屏的关键修复**)
- 新增 `VAD_BLOCK_A_OFFSET`, `VAD_BLOCK_B_OFFSET` 等常量
- `readCapture()`: 同时检查 Block A 和 Block B 的 PCM 数据
- `writeRender()`: 使用 Block A offset 写入 render 数据

#### 2. `VadBridgeMode.h` — Poll 线程限速 + 静音跳过
- `pollLoop()`: 使用 QPC 精确计时，以 48kHz 速率限制 IOCTL 调用
- 静音检测: 当 `readCapture` 返回全零时不推进 writePos（防止 ring buffer 无限积压）
- `TARGET_SAMPLE_RATE = 48000.0`

#### 3. `W2V_PluginEditor.cpp` — 重新启用 VAD 设备探测
- 第 109 行: 从 `DISABLED` 改回启用，调用 `VadBridgeMode::probeDevices()`
- 在下拉列表中显示 `[VAD]` 标签的设备

#### 4. `W2V_PluginProcessor.cpp` — 采样率 + 诊断修正
- `enableVadBridgeMode()`: deviceSampleRate 从 44100 → 48000 (匹配 TARGET_SAMPLE_RATE)
- `getAvailableSamples()`: 新增 VAD 模式分支，使用 `vadBridge->getWritePos()` 而非 processor 自己的 writePos

### 驱动重装 (进行中，即将重启)

1. ✅ `devcon remove '*ASIOVADPRO'` — 设备节点已移除
2. ✅ `sc.exe delete asiovadpro` — 服务已删除
3. ✅ `pnputil /delete-driver oem75.inf /uninstall /force` — 驱动包已删除
4. ✅ 驱动文件 `asiovadpro.sys` 已删除
5. ✅ 启动了官方安装程序 `D:\link\ASIOLinkPro24CE.exe`
6. ⏳ **需要重启电脑** 使新驱动生效

### 重启后待办事项

1. 验证 ASIOVADPRO 驱动正常启动（`pnputil /enum-devices /class MEDIA`）
2. 打开 asiolinktool.exe 面板，确认声音路由正常
3. 在 VST 宿主中加载 ASMRTOP WDM2VST.vst3，选择 [VAD] 设备验证:
   - CACHED BUF 应该稳定在 0 附近（因为没有静音积压了）
   - 如果 asiolinktool.exe 面板在播放，可能能收到真实音频
4. 测试 IPC 模式（选择 [IPC] 设备）— 这是真正的 WDM2VST 核心功能
5. 如果 IOCTL 模式仍无法获取真实音频，需要进一步逆向 asiolink.dll 的共享内存机制

### 编译命令参考
```powershell
# 编译 WDM2VST + VST2WDM
cmake --build d:\Autigravity\wdm2vst\ASMRTOP_Plugins\build --config Release --target ASMRTOP_WDM2VST_VST3 ASMRTOP_VST2WDM_VST3

# 部署
Copy-Item "...\ASMRTOP WDM2VST.vst3" -Destination "C:\Program Files\Common Files\VST3\" -Recurse -Force
```
