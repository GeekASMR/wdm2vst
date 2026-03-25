$dir = "E:\Personal\Desktop\qudong"
Write-Host "=== Checking driver signatures in $dir ===" -ForegroundColor Cyan
Write-Host ""

$files = @("VirtualAudioDriver.sys", "asmrtopvirtualaudio.cat", "VirtualAudioDriver.inf")

foreach ($f in $files) {
    $path = Join-Path $dir $f
    if (Test-Path $path) {
        Write-Host "--- $f ---" -ForegroundColor Yellow
        $sig = Get-AuthenticodeSignature $path
        Write-Host "  Status:      $($sig.Status)"
        Write-Host "  StatusMsg:   $($sig.StatusMessage)"
        if ($sig.SignerCertificate) {
            Write-Host "  Subject:     $($sig.SignerCertificate.Subject)"
            Write-Host "  Issuer:      $($sig.SignerCertificate.Issuer)"
            Write-Host "  NotBefore:   $($sig.SignerCertificate.NotBefore)"
            Write-Host "  NotAfter:    $($sig.SignerCertificate.NotAfter)"
            Write-Host "  Thumbprint:  $($sig.SignerCertificate.Thumbprint)"
        } else {
            Write-Host "  Certificate: NONE (unsigned)" -ForegroundColor Red
        }
        Write-Host ""
    } else {
        Write-Host "--- $f --- NOT FOUND" -ForegroundColor Red
        Write-Host ""
    }
}

# Also try signtool if available
$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe"
if (Test-Path $signtool) {
    Write-Host "=== SignTool Verify ===" -ForegroundColor Cyan
    foreach ($f in @("VirtualAudioDriver.sys", "asmrtopvirtualaudio.cat")) {
        $path = Join-Path $dir $f
        if (Test-Path $path) {
            Write-Host "--- $f ---" -ForegroundColor Yellow
            & $signtool verify /pa /v $path 2>&1 | Write-Host
            Write-Host ""
        }
    }
}
