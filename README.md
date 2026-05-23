> # 📦 此仓库已归档 / This repository is archived
>
> **WDM2VST 已重构为 [WDM2VST-Ultra](https://github.com/GeekASMR/WDM2VST-Ultra)，新版下载请前往：**
>
> ## 👉 [Releases · GeekASMR/WDM2VST-Ultra](https://github.com/GeekASMR/WDM2VST-Ultra/releases/latest)
>
> ---
>
> WDM2VST has been rewritten as [WDM2VST-Ultra](https://github.com/GeekASMR/WDM2VST-Ultra).
> Download the latest installer from the **[Releases page](https://github.com/GeekASMR/WDM2VST-Ultra/releases/latest)**.
>
> ---
>
> ### 为什么换仓库？/ Why a new repo?
>
> v1.0.0 起项目重命名为 `WDM2VST Ultra`，是一次较大的重构：
>
> - 新增 `INST WDM2VST Ultra`、`Send Ultra`、`Receive Ultra` 等新插件
> - 安装路径、文件名、内部 UID 全部变更（旧的 v3.2.x 可以与新版共存，但建议先卸载旧版）
> - 安装器改为 Tauri 自渲染 UI，体积更小、UX 更现代
>
> 老版本不再维护。本仓库**仅作为历史归档保留**，所有 v3.2.x 安装包仍可在下方 Releases 中下载（如果你确实需要老版本）。
>
> ### 升级步骤 / Upgrade
>
> 1. 卸载旧版：控制面板 → 程序与功能 → 删除 `WDM2VST 虚拟音频路由`
> 2. 下载新版：[WDM2VST_Ultra_v\<latest\>_Setup.exe](https://github.com/GeekASMR/WDM2VST-Ultra/releases/latest)
> 3. 以管理员身份运行安装器
> 4. 在 DAW 中重新扫描 VST3 插件
>
> 详细排查见新仓库 README。

---

# 🎛️ WDM2VST 虚拟音频路由插件套件 (v3.x · 已归档)

> 本节内容仅作历史归档参考，新用户请使用 [WDM2VST-Ultra](https://github.com/GeekASMR/WDM2VST-Ultra)。

WDM2VST 是 Windows 系统音频 (WDM/WASAPI) 与 VST3 宿主之间的无损音频桥接方案。免驱动安装、亚毫秒级延迟、局域网串流、手机实时监听。

### 旧版四大插件

| 插件 | 方向 | 说明 |
|------|------|------|
| WDM2VST | 系统 → DAW | 将 Windows 系统音频实时捕获到 DAW |
| VST2WDM | DAW → 系统 | 将 DAW 音频输出到任意系统音频设备 |
| Audio Send | 插件 → 插件 | 跨轨道 / 跨插件 / 跨 DAW 实时点对点音频发送 |
| Audio Receive | 插件 → 插件 | 点对点音频接收，自适应抖动控制 |

### 旧版安装路径

```
VST3 插件：  C:\Program Files\Common Files\VST3\VirtualAudioRouter\
音频驱动：    通过 PnP 自动安装 (VirtualAudioRouter.sys)
```

### 老版本下载

[Releases page (archived)](https://github.com/GeekASMR/wdm2vst/releases) — 最新一版是 `v3.2.17`。

---

<p align="center">
  <sub>已被 <a href="https://github.com/GeekASMR/WDM2VST-Ultra">WDM2VST-Ultra</a> 取代 · 由 <b>ASMRTOP Studio</b> 出品</sub>
</p>
