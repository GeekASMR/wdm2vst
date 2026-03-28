param (
    [Parameter(Mandatory=$true)][string]$newVersion
)

$oldVersion = Get-Content ".\version.txt" | Out-String
$oldVersion = $oldVersion.Trim()

if (-not $oldVersion) {
    $oldVersion = "3.2.1"
}

Write-Host "Updating from $oldVersion to $newVersion..."

# 1. Update version.txt
Set-Content -Path ".\version.txt" -Value $newVersion
Write-Host "Updated version.txt"

# 2. Update CMakeLists.txt
$cmakePath = ".\ASMRTOP_Plugins\CMakeLists.txt"
$cmakeContent = Get-Content $cmakePath -Raw
$cmakeContent = $cmakeContent -replace 'VERSION "\d+\.\d+\.\d+"', "VERSION `"$newVersion`""
Set-Content -Path $cmakePath -Value $cmakeContent
Write-Host "Updated CMakeLists.txt"

# 3. Update Setup_PUBLIC.iss
$issPath = ".\ASMRTOP_Plugins\installer\Setup_PUBLIC.iss"
$issContent = Get-Content $issPath -Raw
$issContent = $issContent -replace '#define MyAppVersion "\d+\.\d+\.\d+"', "#define MyAppVersion `"$newVersion`""
Set-Content -Path $issPath -Value $issContent
Write-Host "Updated Setup_PUBLIC.iss"

# 4. Update build_both.ps1
$buildPs1 = ".\ASMRTOP_Plugins\build_both.ps1"
$buildContent = Get-Content $buildPs1 -Raw
$buildContent = $buildContent -replace 'updated to \d+\.\d+\.\d+', "updated to $newVersion"
Set-Content -Path $buildPs1 -Value $buildContent
Write-Host "Updated build_both.ps1"

# 5. Update edit_ftp_html.py
$pyPath = ".\edit_ftp_html.py"
$pyContent = Get-Content $pyPath -Raw
$pyContent = $pyContent -replace $oldVersion, $newVersion
Set-Content -Path $pyPath -Value $pyContent
Write-Host "Updated edit_ftp_html.py"

# 6. Copy release notes
$oldNotes = ".\v$oldVersion.txt"
$newNotes = ".\v$newVersion.txt"
if (Test-Path $oldNotes) {
    $noteContent = Get-Content $oldNotes -Raw
    $noteContent = $noteContent -replace $oldVersion, $newVersion
    Set-Content -Path $newNotes -Value $noteContent
    Write-Host "Created $newNotes based on $oldNotes"
}

Write-Host "Version successfully unified to $newVersion!"
