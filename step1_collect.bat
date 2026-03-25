@echo off
setlocal
set BD=d:\Autigravity\wdm2vst\installer_build
set PK=d:\Autigravity\wdm2vst\ASMRTOP_Driver_V3\x64\Release\package

mkdir "%BD%" 2>nul
copy /Y "%PK%\VirtualAudioDriver.sys" "%BD%\"
copy /Y "%PK%\VirtualAudioDriver.inf" "%BD%\"
copy /Y "%PK%\asmrtopvirtualaudio.cat" "%BD%\"
copy /Y "d:\Autigravity\wdm2vst\ASMRTOP_Install.bat" "%BD%\"

echo Done. Files in %BD%
pause
