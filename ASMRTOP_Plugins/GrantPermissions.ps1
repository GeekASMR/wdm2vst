$ErrorActionPreference = "SilentlyContinue"
$paths = @("HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render", "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture")
foreach ($p in $paths) {
    $subkeys = Get-ChildItem -Path $p
    foreach ($sub in $subkeys) {
        $propPath = $sub.PSPath + "\Properties"
        
        # Take ownership and set full control
        $Rule = New-Object System.Security.AccessControl.RegistryAccessRule(
            "Administrators",
            "FullControl",
            "ContainerInherit,ObjectInherit",
            "None",
            "Allow"
        )
        
        $acl = Get-Acl $propPath
        if ($acl -ne $null) {
            $acl.SetAccessRule($Rule)
            Set-Acl -Path $propPath -AclObject $acl
        }
    }
}
Write-Host "Permissions updated"
