@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
title ASMRTOP V3 Driver - Auto Install

:: ============================================================
::  Self-elevate to admin
:: ============================================================
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [*] Requesting admin...
    powershell -Command "Start-Process cmd -ArgumentList '/c \"%~f0\"' -Verb RunAs"
    exit /b
)

cls
echo ============================================================
echo   ASMRTOP Virtual Audio Driver V3 - Auto Installer
echo ============================================================
echo.

:: ============================================================
:: Step 1: Check files
:: ============================================================
echo [Step 1/5] Checking driver files...
if not exist "%~dp0VirtualAudioDriver.sys" (
    echo   ERROR: VirtualAudioDriver.sys missing!
    goto fail
)
if not exist "%~dp0VirtualAudioDriver.inf" (
    echo   ERROR: VirtualAudioDriver.inf missing!
    goto fail
)
echo   OK - All files present.
echo.

:: ============================================================
:: Step 2: Enable test signing
:: ============================================================
echo [Step 2/5] Enabling test signing mode...
bcdedit /set testsigning on >nul 2>&1
if %errorlevel% equ 0 (
    echo   OK - Test signing enabled.
) else (
    echo   WARNING: Could not enable test signing.
    echo   If Secure Boot is ON, you must disable it in VM settings first.
    echo   VM Settings: Firmware ^> Disable Secure Boot
)
echo.

:: ============================================================
:: Step 3: Install test certificate
:: ============================================================
echo [Step 3/5] Installing driver certificate...
if exist "%~dp0asmrtopvirtualaudio.cat" (
    :: Extract the signer cert and add to trusted store
    powershell -Command "$sig = Get-AuthenticodeSignature '%~dp0VirtualAudioDriver.sys'; if($sig.SignerCertificate){ $cert = $sig.SignerCertificate; $store = New-Object System.Security.Cryptography.X509Certificates.X509Store('TrustedPublisher','LocalMachine'); $store.Open('ReadWrite'); $store.Add($cert); $store.Close(); $store2 = New-Object System.Security.Cryptography.X509Certificates.X509Store('Root','LocalMachine'); $store2.Open('ReadWrite'); $store2.Add($cert); $store2.Close(); Write-Host '  OK - Certificate installed to trusted store.' } else { Write-Host '  SKIP - No certificate found (unsigned driver).' }"
) else (
    echo   SKIP - No catalog file found.
)
echo.

:: ============================================================
:: Step 4: Remove old driver (if exists)
:: ============================================================
echo [Step 4/5] Removing old driver (if any)...

:: Try devcon
set DEVCON=
for %%p in (
    "C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe"
    "C:\Program Files (x86)\Windows Kits\10\Tools\10.0.22621.0\x64\devcon.exe"
    "C:\Program Files (x86)\Windows Kits\10\Tools\10.0.22000.0\x64\devcon.exe"
    "C:\Program Files (x86)\Windows Kits\10\Tools\10.0.19041.0\x64\devcon.exe"
    "%~dp0devcon.exe"
) do (
    if exist %%p (
        set "DEVCON=%%~p"
        goto devcon_ok
    )
)

:: No devcon - try to use pnputil as fallback
echo   devcon.exe not found. Trying pnputil...
echo.
echo   ===========================================================
echo   WARNING: This driver needs devcon.exe to create the device.
echo   ===========================================================
echo.
echo   Downloading devcon is not possible automatically.
echo   Please do ONE of the following:
echo.
echo   Option A: Copy devcon.exe to this folder
echo     From WDK: C:\Program Files (x86)\Windows Kits\10\Tools\...\x64\devcon.exe
echo.
echo   Option B: Use pnputil (may need manual device creation)
echo.
set /p usepnp="  Try pnputil anyway? [Y/N]: "
if /i not "%usepnp%"=="Y" goto fail

echo   Adding driver via pnputil...
pnputil /delete-driver oem*.inf /uninstall /force >nul 2>&1
pnputil /add-driver "%~dp0VirtualAudioDriver.inf" /install
if %errorlevel% equ 0 (
    echo.
    echo   SUCCESS via pnputil!
    echo   But the device node may not be created automatically.
    echo   You may need to add the device manually in Device Manager:
    echo     Action ^> Add legacy hardware ^> MEDIA ^> ASMRTOP Audio Router
    goto done
) else (
    echo   FAILED. Error code: %errorlevel%
    goto fail
)

:devcon_ok
echo   Using: %DEVCON%
"%DEVCON%" remove ROOT\ASMRTOPVirtualAudio >nul 2>&1
echo   OK - Old driver cleaned.
echo.

:: ============================================================
:: Step 5: Install new driver
:: ============================================================
echo [Step 5/5] Installing driver...
"%DEVCON%" install "%~dp0VirtualAudioDriver.inf" ROOT\ASMRTOPVirtualAudio

if %errorlevel% equ 0 (
    goto done
) else (
    echo.
    echo   Install returned error %errorlevel%.
    echo.
    echo   Trying alternative method (update)...
    "%DEVCON%" update "%~dp0VirtualAudioDriver.inf" ROOT\ASMRTOPVirtualAudio
    if !errorlevel! equ 0 (
        goto done
    )
    goto fail
)

:: ============================================================
:done
:: ============================================================
echo.
echo ============================================================
echo   [OK] Installation Complete!
echo ============================================================
echo.
echo   Audio Endpoints Created:
echo     Play: ASMR Audio 1/2, 3/4, 5/6, 7/8
echo     Rec:  ASMR Mic  1/2, 3/4, 5/6, 7/8
echo.
echo   If this is the first time enabling test signing,
echo   please REBOOT for the driver to load.
echo.
echo ============================================================
echo.
set /p reboot="  Reboot now? [Y/N]: "
if /i "%reboot%"=="Y" shutdown /r /t 3 /c "Reboot for ASMRTOP driver"
goto end

:fail
echo.
echo ============================================================
echo   [FAIL] Installation failed.
echo ============================================================
echo.
echo   Troubleshooting:
echo.
echo   1. SECURE BOOT: Must be OFF in VM settings
echo      - Hyper-V: Settings ^> Security ^> Uncheck Secure Boot
echo      - VMware:  VM Settings ^> Options ^> Advanced ^> Firmware: BIOS
echo      - VirtualBox: Settings ^> System ^> Uncheck Enable EFI
echo.
echo   2. TEST SIGNING: Run in admin CMD:
echo      bcdedit /set testsigning on
echo      Then reboot.
echo.
echo   3. DEVCON.EXE: Required for root-enumerated driver
echo      Install Windows Driver Kit (WDK) or copy devcon.exe here.
echo.
echo   4. REBOOT: If you just enabled test signing, reboot first!
echo.
echo ============================================================

:end
echo.
pause
