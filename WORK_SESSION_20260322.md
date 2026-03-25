# WDM2VST 开发工作日志 - 2026-03-22

> [!IMPORTANT]
> 蓝屏恢复参考文档。如果蓝屏重启后看到这个文件，请直接继续下面的"当前状态"部分。

---

## 一、已完成工作

### 1. 驱动安装（4通道 WDM2VST）
- 使用原厂 WHQL 签名的 `wdm2vst.sys` / `wdm2vst.inf` / `wdm2vst.cat`
- 通过 `devcon.exe install` 安装了 **4 个实例**
- 驱动节点: `ROOT\MEDIA\0002` ~ `ROOT\MEDIA\0005`
- 硬件 ID: `*WDM2VST`
- 注册表初始化: `HKLM\SOFTWARE\ODeusAudio\Wdm2Vst\NumDevices=1, NumChannels=8`

### 2. 设备改名（公开版 Virtual Bridge）
- **方法**: 通过 Windows COM API (`IMMDevice::OpenPropertyStore(STGM_READWRITE)`)
- **原因**: MMDevices 注册表被系统 ACL 保护，`reg add` 和 PowerShell 都报"拒绝访问"
- **工具**: `d:\Autigravity\wdm2vst\rename_endpoints.exe` (C++ 编译)
  - 源码: `d:\Autigravity\wdm2vst\rename_endpoints.cpp`
- **改名结果**:
  - Render: Virtual Bridge 1~4
  - Capture: Virtual Bridge Mic 1~4
- **三个属性全改**:
  - `PKEY_Device_FriendlyName` ({a45c254e-df1c-4efd-8020-67d146a850e0},14)
  - `PKEY_DeviceInterface_FriendlyName` ({b3f8fa53-0004-438e-9003-51a46e139bfc},6)
  - `PKEY_Device_DeviceDesc` ({a45c254e-df1c-4efd-8020-67d146a850e0},2)
- **注意**: 第二行设备名是通过 `HKLM\SYSTEM\CurrentControlSet\Enum\ROOT\MEDIA\000X\FriendlyName` 修改

### 3. 安装包打包
- **公开版安装包**: `d:\Autigravity\wdm2vst\VirtualBridge_4CH_Installer.exe` (285KB)
  - 也复制到: `E:\Antigravity\成品开发\VirtualBridge_4CH_Installer.exe`
- **打包方式**: `iexpress.exe` + SED 配置文件
- **SED 文件**: `d:\Autigravity\wdm2vst\installer_public.sed`
- **打包源目录**: `d:\Autigravity\wdm2vst\PackagePublic\`
  - 含: wdm2vst.sys, wdm2vst.inf, wdm2vst.cat, devcon.exe, rename_endpoints.exe, install.bat
- **安装流程**: 注册表初始化 → devcon×4 → 等待6秒 → rename_endpoints.exe → FriendlyName修改 → 重启音频服务

### 4. 成品目录结构
```
E:\Antigravity\成品开发\
├── VirtualBridge_4CH_Installer.exe       # 公开版安装包
├── WDM2VST_Bridge_公开版\               # 公开版散装文件
│   ├── wdm2vst.sys/inf/cat
│   ├── devcon.exe
│   ├── rename_endpoints.exe/cpp
│   ├── install.bat
│   └── VirtualBridge_4CH_Installer.exe
└── WDM2VST_Bridge_品牌版\               # 品牌版（ASMR Bridge）
    ├── wdm2vst.sys/inf/cat
    ├── devcon.exe
    ├── rename_endpoints.cpp              # ASMR Bridge 名称版本
    ├── rename_endpoints_brand.cpp
    └── install.bat
