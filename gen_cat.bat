@echo off
echo Generating CAT for ASMRTOP...
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\inf2cat.exe" /driver:D:\Autigravity\sgin\wdm2vst\asmrtop /os:10_x64
echo Result: %errorlevel%
echo.
echo Generating CAT for PUBLIC...
"C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\inf2cat.exe" /driver:D:\Autigravity\sgin\wdm2vst\gongkai /os:10_x64
echo Result: %errorlevel%
echo.
echo Files in asmrtop:
dir D:\Autigravity\sgin\wdm2vst\asmrtop
echo.
echo Files in gongkai:
dir D:\Autigravity\sgin\wdm2vst\gongkai
pause
