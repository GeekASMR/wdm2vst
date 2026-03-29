$ErrorActionPreference = "Stop"

Set-Location D:\Autigravity\wdm2vst\ASMRTOP_Plugins

Write-Host "`n=== Building Setup_PUBLIC_v3.2.4 ==="
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" ".\installer\Setup_PUBLIC.iss"
if ($LASTEXITCODE -ne 0) { throw "ISCC failed" }

Write-Host "`n=== Copying Setup to Release folder ==="
Copy-Item ".\installer\WDM2VST_Public_v3.2.4_Setup.exe" -Destination "..\Release\WDM2VST_Public_v3.2.4_Setup.exe" -Force

Write-Host "`n=== Signing Setup ==="
$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe"
& $signtool sign /v /s PrivateCertStore /sha1 9B9991FE551A176B5C17865FFF7B9E773D02E2C8 /fd sha256 /tr http://timestamp.digicert.com /td sha256 "..\Release\WDM2VST_Public_v3.2.4_Setup.exe"
if ($LASTEXITCODE -ne 0) { throw "SignTool failed" }

Set-Location D:\Autigravity\wdm2vst

Write-Host "`n=== Git Commit ==="
git config --global user.email "geek_asmrtop@github.com"
git config --global user.name "Geek ASMR"
git add .
git commit -m "Release v3.2.4: Sync fixes disabled, stable sync logic"
git push origin master

Write-Host "`n=== GH CLI Release ==="
gh release create v3.2.4 ".\Release\WDM2VST_Public_v3.2.4_Setup.exe" -F v3.2.4.txt -t "WDM2VST V3.2.4 Production Release"

Write-Host "`n=== DONE ==="
