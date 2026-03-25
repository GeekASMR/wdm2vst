@echo off
chcp 65001 >nul 2>&1
echo ========================================
echo  Clean all WDM2VST / ASMRTOP drivers
echo ========================================
echo.

echo [1/5] Removing device nodes...
devcon.exe remove "ROOT\VirtualAudioRouter" >nul 2>&1
devcon.exe remove "ROOT\ASMRTOPAudioRouter" >nul 2>&1
devcon.exe remove "ROOT\ASMRTOPVirtualAudio" >nul 2>&1
devcon.exe remove "ROOT\wdm2vst" >nul 2>&1
echo   Done

echo [2/5] Deleting driver services...
sc.exe stop VirtualAudioRouter >nul 2>&1
sc.exe delete VirtualAudioRouter >nul 2>&1
sc.exe stop ASMRTOPAudioRouter >nul 2>&1
sc.exe delete ASMRTOPAudioRouter >nul 2>&1
sc.exe stop ASMRTOPVirtualAudio >nul 2>&1
sc.exe delete ASMRTOPVirtualAudio >nul 2>&1
sc.exe stop wdm2vst >nul 2>&1
sc.exe delete wdm2vst >nul 2>&1
sc.exe stop VirtualAudioDriver >nul 2>&1
sc.exe delete VirtualAudioDriver >nul 2>&1
echo   Done

echo [3/5] Cleaning registry...
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\VirtualAudioRouter" /f >nul 2>&1
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\ASMRTOPAudioRouter" /f >nul 2>&1
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\ASMRTOPVirtualAudio" /f >nul 2>&1
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\wdm2vst" /f >nul 2>&1
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\VirtualAudioDriver" /f >nul 2>&1
echo   Done

echo [4/5] Removing DriverStore packages...
setlocal enabledelayedexpansion
set "FOUND=0"
for /f "tokens=*" %%L in ('pnputil /enum-drivers 2^>nul') do (
    echo %%L | findstr /i /c:"oem" >nul 2>&1
    if not errorlevel 1 (
        for /f "tokens=2 delims=:" %%F in ("%%L") do (
            set "OEM=%%F"
            set "OEM=!OEM: =!"
        )
    )
    echo %%L | findstr /i "virtualaudiorouter virtualaudiodriver asmrtopaudiorouter wdm2vst" >nul 2>&1
    if not errorlevel 1 (
        echo   Deleting !OEM!...
        pnputil /delete-driver !OEM! /force >nul 2>&1
        set "FOUND=1"
    )
)
if "!FOUND!"=="0" echo   No packages found
endlocal
echo   Done

echo [5/5] Verifying...
echo.
pnputil /enum-devices /class "MEDIA" /problem 2>nul
echo.
echo ========================================
echo  Cleanup complete! Please REBOOT now.
echo ========================================
pause