```
> 品牌版的 rename_endpoints.exe 还没编译（之前编译环境卡死了）

### 5. VST 插件多通道支持代码改动

#### 修改的文件:
| 文件 | 关键改动 |
|---|---|
| `ODeusVadBridge.h` | `open(deviceIndex)` + `enumerateVadDevices()` 枚举全部设备 |
| `VadBridgeMode.h` | `bridge.open(deviceIndex)` 传参 + 匹配 Virtual Bridge / ASMR Bridge |
| `W2V_PluginProcessor.h/.cpp` | `enableVadBridgeMode(name, deviceIndex)` |
| `W2V_PluginEditor.cpp` | 提取 deviceIndex 从 itemId (8000+i) + WASAPI 分类新名称 |

#### 数据流:
```
下拉选择 "Virtual Bridge 2 [VAD]" (id=8001)
  → deviceIndex = 1
  → enableVadBridgeMode("...", 1)
  → VadBridgeMode::start(Capture, 1)
  → bridge.open(1)
  → enumerateVadDevices() → 取 devices[1].interfacePath
  → CreateFileA() → attach() → startTransfer() → pollLoop()
```

---

## 二、命名方案

| 版本 | 扬声器 | 麦克风 |
|---|---|---|
| **品牌版 (ASMRTOP)** | ASMR Bridge 1~4 | ASMR Bridge Mic 1~4 |
| **公开版 (通用)** | Virtual Bridge 1~4 | Virtual Bridge Mic 1~4 |

---

## 三、当前状态

- ✅ 本机已安装 4 通道 WDM2VST，已改名为 Virtual Bridge
- ✅ 安装包已打包 (VirtualBridge_4CH_Installer.exe)
- ✅ VST 插件代码已修改支持多通道设备发现
- ⏳ **待测试**: 插件编译 + 连接 4 个驱动通道
- ⏳ **待完成**: 品牌版 rename_endpoints.exe 编译
- ⏳ **待完成**: 品牌版安装包打包

---

## 四、关键文件路径

### 驱动文件
- `d:\Autigravity\wdm2vst\PackageB\` - 原始驱动文件
- `d:\Autigravity\wdm2vst\PackagePublic\` - 公开版打包源

### 工具
- `d:\Autigravity\wdm2vst\rename_endpoints.exe` - 改名工具（已编译）
- `d:\Autigravity\wdm2vst\rename_endpoints.cpp` - 公开版源码
- `d:\Autigravity\wdm2vst\list_audio.exe` - 音频设备诊断工具

### 插件源码
- `d:\Autigravity\wdm2vst\ASMRTOP_Plugins\Source\ODeusVadBridge.h` - 设备桥接（已改）
- `d:\Autigravity\wdm2vst\ASMRTOP_Plugins\Source\VadBridgeMode.h` - VAD桥接模式（已改）
- `d:\Autigravity\wdm2vst\ASMRTOP_Plugins\Source\W2V_PluginProcessor.h/.cpp` - 处理器（已改）
- `d:\Autigravity\wdm2vst\ASMRTOP_Plugins\Source\W2V_PluginEditor.cpp` - 编辑器UI（已改）

### 原厂参考
- `C:\Program Files\VSTPlugins\Tools\wdm2vst.dll` - ODeus 原厂 VST2 插件
- 共享内存名: `Global\{Wdm2VST88EDC269-54DF-428C-BA96-F2DA06BC7BB1}`
- 注册表: `Software\ODeusAudio\Wdm2Vst`
- IOCTL 协议: 0x9C402410 (Attach) / 0x9C402414 (Detach) / 0x9C402420 (Start) / 0x9C402428 (Transfer)
- Magic: 0x0131CA08

### 安装/卸载
- `d:\Autigravity\wdm2vst\installer_public.sed` - IExpress 配置
- `d:\Autigravity\wdm2vst\uninstall_all.bat` - 全部卸载脚本

---

## 五、已知风险

> [!CAUTION]
> - 原厂 asiovadpro.sys 驱动 **不支持 Overlapped I/O**，使用 `FILE_FLAG_OVERLAPPED` + `CancelIoEx` 会导致 BSOD
> - 所有 IOCTL 必须在**工作线程**上同步调用
> - Transfer 缓冲区必须 >= 0x54C (1356) 字节，否则 PAGE_FAULT → BSOD
> - MMDevices 注册表受 ACL 保护，只能通过 COM API 修改
