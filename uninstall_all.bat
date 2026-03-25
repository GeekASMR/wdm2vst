@echo off
set DEVCON="C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe"

echo =======================================
echo   Complete Driver Cleanup Tool
echo =======================================
echo.
echo *** WARNING ***
echo Please close ALL DAWs, Studio One, OBS, and ASIO Link panels NOW.
echo If any program is using the sound cards, the uninstallation WILL hang!
echo.
pause

echo.
echo [1] Force closing ASIO Link Tool...
taskkill /T /F /IM asiolinktool.exe >nul 2>&1
taskkill /T /F /IM asiolinktool32.exe >nul 2>&1
taskkill /T /F /IM asiolinktool64.exe >nul 2>&1

echo.
echo [2] Sending PnP remove commands...
%DEVCON% remove "*ASIOVADPRO"
%DEVCON% remove "*WDM2VST"

echo.
echo [3] Stopping and deleting services...
sc stop asiovadpro >nul 2>&1
sc delete asiovadpro >nul 2>&1
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\asiovadpro" /f >nul 2>&1

sc stop wdm2vst >nul 2>&1
sc delete wdm2vst >nul 2>&1
reg delete "HKLM\SYSTEM\CurrentControlSet\Services\wdm2vst" /f >nul 2>&1

echo.
echo [4] Deleting SYS binaries...
del /f /q "C:\Windows\System32\drivers\asiovadpro.sys" >nul 2>&1
rem NOT deleting wdm2vst.sys because we need it for our 4-channel installation!

echo.
echo [5] Purging OEM INF packages from DriverStore (Please wait)...
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; $out = pnputil /enum-drivers; $currentOem = ''; foreach($line in $out) { if($line -match 'oem\d+\.inf') { $currentOem = ($line -split ':')[1].Trim() }; if($line -match 'asiovadpro|wdm2vst|John Shield|ODeus') { Write-Host 'Purging' $currentOem; pnputil /delete-driver $currentOem /uninstall /force } }"

echo.
echo =======================================
echo Cleanup Complete!
echo.
echo IMPORTANT: If the scripts couldn't fully delete the packages because
echo they were "in use", you MUST REBOOT your computer now.
echo.
echo After rebooting, double-click 'install_4ch.bat' to install our 4-channel driver!
echo =======================================
pause
