import ftplib
ftp = ftplib.FTP(timeout=15)
ftp.connect('103.80.27.48', 21)
ftp.login('geek_asmrtop_cn', '5yifcKJkbKefm4JR')
ftp.set_pasv(True)
with open('wdm2vst.html', 'rb') as f:
    ftp.storbinary('STOR wdm2vst.html', f)
try:
    with open('WDM2VST_Debug_Guide.html', 'rb') as debug_f:
        ftp.storbinary('STOR wdm2vst_debug.html', debug_f)
except FileNotFoundError:
    print("Debug guide not found, skipping...")
ftp.quit()
print('FTP SUCCESS')
