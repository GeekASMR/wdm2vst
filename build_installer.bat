@echo off
:: Build ASMRTOP Installer EXE
:: This script packages the driver files into a self-extracting EXE

setlocal
set BUILD_DIR=d:\Autigravity\wdm2vst\installer_build
set PKG_DIR=d:\Autigravity\wdm2vst\ASMRTOP_Driver_V3\x64\Release\package
set OUTPUT=d:\Autigravity\wdm2vst\ASMRTOP_4Channel_AutoInstall_V3_New.exe

echo [1/3] Preparing build directory...
mkdir "%BUILD_DIR%" 2>nul
copy /Y "%PKG_DIR%\VirtualAudioDriver.sys" "%BUILD_DIR%\"
copy /Y "%PKG_DIR%\VirtualAudioDriver.inf" "%BUILD_DIR%\"
copy /Y "%PKG_DIR%\asmrtopvirtualaudio.cat" "%BUILD_DIR%\"
copy /Y "d:\Autigravity\wdm2vst\ASMRTOP_Install.bat" "%BUILD_DIR%\"

:: Also try to copy devcon.exe if available
if exist "C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe" (
    copy /Y "C:\Program Files (x86)\Windows Kits\10\Tools\10.0.26100.0\x64\devcon.exe" "%BUILD_DIR%\"
    echo   + devcon.exe bundled
)

echo [2/3] Creating SED file...
(
echo [Version]
echo Class=IEXPRESS
echo SEDVersion=3
echo [Options]
echo PackagePurpose=InstallApp
echo ShowInstallProgramWindow=0
echo HideExtractAnimation=0
echo UseLongFileName=1
echo InsideCompressed=0
echo CAB_FixedSize=0
echo CAB_ResvCodeSigning=0
echo RebootMode=N
echo InstallPrompt=Install ASMRTOP Virtual Audio Driver V3?
echo DisplayLicense=
echo FinishMessage=
echo TargetName=%OUTPUT%
echo FriendlyName=ASMRTOP Audio Router V3
echo AppLaunched=ASMRTOP_Install.bat
echo PostInstallCmd=^<None^>
echo AdminQuietInstCmd=
echo UserQuietInstCmd=
echo SourceFiles=SourceFiles
echo [Strings]
echo FILE0="VirtualAudioDriver.sys"
echo FILE1="VirtualAudioDriver.inf"
echo FILE2="asmrtopvirtualaudio.cat"
echo FILE3="ASMRTOP_Install.bat"
) > "%BUILD_DIR%\package.SED"

:: Check if devcon was copied
if exist "%BUILD_DIR%\devcon.exe" (
    echo FILE4="devcon.exe" >> "%BUILD_DIR%\package.SED"
)

(
echo [SourceFiles]
echo SourceFiles0=%BUILD_DIR%\
echo [SourceFiles0]
echo %%FILE0%%=
echo %%FILE1%%=
echo %%FILE2%%=
echo %%FILE3%%=
) >> "%BUILD_DIR%\package.SED"

if exist "%BUILD_DIR%\devcon.exe" (
    echo %%FILE4%%=>> "%BUILD_DIR%\package.SED"
)

echo [3/3] Building EXE with IExpress...
iexpress /N /Q "%BUILD_DIR%\package.SED"

if exist "%OUTPUT%" (
    echo.
    echo ============================================================
    echo   SUCCESS! Installer created:
    echo   %OUTPUT%
    echo ============================================================
    echo.
    echo   Copy this file to your VM and double-click to install!
    
    :: Also copy to Desktop
    copy /Y "%OUTPUT%" "E:\Personal\Desktop\qudong\" 2>nul
    if %errorlevel% equ 0 (
        echo   Also copied to: E:\Personal\Desktop\qudong\
    )
) else (
    echo   ERROR: Failed to create EXE.
)

echo.
pause
