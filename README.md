<p align="center">
  <img src="https://img.shields.io/badge/平台-Windows%2010%2F11-0078D6?style=flat-square&logo=windows" />
  <img src="https://img.shields.io/badge/格式-VST3-FF6600?style=flat-square" />
  <img src="https://img.shields.io/github/v/release/GeekASMR/wdm2vst?style=flat-square&color=00BCD4&label=最新版本" />
  <img src="https://img.shields.io/github/downloads/GeekASMR/wdm2vst/total?style=flat-square&color=4CAF50&label=总下载量" />
</p>

<h1 align="center">🎛️ WDM2VST 虚拟音频路由插件套件</h1>

<p align="center">
  <b>Windows 系统音频 (WDM/WASAPI) 与 VST3 宿主之间的无损音频桥接方案</b><br/>
  免驱动安装 · 亚毫秒级延迟 · 局域网串流 · 手机实时监听
</p>

---

## ✨ 核心功能

### 🔊 四大插件

| 插件 | 方向 | 说明 |
|------|------|------|
| **WDM2VST** | 系统 → DAW | 将 Windows 系统音频实时捕获到 DAW |
| **VST2WDM** | DAW → 系统 | 将 DAW 音频输出到任意系统音频设备 |
| **Audio Send** | 插件 → 插件 | 跨轨道 / 跨插件 / 跨 DAW 实例的点对点音频发送 |
| **Audio Receive** | 插件 → 插件 | 点对点音频接收，自适应抖动控制 |

### ⚡ 三种传输模式

```
┌─────────────────────────────────────────────────────────────────┐
│  共享内存 (IPC)          ■■■■■■■■■■■■■  ~0.5ms   本机传输     │
│  UDP 数据报              ■■■■■■■■■■     ~2ms     局域网传输   │
│  WebSocket (二进制)      ■■■■■■         ~80ms    手机/网页    │
└─────────────────────────────────────────────────────────────────┘
```

- **共享内存 (IPC)** — 通过 Windows 命名共享内存实现无锁环形缓冲，亚毫秒级零拷贝音频桥接
- **UDP** — 自定义轻量协议，12 字节包头 (`ASMR` 标识)，自动分片适配 1500B MTU，自适应抖动缓冲维持 ≤1.5 buffer 超低延迟
- **WebSocket** — 内置 HTTP + WebSocket 服务器，内嵌 HTML5 监听页面，手机浏览器直连即可实时监听波形、电平表和音量控制

### 📱 手机监听

在手机浏览器打开 `http://<电脑IP>:18820` 即可——无需安装任何 App

- 实时立体声波形显示
- L/R 电平表
- 音量滑块 & 静音开关
- 自动采样率重采样（如 48kHz DAW → 44.1kHz 手机）
- 中文 / 英文界面切换
- iOS & Android 均支持（基于 Web Audio API）

---

## 📦 安装方式

### 一键安装（推荐）

