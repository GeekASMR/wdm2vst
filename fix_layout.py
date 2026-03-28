import re
import ftplib

with open('wdm2vst.html', 'r', encoding='utf-8') as f:
    text = f.read()

# Fix nested pcard
text = re.sub(r'<div class="pcard">\s*<div class="pcard">\s*<h3>INST WDM2VST</h3>', r'<div class="pcard">\n        <h3>INST WDM2VST</h3>', text)

# Professional changelog
professional_changelog = """    <ul>
      <li><strong>修复：跨时钟音频累积延迟问题</strong>：重构了重采样同步算法。当系统音频与宿主项目采样率不匹配时（例如 48kHz 转换至 44.1kHz），彻底解决了内部时钟偏差引发的延迟缓慢累积的问题，确保长期运行保持毫秒级音画同步。</li>
      <li><strong>修复：回声与防溢出控制</strong>：增加了毫秒级的缓冲区状态校验，系统严重拥堵导致音频堆积超过 80ms 时将自动重置并硬同步至最新时间点，防止卡顿后产生持续性回声。</li>
      <li><strong>新增：乐器轨专用接收端 (INST WDM2VST)</strong>：提供声明为 VSTi 虚拟乐器格式的特殊版本插件，满足 FL Studio、Ableton Live 等宿主的通道类型要求，允许用户在纯缓冲式的空白 MIDI 或乐器通道内强制捕获系统级音频音频。</li>
    </ul>"""
text = re.sub(r'<ul>.*?<li>.*?</li>.*?</ul>', professional_changelog, text, flags=re.IGNORECASE | re.DOTALL)

with open('wdm2vst.html', 'w', encoding='utf-8') as f:
    f.write(text)

print("Uploading clean fixed HTML...")
ftp = ftplib.FTP(timeout=15)
ftp.connect('103.80.27.48', 21)
ftp.login('geek_asmrtop_cn', '5yifcKJkbKefm4JR')
ftp.set_pasv(True)
with open('wdm2vst.html', 'rb') as f:
    ftp.storbinary('STOR wdm2vst.html', f)
ftp.quit()
print("Website layout and professional changelog fixed and uploaded.")
