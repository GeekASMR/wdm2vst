import ftplib
import re

print("Connecting to FTP...")
ftp = ftplib.FTP(timeout=15)
ftp.connect('103.80.27.48', 21)
ftp.login('geek_asmrtop_cn', '5yifcKJkbKefm4JR')
ftp.set_pasv(False)

remote_file = 'wdm2vst.html'
local_file = 'wdm2vst.html'

print("Downloading from FTP...")
with open(local_file, 'wb') as f:
    ftp.retrbinary(f'RETR {remote_file}', f.write)

print("Applying modifications...")
with open(local_file, 'r', encoding='utf-8', errors='ignore') as f:
    content = f.read()

# Replace version string 
content = re.sub(r'v[23]\.1\.[01] · Public Edition', r'V3.1.1 · Public Edition', content, flags=re.IGNORECASE)
content = re.sub(r'WDM2VST_v[23]\.1\.[01]_Setup\.exe', r'WDM2VST_Public_v3.1.1_Setup.exe', content, flags=re.IGNORECASE)
content = re.sub(r'WDM2VST_Public_v3\.1\.0_Setup\.exe', r'WDM2VST_Public_v3.1.1_Setup.exe', content, flags=re.IGNORECASE)

update_block = """    <div class="step"><div><h4>版本更新检测</h4><p>内置自动化版本侦测系统。当检测到新版本时，UI自动变形展现极客风格呼吸幻彩按钮，全自动重定向至最新发布页。</p></div></div>
    <div class="step"><div><h4>内核级遥测监控 (Telemetry)</h4><p>首创诊断式宿主音频配置收集，实时协助排查因未知原因引发的爆音问题。</p></div></div>"""

if "版本更新检测" not in content:
    content = content.replace('<div class="steps">', '<div class="steps">\n' + update_block)

print("Uploading back to FTP...")
with open(local_file, 'w', encoding='utf-8') as f:
    f.write(content)

with open(local_file, 'rb') as f:
    ftp.storbinary(f'STOR {remote_file}', f)

ftp.quit()
print("Website updated and deployed on FTP.")
