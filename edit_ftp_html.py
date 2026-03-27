import ftplib
import re

print("Connecting to FTP...")
ftp = ftplib.FTP(timeout=15)
ftp.connect('103.80.27.48', 21)
ftp.login('geek_asmrtop_cn', '5yifcKJkbKefm4JR')
ftp.set_pasv(True)

remote_file = 'wdm2vst.html'
local_file = 'wdm2vst.html'

print("Downloading from FTP...")
with open(local_file, 'wb') as f:
    ftp.retrbinary(f'RETR {remote_file}', f.write)

print("Applying modifications...")
with open(local_file, 'r', encoding='utf-8', errors='ignore') as f:
    content = f.read()

# Replace version string 
content = re.sub(r'v[23]\.\d+\.\d+ · Public Edition', r'V3.2.1 · Public Edition', content, flags=re.IGNORECASE)
content = re.sub(r'WDM2VST_v[23]\.\d+\.\d+_Setup\.exe', r'WDM2VST_Public_v3.2.1_Setup.exe', content, flags=re.IGNORECASE)
content = re.sub(r'WDM2VST_Public_v[23]\.\d+\.\d+_Setup\.exe', r'WDM2VST_Public_v3.2.1_Setup.exe', content, flags=re.IGNORECASE)

# Update the changelog header and button text
# First replace all explicit 3.2.0 to 3.2.1
content = re.sub(r'V3\.2\.0 稳定版 更新日志', r'V3.2.1 稳定版 更新日志', content, flags=re.IGNORECASE)
content = re.sub(r'下载最新 V3\.2\.0', r'下载最新 V3.2.1', content, flags=re.IGNORECASE)
content = re.sub(r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3\.2\.0"', r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3.2.1"', content, flags=re.IGNORECASE)

# Then fix the collateral damage where historical block got renamed to 3.2.1 by targeting the specific historical span class and reverting it back to 3.2.0
content = re.sub(r'V3\.2\.1 稳定版 更新日志( <span style="font-size:12px;color:#777;font-weight:600;margin-left:8px">\(历史大版本\))', r'V3.2.0 稳定版 更新日志\1', content)
content = re.sub(r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3\.\d+\.\d+"', r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3.2.1"', content, flags=re.IGNORECASE)

# Insert the new comprehensive changelog about double playback and tmp logic 
new_changelog = """    <ul>
      <li><strong>彻底屏蔽双重播放 Bug</strong>: 完美修复在 DAW 加载 VST2WDM 插件并启用 IPC 时，底层物理设备依然会并发播放从而产生幽灵重音的历史遗留故障（通过重构端点释放机制实现）。</li>
      <li><strong>全套工程构建链重装上阵</strong>: 剔除了之前臃肿的 CMake 冗余目录层，全维清剿因 MSBuild 散落遗留污染引发的嵌套崩溃；直接产出极致轻量的标准 VST3 包。</li>
      <li><strong>安装目录 100% 纯净洗地</strong>: 重写 Inno Setup 安装包内核，引入 <code>{tmp}</code> 动态缓存提取技术。所有注册表和批处理配置杂乱文件会在驱动安装成功的瞬间自动全数自毁，保证 Program Files 绝对干净。</li>
      <li><strong>内核级 DPF 状态机诊断</strong>: 底层 WaveRT 音频流引擎全新注入无损状态机连线日志，精准解决令人头秃的系统级“采样率不匹配导致的各类鬼畜爆音”。</li>
      <li><strong>Code 37 阻断拦截引导防护</strong>: 原生针对微软最新安全系统严格拦截未签名驱动（设备管理器报 37 / 52 错）问题提供即时图形化弹窗避障指示：直接关闭内存完整性进行恢复。</li>
    </ul>"""
content = re.sub(r'<ul>.*?<li>.*?</li>.*?</ul>', new_changelog, content, flags=re.IGNORECASE | re.DOTALL)

update_block = """    <div class="step"><div><h4>版本更新检测</h4><p>内置自动化版本侦测系统。当检测到新版本时，UI自动变形展现极客风格呼吸幻彩按钮，全自动重定向至最新发布页。</p></div></div>
    <div class="step"><div><h4>内核级遥测监控 (Telemetry)</h4><p>首创诊断式宿主音频配置收集，实时协助排查因未知原因引发的爆音问题。</p></div></div>
    <div class="step"><div><h4>⚠️安装异常修复手册</h4><p>若安装后驱动提示设备异常（代码37/52），请关闭 Windows 安全中心 -> 设备安全性中的【内存完整性】后重启即可。</p></div></div>"""

if "版本更新检测" not in content:
    content = content.replace('<div class="steps">', '<div class="steps">\n' + update_block)

print("Uploading back to FTP...")
with open(local_file, 'w', encoding='utf-8') as f:
    f.write(content)

with open(local_file, 'rb') as f:
    ftp.storbinary(f'STOR {remote_file}', f)

ftp.quit()
print("Website updated and deployed on FTP.")
