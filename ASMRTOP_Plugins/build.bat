@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: Detect brand from PluginBranding.h
:: ============================================================================
set BRAND=ASMRTOP
findstr /C:"//#define ASMRTOP_PLUGIN_BRAND" Source\PluginBranding.h >nul 2>&1
if %errorlevel% equ 0 (
    set BRAND=PUBLIC
)

if "%BRAND%"=="ASMRTOP" (
    set W2V_NAME=ASMRTOP WDM2VST
    set V2W_NAME=ASMRTOP VST2WDM
) else (
    set W2V_NAME=WDM2VST
    set V2W_NAME=VST2WDM
)

echo ========================================
echo  Building: %W2V_NAME% / %V2W_NAME%
echo  Brand:    %BRAND%
echo ========================================
echo.

echo Configuring JUCE Project...
cmake -B build
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Building Release version...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Installing VST3 plugins...

set VST3_DIR=C:\Program Files\Common Files\VST3
set W2V_SRC=D:\wdm2vst\ASMRTOP_Plugins\build\ASMRTOP_WDM2VST_artefacts\Release\VST3\%W2V_NAME%.vst3
set V2W_SRC=D:\wdm2vst\ASMRTOP_Plugins\build\ASMRTOP_VST2WDM_artefacts\Release\VST3\%V2W_NAME%.vst3

xcopy /E /I /Y "%W2V_SRC%" "%VST3_DIR%\%W2V_NAME%.vst3"
xcopy /E /I /Y "%V2W_SRC%" "%VST3_DIR%\%V2W_NAME%.vst3"

echo.
echo ========================================
echo  Done! Installed to:
echo    %VST3_DIR%\%W2V_NAME%.vst3
echo    %VST3_DIR%\%V2W_NAME%.vst3
echo ========================================
