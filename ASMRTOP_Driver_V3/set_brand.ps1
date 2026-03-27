<#
.SYNOPSIS
    Switch branding between ASMRTOP and Public versions.
.DESCRIPTION
    Updates all branding files across both the Driver and Plugin projects:
    - Driver:  branding.h, VirtualAudioDriver.inx
    - Plugins: PluginBranding.h (CMakeLists.txt reads it automatically)
.PARAMETER Brand
    "asmrtop" or "public"
.EXAMPLE
    .\set_brand.ps1 asmrtop
    .\set_brand.ps1 public
#>
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("asmrtop","public")]
    [string]$Brand
)

$scriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$brandingH  = Join-Path $scriptDir "Source\Inc\branding.h"
$inxFile    = Join-Path $scriptDir "Source\Main\VirtualAudioDriver.inx"
$pluginDir  = Join-Path (Split-Path -Parent $scriptDir) "ASMRTOP_Plugins"
$pluginBrandH = Join-Path $pluginDir "Source\PluginBranding.h"

# ============================================================================
# Brand definitions
# ============================================================================
$brands = @{
    asmrtop = @{
        Define        = '#define ASMRTOP_BRAND'
        PluginDefine  = '#define ASMRTOP_PLUGIN_BRAND'
        Provider      = 'ASMRTOP'
        DeviceDesc    = 'ASMRTOP Audio Router'
        SvcDesc       = 'ASMRTOP Audio Router'
        HardwareId    = 'ASMRTOPVirtualAudio'
        ServiceName   = 'ASMRTOPVirtualAudio'
        Speaker       = @('ASMR Audio 1/2','ASMR Audio 3/4','ASMR Audio 5/6','ASMR Audio 7/8')
        Mic           = @('ASMR Mic 1/2','ASMR Mic 3/4','ASMR Mic 5/6','ASMR Mic 7/8')
        IpcBaseName   = 'AsmrtopWDM'
        PluginW2VName = 'ASMRTOP WDM2VST'
        PluginV2WName = 'ASMRTOP VST2WDM'
    }
    public = @{
        Define        = '//#define ASMRTOP_BRAND'
        PluginDefine  = '//#define ASMRTOP_PLUGIN_BRAND'
        Provider      = 'VirtualAudioRouter'
        DeviceDesc    = 'Virtual Audio Router'
        SvcDesc       = 'Virtual Audio Router'
        HardwareId    = 'VirtualAudioRouter'
        ServiceName   = 'VirtualAudioRouter'
        Speaker       = @('Virtual 1/2','Virtual 3/4','Virtual 5/6','Virtual 7/8')
        Mic           = @('Mic 1/2','Mic 3/4','Mic 5/6','Mic 7/8')
        IpcBaseName   = 'VirtualAudioWDM'
        PluginW2VName = 'WDM2VST'
        PluginV2WName = 'VST2WDM'
    }
}

$b = $brands[$Brand]

Write-Host ""
Write-Host "================================================" -ForegroundColor Magenta
Write-Host "  Switching to: $($Brand.ToUpper()) brand" -ForegroundColor Magenta
Write-Host "================================================" -ForegroundColor Magenta
Write-Host ""

# ============================================================================
# 1. Update branding.h  (Driver)
# ============================================================================
Write-Host "[Driver] Updating branding.h ..." -ForegroundColor Cyan

$hContent = Get-Content $brandingH -Raw -Encoding UTF8
$hContent = $hContent -replace '(?m)^(//)?#define ASMRTOP_BRAND', $b.Define
Set-Content -Path $brandingH -Value $hContent -Encoding UTF8 -NoNewline
Write-Host "  OK" -ForegroundColor Green

# ============================================================================
# 2. Update VirtualAudioDriver.inx  (Driver)
# ============================================================================
Write-Host "[Driver] Updating VirtualAudioDriver.inx ..." -ForegroundColor Cyan

$inxContent = Get-Content $inxFile -Raw -Encoding UTF8

# Brand marker
$inxContent = $inxContent -replace '>>> BRAND:\w+ <<<', ">>> BRAND:$($Brand.ToUpper()) <<<"

