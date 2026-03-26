@echo off
title WDM2VST Ultimate Cleanup Tool
echo ==============================================================
echo    WDM2VST / ASMRTOP AUDIO ROUTER - ULTIMATE CLEANUP SCRIPT
echo ==============================================================
echo.
echo WARNING: This script will forcibly remove:
echo     1. Virtual Audio Kernel Devices (devcon)
echo     2. PNP Driver Packages (pnputil)
echo     3. Injected signature certificates (time.reg)
echo     4. VST3 plugin files and folders
echo     5. Registry uninstaller records
echo.
echo PLEASE ENSURE YOU RUN THIS AS ADMINISTRATOR!
pause

echo.
echo [1/5] Removing Virtual Audio Kernel Devices...
"%~dp0devcon.exe" remove ROOT\ASMRTOPVirtualAudio >nul 2>&1
"%~dp0devcon.exe" remove ROOT\VirtualAudioRouter >nul 2>&1
"%~dp0devcon.exe" remove ROOT\VirtualAudioWDM >nul 2>&1

echo.
echo [2/5] Removing PNP Driver Packages from System Store...
powershell -ExecutionPolicy Bypass -Command "$drivers = pnputil /enum-drivers | Out-String; $blocks = $drivers -split '(?=Published Name:|发布名称:)'; foreach ($block in $blocks) { if ($block -match 'VirtualAudioRouter.inf' -or $block -match 'VirtualAudioDriver.inf') { if ($block -match '(?:Published Name:|发布名称:)\s+(oem\d+\.inf)') { $oemName = $matches[1]; Write-Host 'Purging '$oemName'...'; pnputil /delete-driver $oemName /uninstall /force | Out-Null } } }"

echo.
echo [3/5] Cleaning up injected certificates...
powershell -ExecutionPolicy Bypass -Command "Write-Host 'Scanning Cert stores...'; $paths = @('Cert:\LocalMachine\TrustedPublisher', 'Cert:\CurrentUser\TrustedPublisher', 'Cert:\LocalMachine\Root', 'Cert:\CurrentUser\Root'); foreach ($p in $paths) { if (Test-Path $p) { Get-ChildItem -Path $p -ErrorAction SilentlyContinue | Where-Object { $_.Subject -match 'VirtualAudio' -or $_.Subject -match 'ASMRTOP' } | ForEach-Object { Write-Host 'Removing Cert: ' $_.Subject; Remove-Item $_.PSPath -Force } } }"
reg delete "HKLM\SOFTWARE\Microsoft\SystemCertificates\ROOT\Certificates\E403A1DFC8F377E0F4AA43A83EE9EA079A1F55F2" /f >nul 2>&1
reg delete "HKLM\SOFTWARE\Microsoft\SystemCertificates\TrustedPublisher\Certificates\E403A1DFC8F377E0F4AA43A83EE9EA079A1F55F2" /f >nul 2>&1
reg delete "HKCU\SOFTWARE\Microsoft\SystemCertificates\TrustedPublisher\Certificates\E403A1DFC8F377E0F4AA43A83EE9EA079A1F55F2" /f >nul 2>&1

echo.
echo [4/5] Deleting VST3 Plugin Directories and legacy files...
rmdir /s /q "C:\Program Files\Common Files\VST3\VirtualAudioRouter" >nul 2>&1
rmdir /s /q "C:\Program Files\Common Files\VST3\ASMRTOP" >nul 2>&1
del /f /q "C:\Program Files\Common Files\VST3\WDM2VST.vst3" >nul 2>&1
del /f /q "C:\Program Files\Common Files\VST3\VST2WDM.vst3" >nul 2>&1
del /f /q "C:\Program Files\Common Files\VST3\Audio Send.vst3" >nul 2>&1
del /f /q "C:\Program Files\Common Files\VST3\Audio Receive.vst3" >nul 2>&1
del /f /q "C:\Program Files\Common Files\VST3\INST WDM2VST.vst3" >nul 2>&1
del /f /q "C:\Program Files\Common Files\VST3\ASMRTOP*.*" >nul 2>&1

echo.
echo [5/5] Deleting Uninstaller Registry Keys...
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{B4A7F2E1-3C5D-4E6F-8A9B-1C2D3E4F5A6B}_is1" /f >nul 2>&1
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{A5B8C9D0-E1F2-4A3B-8C5D-6E7F8091A2B3}_is1" /f >nul 2>&1
reg delete "HKLM\Software\VirtualAudioRouter" /f >nul 2>&1

echo.
echo ==============================================================
echo  CLEANUP COMPLETE!
echo  Your system is now ready for a fresh WDM2VST test installation!
echo ==============================================================
pause
