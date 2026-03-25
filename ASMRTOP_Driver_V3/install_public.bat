@echo off
echo ==========================================
echo  Install Public Version Driver
echo ==========================================
echo.

cd /d "D:\wdm2vst\ASMRTOP_Driver_V2\Source\Main\x64\Release"

echo Files:
dir /b *.sys *.inf *.cat
echo.

echo Installing driver...
"C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe" install VirtualAudioDriver.inf ROOT\ASMRTOPVirtualAudio

if %ERRORLEVEL% equ 0 (
    echo.
    echo SUCCESS! Driver installed.
) else (
    echo.
    echo FAILED with code %ERRORLEVEL%
    echo.
    echo If this fails, try:
    echo   1. Reboot (test signing needs reboot)
    echo   2. Run this script as Administrator
)

echo.
pause
