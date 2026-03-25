import ftplib

try:
    print("Connecting to FTP...")
    ftp = ftplib.FTP('103.80.27.48')
    ftp.login('geek_asmrtop_cn', '5yifcKJkbKefm4JR')
    ftp.set_pasv(False)
    
    print("Downloading wdm2vst.html...")
    with open('D:/Autigravity/wdm2vst/wdm2vst_ftp.html', 'wb') as f:
        ftp.retrbinary('RETR wdm2vst.html', f.write)
        
    ftp.quit()
    print("Download complete.")
except Exception as e:
    print(f"Error: {e}")
