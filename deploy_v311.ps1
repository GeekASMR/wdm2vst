$ErrorActionPreference = "Stop"

cd D:\Autigravity\wdm2vst\ASMRTOP_Plugins

Write-Host "=== Cleaning Old Files Collisions ==="
Remove-Item '.\build_public\ASMRTOP_*_artefacts\Release\VST3\*.vst3' -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item '.\build_asmrtop\ASMRTOP_*_artefacts\Release\VST3\*.vst3' -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "`n=== Building VSTs ==="
& .\build_both.ps1
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "`n=== Building Setup_PUBLIC_v3.1.1 ==="
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" ".\installer\Setup_PUBLIC.iss"
if ($LASTEXITCODE -ne 0) { throw "ISCC failed" }

Write-Host "`n=== Copying Setup to Release folder ==="
Copy-Item ".\installer\WDM2VST_Public_v3.1.1_Setup.exe" -Destination "..\Release\WDM2VST_Public_v3.1.1_Setup.exe" -Force

cd D:\Autigravity\wdm2vst
Write-Host "`n=== Updating FTP HTML ==="
python edit_ftp_html.py

Write-Host "`n=== Git Commit ==="
git config --global user.email "geek_asmrtop@github.com"
git config --global user.name "Geek ASMR"
git add .
git commit -m "Release v3.1.1: Fix Duplicated Output from VST2WDM Auto-Connect"
git push origin HEAD

Write-Host "`n=== GH CLI Release ==="
gh release create v3.1.1 ".\Release\WDM2VST_Public_v3.1.1_Setup.exe" -F release_notes.md -t "WDM2VST V3.1.1 Production Release"

Write-Host "`n=== DONE ==="
