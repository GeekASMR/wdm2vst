$pwd = "d:\Autigravity\wdm2vst\ASMRTOP_Plugins"
cd $pwd

$vst3Dir = "C:\Program Files\Common Files\VST3"

function BuildBrand($BrandStr, $BuildDir) {
    Write-Host "Building Plugins: $BrandStr"
    powershell.exe -ExecutionPolicy Bypass -File "..\ASMRTOP_Driver_V3\set_brand.ps1" $BrandStr
    
    cmake -B $BuildDir
    if ($LASTEXITCODE -ne 0) { Write-Error "CMake configure failed for $BrandStr"; exit 1 }
    
    # Pre-cleanup flattened files to avoid MSBuild error MSB3191
    Get-ChildItem -Path "$BuildDir\*" -Filter "*.vst3" -Recurse -File | Remove-Item -Force -ErrorAction SilentlyContinue

    cmake --build $BuildDir --config Release
    if ($LASTEXITCODE -ne 0) { Write-Error "CMake build failed for $BrandStr"; exit 1 }
    
    # Flatten the .vst3 bundles into purely single file .vst3 binaries per user request
    $vst3bundles = Get-ChildItem -Path "$BuildDir\*" -Filter "*.vst3" -Recurse -Directory
    foreach ($bundle in $vst3bundles) {
        $innerDll = Get-ChildItem -Path $bundle.FullName -Filter "*.vst3" -Recurse -File | Select-Object -First 1
        if ($innerDll) {
            $tempBundle = "$($bundle.FullName)_tmp"
            Rename-Item -Path $bundle.FullName -NewName "$($bundle.Name)_tmp" -Force
            # Now extract the dll from the renamed bundle
            $newInnerDll = Get-ChildItem -Path $tempBundle -Filter "*.vst3" -Recurse -File | Select-Object -First 1
            $destPath = "$($bundle.Parent.FullName)\$($bundle.Name)"
            Copy-Item -Path $newInnerDll.FullName -Destination $destPath -Force
            Remove-Item -Path $tempBundle -Recurse -Force
        }
    }

    # Now the artefacts are genuine single-file .vst3 files, we install them
    $innerVst3 = Get-ChildItem -Path "$BuildDir\*" -Filter "*.vst3" -Recurse -File

    # Determine manufacturer folder
    $manufFolder = if ($BrandStr -eq "public") { "VirtualAudioRouter" } else { "ASMRTOP" }
    $targetDir = "$vst3Dir\$manufFolder"
    if (!(Test-Path $targetDir)) { New-Item -ItemType Directory -Force -Path $targetDir | Out-Null }

    foreach ($v in $innerVst3) {
        Write-Host "Installing single-file $($v.Name) into $manufFolder..."
        Copy-Item -Path $v.FullName -Destination "$targetDir\$($v.Name)" -Force
    }
}

BuildBrand "public" "build_public"
BuildBrand "asmrtop" "build_asmrtop"

Write-Host "All plugin versions updated to 3.2.0 and installed successfully!"
