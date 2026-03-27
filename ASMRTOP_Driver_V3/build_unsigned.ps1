# -*- coding: utf-8 -*-
<#
.SYNOPSIS
    Build both branded and public drivers (unsigned) with inf2cat only.
#>
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

$ErrorActionPreference = "Stop"

$driverDir = "d:\Autigravity\wdm2vst\ASMRTOP_Driver_V3"
$pkg = "$driverDir\x64\Release\package"
$outDirName1 = [char]0x7B7E + [char]0x540D
$outDirName2 = [char]0x5171 + [char]0x5B58 + [char]0x7248 + [char]0x672A + [char]0x7B7E + [char]0x540D
$outDir = "D:\Autigravity\sgin\$outDirName1\$outDirName2"
$inf2cat = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe"

cd $driverDir

# Clean package dir
Remove-Item "$pkg\*.sys" -Force -ErrorAction SilentlyContinue
Remove-Item "$pkg\*.inf" -Force -ErrorAction SilentlyContinue
Remove-Item "$pkg\*.cat" -Force -ErrorAction SilentlyContinue

function BuildAndPackageUnsigned($BrandStr, $SysName, $InfName, $CatName) {
    Write-Host ""
    Write-Host "============================================" -ForegroundColor Magenta
    Write-Host "  Building: $BrandStr  ($SysName)" -ForegroundColor Magenta
    Write-Host "============================================" -ForegroundColor Magenta

    # 1. Switch brand (updates .inx with correct HW ID, service name, etc.)
    Write-Host "[1/5] Switching brand to $BrandStr..." -ForegroundColor Cyan
    powershell.exe -ExecutionPolicy Bypass -File .\set_brand.ps1 $BrandStr

    # 2. Patch .inx for sys/cat filenames
    Write-Host "[2/5] Patching .inx filenames..." -ForegroundColor Cyan
    Remove-Item "$pkg\VirtualAudioDriver.*" -Force -ErrorAction SilentlyContinue

    $inx = "Source\Main\VirtualAudioDriver.inx"
    $c = Get-Content $inx -Raw -Encoding UTF8

    $c = $c -replace '(?i)VirtualAudioRouter\.sys', $SysName
    $c = $c -replace '(?i)ASMRTOPVirtualAudio\.sys', $SysName
    $c = $c -replace '(?i)ASMRTOPAudioRouter\.sys', $SysName
    $c = $c -replace '(?i)VirtualAudioDriver\.sys', $SysName

    $c = $c -replace '(?i)CatalogFile\s*=\s*[A-Za-z0-9_]+\.cat', "CatalogFile = $CatName"

    Set-Content $inx $c -Encoding UTF8 -NoNewline

    # 3. Build
    Write-Host "[3/5] Building driver (MSBuild)..." -ForegroundColor Cyan
    cmd.exe /c "build.bat release x64 < NUL"

    if (!(Test-Path "$pkg\VirtualAudioDriver.sys")) {
        Write-Host "  ERROR: Build failed - VirtualAudioDriver.sys not found!" -ForegroundColor Red
        return
    }

    # 4. Rename outputs
    Write-Host "[4/5] Renaming outputs..." -ForegroundColor Cyan
    Rename-Item "$pkg\VirtualAudioDriver.sys" $SysName -Force
    Rename-Item "$pkg\VirtualAudioDriver.inf" $InfName -Force

    # Fix DriverVer and save as Unicode (INF standard)
    $infPath = "$pkg\$InfName"
    $infContent = Get-Content $infPath -Encoding UTF8
    $infContent = $infContent -replace '(?i)DriverVer\s*=.*', 'DriverVer = 03/27/2026, 3.2.0.0'
    $infContent | Set-Content $infPath -Encoding Unicode

    # 5. Run inf2cat (NO signing)
    Write-Host "[5/5] Running Inf2Cat..." -ForegroundColor Cyan
    Push-Location $pkg
    $others = Get-ChildItem "*.inf" | Where-Object { $_.Name -ne $InfName }
    foreach ($o in $others) { Rename-Item $o.Name "$($o.Name).hidden" }

    & $inf2cat /driver:"." /os:10_X64

    foreach ($o in $others) { Rename-Item "$($o.Name).hidden" $o.Name }
    Pop-Location

    # Copy to output
    Write-Host "Copying to output dir..." -ForegroundColor Cyan
    Copy-Item "$pkg\$SysName" "$outDir\$SysName" -Force
    Copy-Item "$pkg\$InfName" "$outDir\$InfName" -Force
    Copy-Item "$pkg\$CatName" "$outDir\$CatName" -Force

    Write-Host "  OK: $SysName, $InfName, $CatName -> $outDir" -ForegroundColor Green
}

# Build Public version first
BuildAndPackageUnsigned "public" "VirtualAudioRouter.sys" "VirtualAudioRouter.inf" "VirtualAudioRouter.cat"

# Build ASMRTOP branded version
BuildAndPackageUnsigned "asmrtop" "ASMRTOPAudioRouter.sys" "ASMRTOPAudioRouter.inf" "ASMRTOPAudioRouter.cat"

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host "  ALL DONE!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
Write-Host "Output directory: $outDir" -ForegroundColor Yellow
Write-Host ""
Get-ChildItem $outDir | Format-Table Name, Length -AutoSize
