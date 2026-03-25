import winreg

paths = [
    r"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render",
    r"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture"
]
for p in paths:
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, p) as h:
            for i in range(winreg.QueryInfoKey(h)[0]):
                sub = winreg.EnumKey(h, i)
                try:
                    with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, f"{p}\\{sub}\\Properties") as ph:
                        for j in range(winreg.QueryInfoKey(ph)[1]):
                            val = winreg.EnumValue(ph, j)
                            if "a45" in val[0] or "b3f" in val[0]:
                                vstr = str(val[1])
                                if "Top" in vstr or "ASMR" in vstr or "Speakers" in vstr or "Mix" in vstr:
                                    print(f"{val[0]} = {vstr.encode('utf-8', errors='replace').decode('utf-8')}")
                except Exception: pass
    except Exception: pass