1. 从 [Releases 页面](https://github.com/GeekASMR/wdm2vst/releases) 下载 `WDM2VST_Public_vX.X.X_Setup.exe`
2. **以管理员身份**运行安装程序
3. 在 DAW 中扫描 VST3 插件
4. 完成！🎉

### 安装路径

```
VST3 插件：  C:\Program Files\Common Files\VST3\VirtualAudioRouter\
音频驱动：    通过 PnP 自动安装 (VirtualAudioRouter.sys)
```

### 系统要求

- Windows 10 / 11（64 位）
- 任意 VST3 兼容 DAW（Studio One、FL Studio、Cubase、Reaper、Ableton Live 等）

> ⚠️ **重要提示**：如果安装后设备管理器报错 Code 37 或 Code 52，请进入 **Windows 安全中心 → 设备安全性 → 内核隔离**，关闭【**内存完整性**】后重启电脑即可。

---

## 🚀 快速上手

### 📥 捕获系统声音到 DAW

1. 在音频轨道上插入 **WDM2VST**
2. 选择虚拟音频通道（1/2、3/4 等）
3. 播放系统音频——声音自动出现在 DAW 中 🔊

### 📤 将 DAW 音频输出到系统设备

1. 在轨道或总线上插入 **VST2WDM**
2. 选择目标输出设备
3. DAW 音频现在通过系统设备播放 🎧

### 🔀 跨轨道 / 跨 DAW 路由

1. 在源轨道插入 **Audio Send** → 点击 **启动**
2. 在目标轨道插入 **Audio Receive** → 点击 **启动**
3. 音频自动流通，延迟 <2ms ⚡

### 📱 手机监听

1. 在 **Audio Send**（或 **VST2WDM**）上启用 WebSocket
2. 手机浏览器打开 `http://<电脑IP>:18820`
3. 随时随地监听你的混音 📱

---

## 🏗️ 系统架构

```
┌────────────────────────────────────────────────────────────┐
│                      DAW（VST3 宿主）                      │
│                                                            │
│  ┌──────────┐  共享内存   ┌──────────────────┐              │
│  │ WDM2VST  │◄══════════►│  内核驱动         │              │
│  │ (接收端) │  IPC 环形   │  VirtualAudio    │              │
│  └──────────┘  缓冲区     │  Router.sys      │              │
│                           │  (WDM/PortCls)   │              │
│  ┌──────────┐  共享内存   │                  │              │
│  │ VST2WDM  │══════════►│                  │              │
│  │ (发送端) │  IPC 环形   └──────────────────┘              │
│  └──────────┘  缓冲区                                      │
│                                                            │
│  ┌──────────┐ IPC/UDP/WS ┌──────────┐                      │
│  │  Send    │═══════════►│ Receive  │  P2P 点对点传输       │
│  └──────────┘            └──────────┘                      │
│                    │                                       │
│                    │ WebSocket                              │
│                    ▼                                       │
│              ┌──────────┐                                  │
│              │ 📱 手机  │  HTML5 实时监听                   │
│              └──────────┘                                  │
└────────────────────────────────────────────────────────────┘
```

---

## 🔧 技术细节

### IPC 环形缓冲区

- **大小**：131,072 采样点（2^17）/ 每通道
- **格式**：32 位浮点，立体声（L/R 独立环形缓冲）
- **同步方式**：无锁原子操作 `writePos` / `readPos`
- **延迟模式**：可选 1ms（极限）到 24ms（安全）

### UDP 数据包协议

| 字段 | 大小 | 说明 |
|------|------|------|
| Magic | 4B | `ASMR` 标识符 |
| 通道 ID | 1B | 0-7 路由通道 |
| 声道数 | 1B | 1=单声道, 2=立体声 |
| 采样率 | 2B | 最高 65535 Hz |
| 序列号 | 2B | 循环计数器 |
| 帧大小 | 2B | 每通道采样数 |
| 音频数据 | 可变 | 32 位浮点 PCM |

单包最大 1452 字节（180 立体声采样 + 12B 包头 < 1500 MTU）

### 自适应抖动控制（v3.2.12+）

- 目标缓冲深度 = 1.5 × DAW buffer size
- 超出 3× 自动快进丢弃，防止延迟堆积
- 首次连接直接定位到最低延迟位置，避免"先静音后爆发"

### 防宿主休眠（DAW 兼容性）

所有插件注入不可听 DC 偏移（`1e-7f` ≈ -140dBFS），防止 Studio One 等 VST3 宿主触发插件休眠 / 静音检测。配合 `getTailLengthSeconds() = 99999.0` 实现最大宿主兼容性。

---

## 📋 更新日志

完整版本历史请查看 [Releases 页面](https://github.com/GeekASMR/wdm2vst/releases)。

### v3.2.12（最新版）
- ⚡ P2P Send/Recv 自适应抖动控制——目标缓冲深度 ≤1.5 buffer
- 🔧 首次连接智能定位——不再"先静音后爆发"
- 🛡️ 全插件增强 Studio One 防休眠机制

### v3.2.11
- 🛡️ 修复 Studio One 插件休眠问题（加强 DC offset）
- 📦 全部 4 个插件统一注入防休眠信号

---

## 📄 许可证

本软件为专有软件，详见安装程序中包含的许可条款。

## 🔗 相关链接

- **下载安装**：[GitHub Releases](https://github.com/GeekASMR/wdm2vst/releases)
- **使用文档**：[geek.asmrtop.cn/wdm2vst](https://geek.asmrtop.cn/wdm2vst.html)
- **问题反馈**：[GitHub Issues](https://github.com/GeekASMR/wdm2vst/issues)

---

<p align="center">
  <sub>由 <b>ASMRTOP Studio</b> 用 ❤️ 打造 · 基于 JUCE 框架</sub>
</p>
