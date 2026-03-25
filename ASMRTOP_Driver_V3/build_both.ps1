$pkg = "d:\Autigravity\wdm2vst\ASMRTOP_Driver_V3\x64\Release\package"
Remove-Item "$pkg\*.sys" -Force -ErrorAction SilentlyContinue
Remove-Item "$pkg\*.inf" -Force -ErrorAction SilentlyContinue
Remove-Item "$pkg\*.cat" -Force -ErrorAction SilentlyContinue

$bin = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"
$inf2cat = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe"
$pwd = "d:\Autigravity\wdm2vst\ASMRTOP_Driver_V3"
cd $pwd

function BuildAndPackage($BrandStr, $SysName, $InfName, $CatName) {
    Write-Host "Building $BrandStr..."
    powershell.exe -ExecutionPolicy Bypass -File .\set_brand.ps1 $BrandStr
    
    # Clean output package inside before msbuild
    Remove-Item "$pkg\VirtualAudioDriver.*" -Force -ErrorAction SilentlyContinue
    
    $inx = "Source\Main\VirtualAudioDriver.inx"
    $c = Get-Content $inx -Raw -Encoding UTF8
    
    $c = $c -replace '(?i)VirtualAudioRouter\.sys', $SysName
    $c = $c -replace '(?i)ASMRTOPVirtualAudio\.sys', $SysName
    $c = $c -replace '(?i)ASMRTOPAudioRouter\.sys', $SysName
    $c = $c -replace '(?i)VirtualAudioDriver\.sys', $SysName
    
    # We unify ALL catalog names here with Regex
    $c = $c -replace '(?i)CatalogFile\s*=\s*[A-Za-z0-9_]+\.cat', "CatalogFile = $CatName"
    
    Set-Content $inx $c -Encoding UTF8 -NoNewline

    cmd.exe /c "build.bat release x64 < NUL"
    
    Rename-Item "$pkg\VirtualAudioDriver.sys" $SysName -Force
    Rename-Item "$pkg\VirtualAudioDriver.inf" $InfName -Force
    
    $infPath = "$pkg\$InfName"
    $infContent = Get-Content $infPath -Encoding UTF8
    $infContent = $infContent -replace '(?i)DriverVer\s*=.*', 'DriverVer = 03/24/2026, 3.1.0.0'
    $infContent | Set-Content $infPath -Encoding Unicode

    Push-Location $pkg
    $others = Get-ChildItem "*.inf" | Where-Object { $_.Name -ne $InfName }
    foreach ($o in $others) { Rename-Item $o.Name "$($o.Name).hidden" }
    
    & $inf2cat /driver:"." /os:10_X64
    
    foreach ($o in $others) { Rename-Item "$($o.Name).hidden" $o.Name }

    & $bin\signtool.exe sign /v /a /s PrivateCertStore /n WDM2VST_TestCert /fd sha256 /tr http://timestamp.digicert.com /td sha256 $SysName
    & $bin\signtool.exe sign /v /a /s PrivateCertStore /n WDM2VST_TestCert /fd sha256 /tr http://timestamp.digicert.com /td sha256 $CatName
    Pop-Location
}

BuildAndPackage "public" "VirtualAudioRouter.sys" "VirtualAudioRouter.inf" "VirtualAudioRouter.cat"
BuildAndPackage "asmrtop" "ASMRTOPAudioRouter.sys" "ASMRTOPAudioRouter.inf" "ASMRTOPAudioRouter.cat"
