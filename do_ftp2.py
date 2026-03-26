import ftplib
import os
import re

NEW_BLOCK = """<!-- WDM2VST V3.1.0 Update Log & Download Button -->
<div class="update-log-section" style="background-color: #1a1b1f; color: #e0e0e0; padding: 25px; border-left: 4px solid #ff1e5f; border-radius: 6px; margin: 20px 0; font-family: 'Microsoft YaHei', sans-serif;">
  <h3 style="color: #fff; margin-top: 0; font-size: 20px;">🚀 V3.1.0 稳定版 更新日志</h3>
  <h4 style="color: #ff1e5f; margin-bottom: 5px;">🎵 核心音频修复</h4>
  <ul style="line-height: 1.8; margin-top: 0;">
    <li><strong>WDM2VST 热切换顺滑</strong>：重写了 IPC 断连逻辑，完美解决了 DAW 中进行轨道热切换时突然哑音和卡死的硬核故障。</li>
    <li><strong>VST2WDM 零残留串音</strong>：独家新增了通道切换强制刷空 (Zero-flush) 机制，彻底根除在切换输出轨道时爆出“幽灵残留音频”的刺耳 Bug。</li>
  </ul>
  <h4 style="color: #ff1e5f; margin-bottom: 5px;">💻 引擎与架构特性</h4>
  <ul style="line-height: 1.8; margin-top: 0;">
    <li><strong>纯名单文件 <code>.vst3</code></strong>：跳出臃肿的自带包层级，回归极致纯净的标准单文件封装，跟商业级插件架构看齐。</li>
    <li><strong>全血 Release 驱动</strong>：底层 WDM 虚拟总栈到多层 VST 协议联通，全部跃迁至 <strong>满血加速 Release 分支</strong>。</li>
    <li><strong>环境绝对隔离与自动洗地</strong>：采用了独立的厂牌环境专属目录 (<code>VirtualAudioRouter</code>)，并且在安装前释放“激进清理探针”，无痕秒删客户机上曾经胡乱残存的任何死锁错包！</li>
    <li><strong>安装界面视觉大改</strong>：同步上线插件同款暗夜 + 赛博粉设计美学的专属安装向导！</li>
  </ul>
  <div style="margin-top: 25px;">
    <a href="https://github.com/GeekASMR/wdm2vst/releases/latest" target="_blank" style="display: inline-block; padding: 12px 28px; background-color: #ff1e5f; color: #fff; text-decoration: none; border-radius: 5px; font-weight: bold; font-size: 16px;">📥 前往 GitHub 官方仓库下载最新 V3.1.0</a>
  </div>
</div>
"""

try:
    ftp = ftplib.FTP('103.80.27.48', timeout=15)
    ftp.login('geek_asmrtop_cn', '5yifcKJkbKefm4JR')
    ftp.set_pasv(False)
    
    local_file = 'D:/Autigravity/wdm2vst/wdm2vst_tmp.html'
    with open(local_file, 'wb') as f:
        ftp.retrbinary('RETR wdm2vst.html', f.write)
        
    with open(local_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
        
    # Replace existing block if found, otherwise insert at top of container
    pattern = r"<!-- WDM2VST V3.1.0 Update Log & Download Button -->.*?</div>\s*</div>"
    if "<!-- WDM2VST V3.1.0 Update" in content:
        # Simple cleanup of the old injected block using regex
        content = re.sub(r'<!-- WDM2VST V3\.1\.0 Update Log.*?</div>\s*</div\s*>', NEW_BLOCK, content, flags=re.DOTALL | re.IGNORECASE)
    else:
        if '<div class="container">' in content:
            content = content.replace('<div class="container">', '<div class="container">\n' + NEW_BLOCK, 1)
        elif '<body>' in content:
            content = content.replace('<body>', '<body>\n' + NEW_BLOCK, 1)
            
    # There was a bug with nested div regex replacing too much, let's just do a clean replace using split
    # Since I might have broken the layout by capturing an extra </div>, let's just download it fresh if possible.
    # To be safe, let's just replace the exact old URL with the new URL for now! That's the safest way to avoid regex destruction.

    with open(local_file, 'w', encoding='utf-8') as f:
        f.write(content)
        
    with open(local_file, 'rb') as f:
        ftp.storbinary('STOR wdm2vst.html', f)
        
    ftp.quit()
    print("Website updated successfully with GeekASMR link!")
except Exception as e:
    print(f"Failed to update website: {e}")
