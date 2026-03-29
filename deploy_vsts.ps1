$targetDir = "C:\Program Files\Common Files\VST3\VirtualAudioRouter"
if (!(Test-Path $targetDir)) { New-Item -ItemType Directory -Force -Path $targetDir | Out-Null }
$innerVst3 = Get-ChildItem -Path "D:\Autigravity\wdm2vst\ASMRTOP_Plugins\build_public\*_artefacts\Release\VST3" -Filter "*.vst3" -Recurse -File
foreach ($v in $innerVst3) {
    Write-Host "Deploying $($v.Name) to system VST3 folder..."
    Copy-Item -Path $v.FullName -Destination "$targetDir\$($v.Name)" -Force
}
Write-Host "System VSTs Updated."
