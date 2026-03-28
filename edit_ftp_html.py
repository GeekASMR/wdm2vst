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
content = re.sub(r'v[23]\.\d+\.\d+ · Public Edition', r'V3.2.2 · Public Edition', content, flags=re.IGNORECASE)
content = re.sub(r'WDM2VST_v[23]\.\d+\.\d+_Setup\.exe', r'WDM2VST_Public_v3.2.2_Setup.exe', content, flags=re.IGNORECASE)
content = re.sub(r'WDM2VST_Public_v[23]\.\d+\.\d+_Setup\.exe', r'WDM2VST_Public_v3.2.2_Setup.exe', content, flags=re.IGNORECASE)

# Update the changelog header and button text
# First replace all explicit 3.2.1 to 3.2.2
content = re.sub(r'V3\.2\.1 稳定版 更新日志', r'V3.2.2 稳定版 更新日志', content, flags=re.IGNORECASE)
content = re.sub(r'下载最新 V3\.2\.1', r'下载最新 V3.2.2', content, flags=re.IGNORECASE)
content = re.sub(r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3\.2\.1"', r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3.2.2"', content, flags=re.IGNORECASE)

# Then fix the collateral damage where historical block got renamed to 3.2.2 by targeting the specific historical span class and reverting it back to 3.2.1
content = re.sub(r'V3\.2\.2 稳定版 更新日志( <span style="font-size:12px;color:#777;font-weight:600;margin-left:8px">\(历史大版本\))', r'V3.2.1 稳定版 更新日志\1', content)
content = re.sub(r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3\.\d+\.\d+"', r'href="https://github.com/VirtualAudioRouter/WDM2VST/releases/tag/v3.2.2"', content, flags=re.IGNORECASE)

# Insert the new comprehensive changelog about double playback and tmp logic 
new_changelog = """    <ul>
      <li><strong>彻底解决超大音画延迟漏洞</strong>: 修复了长达2年的历史大坑！放宽了内部重采样时钟死锁倍率，从原先的底层限制最大差值补偿提高至 10 倍以上，彻底解决了从宿主（设置 44.1k）连接跨时钟音频流（如网易云等 48k）时，随着时间推移产生长达 3 秒音画不同步的崩溃体验，现在它能完全丝滑跨时钟！</li>
      <li><strong>自动毫秒级防溢出阻断</strong>: 如果发生了因系统卡顿导致的历史缓存音频堆积（超过 80 毫秒的误差累计），插件将立即以非破坏性强硬清空历史堆压并瞬间重接最新零延迟时间点，防止幽灵回声。</li>
      <li><strong>安装逻辑再次升维</strong>: 从 3.2.0 引用的无痕安装逻辑更加精编。</li>
    </ul>"""
content = re.sub(r'<ul>.*?<li>.*?</li>.*?</ul>', new_changelog, content, flags=re.IGNORECASE | re.DOTALL)

update_block = """    <div class="step"><div><h4>版本更新检测</h4><p>内置自动化版本侦测系统。当检测到新版本时，UI自动变形展现极客风格呼吸幻彩按钮，全自动重定向至最新发布页。</p></div></div>
    <div class="step"><div><h4>内核级遥测监控 (Telemetry)</h4><p>首创诊断式宿主音频配置收集，实时协助排查因未知原因引发的爆音问题。</p></div></div>
    <div class="step"><div><h4>⚠️安装异常修复手册</h4><p>若安装后驱动提示设备异常（代码37/52），请关闭 Windows 安全中心 -> 设备安全性中的【内存完整性】后重启即可。</p></div></div>"""

if "版本更新检测" not in content:
    content = content.replace('<div class="steps">', '<div class="steps">\n' + update_block)

inst_block = """      <div class="pcard">
        <h3>INST WDM2VST</h3><span class="tag">扩展 · 乐器版</span>
        <p>伪装成合成器（Synth/Instrument）的 WDM2VST。专为那些禁止在空白 MIDI/乐器 轨道上直接加载普通效果器的特殊宿主软件（DAW）设计，支持 MIDI 接收并无缝引流系统音频。</p>
        <span class="arrow">系统音 → DAW (乐器轨)</span>
      </div>
"""

inst_dtable = """  <!-- Detail: INST WDM2VST -->
  <div class="dtable">
    <h3><span style="color:#E91E63">🎹</span> INST WDM2VST —— 乐器轨专用捕获端</h3>
    <div class="desc">专为必须加载在虚拟乐器 / MIDI轨 (VSTi/Synth) 的特殊 DAW 设计</div>
    <table>
      <tr><th style="width:110px">属性</th><th>说明</th></tr>
      <tr><td>功能</td><td>伪装成 MIDI 合成器，允许你在没有任何物理音频输入的【空白乐器轨道】上强制加载本接收器，打破通道束缚。</td></tr>
      <tr><td>核心机制</td><td>底层引擎同普通核心版完全一致，具备极致平滑的毫秒级防溢出重采样时钟锁流控。</td></tr>
      <tr><td>适用场景</td><td>如 FL Studio / Ableton Live 等对效果器轨与乐器轨限制严格的宿主软件中强制捕获系统音频等。</td></tr>
    </table>
  </div>
"""

if "INST WDM2VST" not in content:
    content = content.replace('<h3>Audio Receive</h3>', inst_block + '      <div class="pcard">\n        <h3>Audio Receive</h3>')

if "乐器轨专用捕获端" not in content:
    content = content.replace('<!-- Detail: VST2WDM -->', inst_dtable + '  <!-- Detail: VST2WDM -->')


print("Uploading back to FTP...")
with open(local_file, 'w', encoding='utf-8') as f:
    f.write(content)

with open(local_file, 'rb') as f:
    ftp.storbinary(f'STOR {remote_file}', f)

ftp.quit()
print("Website updated and deployed on FTP.")
