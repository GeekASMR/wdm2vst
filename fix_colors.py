import re
import ftplib

with open('wdm2vst.html', 'r', encoding='utf-8') as f:
    text = f.read()

# Replace border color for 4th and add 5th
text = re.sub(r'\.pcard:nth-child\(4\)::before\{background:#4CAF50\}',
              '.pcard:nth-child(4)::before{background:#E91E63}\n  .pcard:nth-child(5)::before{background:#4CAF50}', text)

# Replace tag background for 4th and add 5th
if ".pcard:nth-child(5) .tag" not in text:
    text = re.sub(r'\.pcard:nth-child\(4\)\s*\.tag\{background:rgba\(76,175,80,\.15\);color:#4CAF50\}',
                  '.pcard:nth-child(4) .tag{background:rgba(233,30,99,.15);color:#E91E63}\n  .pcard:nth-child(5) .tag{background:rgba(76,175,80,.15);color:#4CAF50}', text)

with open('wdm2vst.html', 'w', encoding='utf-8') as f:
    f.write(text)

ftp = ftplib.FTP(timeout=15)
ftp.connect('103.80.27.48', 21)
ftp.login('geek_asmrtop_cn', '5yifcKJkbKefm4JR')
ftp.set_pasv(True)
with open('wdm2vst.html', 'rb') as f:
    ftp.storbinary('STOR wdm2vst.html', f)
ftp.quit()

print("Color tag fixed and uploaded.")
