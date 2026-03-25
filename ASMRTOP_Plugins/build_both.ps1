$pwd = "d:\Autigravity\wdm2vst\ASMRTOP_Plugins"
cd $pwd

$vst3Dir = "C:\Program Files\Common Files\VST3"

function BuildBrand($BrandStr, $BuildDir) {
    Write-Host "Building Plugins: $BrandStr"
    powershell.exe -ExecutionPolicy Bypass -File "..\ASMRTOP_Driver_V3\set_brand.ps1" $BrandStr
    
    cmake -B $BuildDir
    if ($LASTEXITCODE -ne 0) { Write-Error "CMake configure failed for $BrandStr"; exit 1 }
    
    cmake --build $BuildDir --config Release
    if ($LASTEXITCODE -ne 0) { Write-Error "CMake build failed for $BrandStr"; exit 1 }
    
    # We use Get-ChildItem to dynamically find the compiled .vst3 folders!
    $vst3Out = Get-ChildItem -Path "$BuildDir\*" -Filter "*.vst3" -Recurse -Directory
    foreach ($v in $vst3Out) {
        Write-Host "Installing $($v.Name)..."
        Copy-Item -Path $v.FullName -Destination "$vst3Dir\$($v.Name)" -Recurse -Force
    }
}

BuildBrand "public" "build_public"
BuildBrand "asmrtop" "build_asmrtop"

Write-Host "All plugin versions updated to 3.0.1 and installed successfully!"
