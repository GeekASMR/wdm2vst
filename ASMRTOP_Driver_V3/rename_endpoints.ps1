<#
.SYNOPSIS
    Rename audio endpoints for Virtual Audio Router / ASMRTOP driver.
    Must be run with Administrator privileges.
.DESCRIPTION
    Uses COM IMMDevice API to rename render and capture endpoints,
    equivalent to right-click rename in Sound control panel.
.PARAMETER Brand
    "public" or "asmrtop". Determines the endpoint names.
#>
param(
    [string]$Brand = "public",
    [string]$DeviceMatch = ""
)

# Define endpoint names per brand
$brands = @{
    public = @{
        Device = "Virtual Audio Router"
        Speaker = @("Virtual 1/2", "Virtual 3/4", "Virtual 5/6", "Virtual 7/8")
        Mic = @("Mic 1/2", "Mic 3/4", "Mic 5/6", "Mic 7/8")
    }
    asmrtop = @{
        Device = "ASMRTOP Audio Router"
        Speaker = @("ASMR Audio 1/2", "ASMR Audio 3/4", "ASMR Audio 5/6", "ASMR Audio 7/8")
        Mic = @("ASMR Mic 1/2", "ASMR Mic 3/4", "ASMR Mic 5/6", "ASMR Mic 7/8")
    }
}

$b = $brands[$Brand.ToLower()]
if (-not $b) { Write-Host "Unknown brand: $Brand" -ForegroundColor Red; exit 1 }
if ($DeviceMatch) { $b.Device = $DeviceMatch }

# Use the AudioDeviceCmdlets approach - direct registry modification via COM
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

[ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")]
class MMDeviceEnumeratorCls {}

[ComImport, Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
interface IMMDeviceEnumerator
{
    int EnumAudioEndpoints(int dataFlow, int dwStateMask, out IMMDeviceCollection ppDevices);
    int GetDefaultAudioEndpoint(int dataFlow, int role, out IMMDevice ppEndpoint);
    int GetDevice(string pwstrId, out IMMDevice ppDevice);
}

[ComImport, Guid("0BD7A1BE-7A1A-44DB-8397-CC5392387B5E"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
interface IMMDeviceCollection
{
    int GetCount(out uint pcDevices);
    int Item(uint nDevice, out IMMDevice ppDevice);
}

[ComImport, Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
interface IMMDevice
{
    int Activate(ref Guid iid, int dwClsCtx, IntPtr pActivationParams, [MarshalAs(UnmanagedType.IUnknown)] out object ppInterface);
    int OpenPropertyStore(int stgmAccess, out IPropertyStore ppProperties);
    int GetId([MarshalAs(UnmanagedType.LPWStr)] out string ppstrId);
    int GetState(out int pdwState);
}

[ComImport, Guid("886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
interface IPropertyStore
{
    int GetCount(out uint cProps);
    int GetAt(uint iProp, out PropertyKey pkey);
    int GetValue(ref PropertyKey key, out PropVariant pv);
    int SetValue(ref PropertyKey key, ref PropVariant propvar);
    int Commit();
}

[StructLayout(LayoutKind.Sequential)]
struct PropertyKey
{
    public Guid fmtid;
    public uint pid;
    public PropertyKey(Guid g, uint p) { fmtid = g; pid = p; }
}

[StructLayout(LayoutKind.Explicit)]
struct PropVariant
{
    [FieldOffset(0)] public ushort vt;
    [FieldOffset(8)] public IntPtr pwszVal;

    public static PropVariant FromString(string s)
    {
        var pv = new PropVariant();
        pv.vt = 31; // VT_LPWSTR
        pv.pwszVal = Marshal.StringToCoTaskMemUni(s);
        return pv;
    }
    public string GetString()
    {
        if (vt == 31 && pwszVal != IntPtr.Zero)
            return Marshal.PtrToStringUni(pwszVal);
        return "";
    }
}

public class AudioEndpointRenamer
{
    // PKEY_Device_FriendlyName = {a45c254e-df1c-4efd-8020-67d146a850e0},14
    static readonly PropertyKey PKEY_FriendlyName = new PropertyKey(
        new Guid(0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0), 14);
    // PKEY_DeviceInterface_FriendlyName = {026e516e-b814-414b-83cd-856d6fef4822},2
    static readonly PropertyKey PKEY_InterfaceFriendlyName = new PropertyKey(
        new Guid(0x026e516e, 0xb814, 0x414b, 0x83, 0xcd, 0x85, 0x6d, 0x6f, 0xef, 0x48, 0x22), 2);

    public static string[] RenameEndpoints(int dataFlow, string deviceMatch, string[] newNames)
    {
        var results = new System.Collections.Generic.List<string>();
        var enumerator = (IMMDeviceEnumerator)new MMDeviceEnumeratorCls();
        IMMDeviceCollection coll;
        enumerator.EnumAudioEndpoints(dataFlow, 0x0F, out coll); // all states
        uint count;
        coll.GetCount(out count);

        int nameIdx = 0;
        for (uint i = 0; i < count && nameIdx < newNames.Length; i++)
        {
            IMMDevice dev;
            coll.Item(i, out dev);
            IPropertyStore store;
            dev.OpenPropertyStore(0, out store); // STGM_READ
            
            PropVariant pvName;
            var key = PKEY_FriendlyName;
            store.GetValue(ref key, out pvName);
            string curName = pvName.GetString();

            if (curName.Contains(deviceMatch))
            {
                IPropertyStore storeW;
                dev.OpenPropertyStore(1, out storeW); // STGM_READWRITE
                
                string fullName = newNames[nameIdx] + " (" + deviceMatch + ")";
                var pvNew = PropVariant.FromString(fullName);
                storeW.SetValue(ref key, ref pvNew);

                var ifKey = PKEY_InterfaceFriendlyName;
                var pvShort = PropVariant.FromString(newNames[nameIdx]);
                storeW.SetValue(ref ifKey, ref pvShort);
                storeW.Commit();

                results.Add("OK: " + curName + " -> " + fullName);
                nameIdx++;
            }
        }
        return results.ToArray();
    }
}
"@ -ErrorAction Stop

Write-Host "Renaming endpoints for brand: $($Brand.ToUpper())" -ForegroundColor Yellow
Write-Host "Device: $($b.Device)" -ForegroundColor Cyan

Write-Host "`nRender (Speakers):" -ForegroundColor Green
$results = [AudioEndpointRenamer]::RenameEndpoints(0, $b.Device, $b.Speaker)
foreach ($r in $results) { Write-Host "  $r" }

Write-Host "`nCapture (Mics):" -ForegroundColor Green
$results = [AudioEndpointRenamer]::RenameEndpoints(1, $b.Device, $b.Mic)
foreach ($r in $results) { Write-Host "  $r" }

# Restart audio service to apply names
Write-Host "`nRestarting audio service..." -ForegroundColor Yellow
Restart-Service AudioSrv -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Write-Host "Done!" -ForegroundColor Green
