$vstDir = "C:\Program Files\Common Files\VST3"
$publicDir = Join-Path $vstDir "VirtualAudioRouter"
$asmrtopDir = Join-Path $vstDir "ASMRTOP"

Remove-Item "$vstDir\*.vst3" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "$vstDir\VirtualAudioRouter" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "$vstDir\ASMRTOP" -Recurse -Force -ErrorAction SilentlyContinue

New-Item -ItemType Directory -Path $publicDir -Force -ErrorAction SilentlyContinue | Out-Null
New-Item -ItemType Directory -Path $asmrtopDir -Force -ErrorAction SilentlyContinue | Out-Null

$basePath = "d:\Autigravity\wdm2vst\ASMRTOP_Plugins"

$publicVst3s = Get-ChildItem -Path "$basePath\build_public\*_artefacts\Release\VST3\*.vst3" -Directory
foreach ($v in $publicVst3s) {
    if ($v.Name -match "\.vst3$") {
        $innerFile = Join-Path $v.FullName "Contents\x86_64-win\$($v.Name)"
        if (Test-Path $innerFile) {
            Write-Host "Deploying (Public) single-file $($v.Name)..."
            Copy-Item -Path $innerFile -Destination "$publicDir\$($v.Name)" -Force
        }
    }
}

$asmrtopVst3s = Get-ChildItem -Path "$basePath\build_asmrtop\*_artefacts\Release\VST3\*.vst3" -Directory
foreach ($v in $asmrtopVst3s) {
    if ($v.Name -match "\.vst3$") {
        $innerFile = Join-Path $v.FullName "Contents\x86_64-win\$($v.Name)"
        if (Test-Path $innerFile) {
            Write-Host "Deploying (ASMRTOP) single-file $($v.Name)..."
            Copy-Item -Path $innerFile -Destination "$asmrtopDir\$($v.Name)" -Force
        }
    }
}

Write-Host "DONE deploying single files!"
