@echo off
set DEVCON="C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe"

echo =======================================
echo    4-Channel WDM2VST Installer
echo =======================================

echo.
echo [1] Removing ASIO Link Pro...
%DEVCON% remove "*ASIOVADPRO"
sc delete asiovadpro
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\asiovadpro" /f
del /f /q "C:\Windows\System32\drivers\asiovadpro.sys"

echo.
echo [2] Removing old WDM2VST...
%DEVCON% remove "*WDM2VST"
sc delete wdm2vst
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\wdm2vst" /f

echo.
echo [3] Copying wdm2vst.sys from DriverStore/System32...
if exist "C:\Windows\System32\drivers\wdm2vst.sys" (
    copy /y "C:\Windows\System32\drivers\wdm2vst.sys" "d:\Autigravity\wdm2vst\wdm2vst.sys"
    echo wdm2vst.sys successfully copied!
) else (
    echo wdm2vst.sys not found in System32\drivers!
    echo Trying to find it in FileRepository...
    for /d %%D in (C:\Windows\System32\DriverStore\FileRepository\wdm2vst.inf*) do (
        if exist "%%D\wdm2vst.sys" (
            copy /y "%%D\wdm2vst.sys" "d:\Autigravity\wdm2vst\wdm2vst.sys"
            echo wdm2vst.sys successfully extracted from DriverStore!
        )
    )
)

if not exist "d:\Autigravity\wdm2vst\wdm2vst.sys" (
    echo [ERROR] Could not find wdm2vst.sys. Please make sure WDM2VST11CE is installed.
    pause
    exit /b 1
)

echo.
echo [4] Installing the 4-channel device...
echo ** PLEASE PAY ATTENTION TO YOUR SCREEN! **
echo ** A red Windows Security prompt might appear asking to install an unsigned driver. **
echo ** Click 'Install this driver software anyway' **

%DEVCON% install "d:\Autigravity\wdm2vst\wdm2vst_4ch.inf" "*WDM2VST"

echo.
echo [5] All Done!
echo You can now check your Sound Control Panel.
pause