# Provider / Mfg
$inxContent = $inxContent -replace '(?m)^ProviderName\s*=\s*"[^"]*"', "ProviderName = `"$($b.Provider)`""
$inxContent = $inxContent -replace '(?m)^MfgName\s*=\s*"[^"]*"', "MfgName      = `"$($b.Provider)`""
$inxContent = $inxContent -replace '(?m)^MsCopyRight\s*=\s*"[^"]*"', "MsCopyRight  = `"$($b.Provider)`""

# Device desc / Service desc
$inxContent = $inxContent -replace '(?m)^ASMRTOPVirtualAudio_SA\.DeviceDesc="[^"]*"', "ASMRTOPVirtualAudio_SA.DeviceDesc=`"$($b.DeviceDesc)`""
$inxContent = $inxContent -replace '(?m)^ASMRTOPVirtualAudio\.SvcDesc="[^"]*"', "ASMRTOPVirtualAudio.SvcDesc=`"$($b.SvcDesc)`""

# Hardware ID  (ROOT\xxx in [Manufacturer] section)
$inxContent = $inxContent -replace 'ROOT\\(ASMRTOPVirtualAudio|VirtualAudioRouter)', "ROOT\$($b.HardwareId)"

# Service name  (AddService= and KmdfService= lines)
$inxContent = $inxContent -replace 'AddService=(ASMRTOPVirtualAudio|VirtualAudioRouter)', "AddService=$($b.ServiceName)"
$inxContent = $inxContent -replace 'KmdfService\s*=\s*(ASMRTOPVirtualAudio|VirtualAudioRouter)', "KmdfService = $($b.ServiceName)"

# Speaker names (1-4)
$spkNames = @('WaveSpeaker','WaveSpeaker2','WaveSpeaker3','WaveSpeaker4')
$spkTopoNames = @('TopologySpeaker','TopologySpeaker2','TopologySpeaker3','TopologySpeaker4')
for ($i = 0; $i -lt 4; $i++) {
    $wn = $spkNames[$i]
    $tn = $spkTopoNames[$i]
    $name = $b.Speaker[$i]
    $inxContent = $inxContent -replace "(?m)^ASMRTOPVirtualAudio\.$wn\.szPname=`"[^`"]*`"", "ASMRTOPVirtualAudio.$wn.szPname=`"$name`""
    $inxContent = $inxContent -replace "(?m)^ASMRTOPVirtualAudio\.$tn\.szPname=`"[^`"]*`"", "ASMRTOPVirtualAudio.$tn.szPname=`"$name`""
}

# Mic names (1-4)
$micWaveNames = @('WaveMicArray1','WaveMicArray2','WaveMicArray3','WaveMicArray4')
$micTopoNames = @('TopologyMicArray1','TopologyMicArray2','TopologyMicArray3','TopologyMicArray4')
for ($i = 0; $i -lt 4; $i++) {
    $wn = $micWaveNames[$i]
    $tn = $micTopoNames[$i]
    $name = $b.Mic[$i]
    $inxContent = $inxContent -replace "(?m)^ASMRTOPVirtualAudio\.$wn\.szPname=`"[^`"]*`"", "ASMRTOPVirtualAudio.$wn.szPname=`"$name`""
    $inxContent = $inxContent -replace "(?m)^ASMRTOPVirtualAudio\.$tn\.szPname=`"[^`"]*`"", "ASMRTOPVirtualAudio.$tn.szPname=`"$name`""
}

# MicArray1CustomName
$inxContent = $inxContent -replace '(?m)^MicArray1CustomName=\s*"[^"]*"', "MicArray1CustomName= `"$($b.Mic[0])`""

# MediaCategories pin display names (for endpoint names in Sound panel)
for ($i = 0; $i -lt 4; $i++) {
    $num = $i + 1
    $inxContent = $inxContent -replace "(?m)^SpeakerPinName$num\s*=\s*`"[^`"]*`"", "SpeakerPinName$num = `"$($b.Speaker[$i])`""
    $inxContent = $inxContent -replace "(?m)^MicPinName$num\s*=\s*`"[^`"]*`"", "MicPinName$num = `"$($b.Mic[$i])`""
}

# Endpoint display names (PKEY_Device_DeviceDesc)
for ($i = 0; $i -lt 4; $i++) {
    $num = $i + 1
    $inxContent = $inxContent -replace "(?m)^SpeakerName$num=`"[^`"]*`"", "SpeakerName$num=`"$($b.Speaker[$i])`""
    $inxContent = $inxContent -replace "(?m)^MicName$num=`"[^`"]*`"", "MicName$num=`"$($b.Mic[$i])`""
}

Set-Content -Path $inxFile -Value $inxContent -Encoding UTF8 -NoNewline
Write-Host "  OK" -ForegroundColor Green

# ============================================================================
# 3. Update PluginBranding.h  (Plugins)
# ============================================================================
if (Test-Path $pluginBrandH) {
    Write-Host "[Plugin] Updating PluginBranding.h ..." -ForegroundColor Cyan

    $pContent = Get-Content $pluginBrandH -Raw -Encoding UTF8

    # Brand marker
    $pContent = $pContent -replace '>>> BRAND:\w+ <<<', ">>> BRAND:$($Brand.ToUpper()) <<<"

    # Toggle the #define
    $pContent = $pContent -replace '(?m)^(//)?#define ASMRTOP_PLUGIN_BRAND', $b.PluginDefine

    Set-Content -Path $pluginBrandH -Value $pContent -Encoding UTF8 -NoNewline
    Write-Host "  OK" -ForegroundColor Green
} else {
    Write-Host "[Plugin] PluginBranding.h not found at: $pluginBrandH" -ForegroundColor Yellow
    Write-Host "  Skipped." -ForegroundColor Yellow
}

# ============================================================================
# Summary
# ============================================================================
Write-Host ""
Write-Host "================================================" -ForegroundColor Yellow
Write-Host "  Brand: $($Brand.ToUpper())" -ForegroundColor Yellow
Write-Host "================================================" -ForegroundColor Yellow
Write-Host ""
Write-Host "  [Driver]"
Write-Host "    Device:     $($b.DeviceDesc)"
Write-Host "    HW ID:      ROOT\$($b.HardwareId)"
Write-Host "    Service:    $($b.ServiceName)"
Write-Host "    Speakers:   $($b.Speaker -join ', ')"
Write-Host "    Mics:       $($b.Mic -join ', ')"
Write-Host "    IPC Base:   $($b.IpcBaseName)"
Write-Host ""
Write-Host "  [Plugins]"
Write-Host "    WDM2VST:    $($b.PluginW2VName)"
Write-Host "    VST2WDM:    $($b.PluginV2WName)"
Write-Host "    IPC Base:   $($b.IpcBaseName)"
Write-Host ""
Write-Host "Run build.bat in each project to compile." -ForegroundColor Cyan
