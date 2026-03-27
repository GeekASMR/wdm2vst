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
content = re.sub(r'V3\.\d+\.\d+ 稳定版 更新日志', r'V3.2.1 稳定版 更新日志', content, flags=re.IGNORECASE)
content = re.sub(r'下载最新 V3\.\d+\.\d+', r'下载最新 V3.2.1', content, flags=re.IGNORECASE)
content = re.sub(r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3\.\d+\.\d+"', r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3.2.1"', content, flags=re.IGNORECASE)

# Insert the new changelog about double playback fix 
new_changelog = """    <ul>
      <li><strong>完全切断双重播放架构 Bug</strong>: 修复了 DAW 加载 VST2WDM 插件并启用 IPC 时，底层物理扬声器依然会错误地同时并发播放双重音频的历史遗留问题。</li>
      <li><strong>安装包与编译架构增强</strong>: 清除了无用的 CMake 目录包裹层，直接产出纯净文件；解决并消灭了因 MSBuild 残留文件夹引发的嵌套崩溃问题。</li>
      <li><strong>卸载遗留项清除机制</strong>: 大幅优化 Inno Setup，解决安装器对安装环境缓存清理的弊病。修复并增强了针对错误代码 37 和 52 的提示弹窗流程。</li>
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
