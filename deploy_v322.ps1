$ErrorActionPreference = "Stop"

Set-Location D:\Autigravity\wdm2vst\ASMRTOP_Plugins

Write-Host "`n=== Building Setup_PUBLIC_v3.2.2 ==="
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" ".\installer\Setup_PUBLIC.iss"
if ($LASTEXITCODE -ne 0) { throw "ISCC failed" }

Write-Host "`n=== Copying Setup to Release folder ==="
Copy-Item ".\installer\WDM2VST_Public_v3.2.2_Setup.exe" -Destination "..\Release\WDM2VST_Public_v3.2.2_Setup.exe" -Force

Set-Location D:\Autigravity\wdm2vst

Write-Host "`n=== Git Commit ==="
git config --global user.email "geek_asmrtop@github.com"
git config --global user.name "Geek ASMR"
git add .
git commit -m "Release v3.2.2: Fix massive latency leak and desync on varying clocks"
git push origin HEAD

Write-Host "`n=== GH CLI Release ==="
gh release create v3.2.2 ".\Release\WDM2VST_Public_v3.2.2_Setup.exe" -F v3.2.2.txt -t "WDM2VST V3.2.2 Production Release"

Write-Host "`n=== DONE ==="
