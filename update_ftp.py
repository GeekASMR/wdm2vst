import ftplib
import os

FTP_HOST = '103.80.27.48'
FTP_USER = 'geek_asmrtop_cn'
FTP_PASS = '5yifcKJkbKefm4JR'

try:
    ftp = ftplib.FTP(FTP_HOST)
    ftp.login(FTP_USER, FTP_PASS)
    print("Logged in successfully.")
    
    # List files in current directory to find index.html or similar
    files = []
    ftp.dir(files.append)
    print("Files in root directory:")
    for f in files:
        print(f)
        
    # Attempt to download index.html if it exists
    # we first check if there is a 'wwwroot' or 'htdocs'
    # we will just list all dirs to see where the web root is
        
    ftp.quit()
    
except Exception as e:
    print(f"FTP Error: {e}")
