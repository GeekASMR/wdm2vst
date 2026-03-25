import winreg

def run():
    changed = 0
    paths = [
        r"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render",
        r"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture"
    ]
    for p in paths:
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, p, 0, winreg.KEY_READ) as h:
                for i in range(winreg.QueryInfoKey(h)[0]):
                    sub = winreg.EnumKey(h, i)
                    try:
                        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, f"{p}\\{sub}\\Properties", 0, winreg.KEY_ALL_ACCESS) as ph:
                            for j in range(winreg.QueryInfoKey(ph)[1]):
                                val_name, val_data, val_type = winreg.EnumValue(ph, j)
                                if val_type == winreg.REG_SZ and isinstance(val_data, str):
                                    new_name = val_data
                                    
                                    # Safe substring matching without caring about exact unicode suffixes
                                    if "Speakers 01" in new_name: new_name = "A-Top Sys 1"
                                    elif "Speakers 02" in new_name: new_name = "A-Top Player 2"
                                    elif "Speakers 03" in new_name: new_name = "A-Top Sys 3"
                                    elif "Speakers 04" in new_name: new_name = "A-Top Sys 4"
                                    elif "Mix 01" in new_name: new_name = "A-Top Cap 1"
                                    elif "Mix 02" in new_name: new_name = "A-Top Cap 2"
                                    elif "Mix 03" in new_name: new_name = "A-Top Cap 3"
                                    elif "Mix 04" in new_name: new_name = "A-Top Cap 4"
                                    elif "ASMRTOP" in new_name: new_name = "A-Top Engine"
                                    
                                    if new_name != val_data:
                                        winreg.SetValueEx(ph, val_name, 0, val_type, new_name)
                                        print(f"[{sub}] Fixed property")
                                        changed += 1
                    except Exception: pass
        except Exception: pass

    enum_path = r"SYSTEM\CurrentControlSet\Enum"
    def scan_enum_key(hkey, current_path):
        nonlocal changed
        try:
            for i in range(winreg.QueryInfoKey(hkey)[0]):
                sub_name = winreg.EnumKey(hkey, i)
                sub_path = f"{current_path}\\{sub_name}"
                try:
                    with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, sub_path, 0, winreg.KEY_ALL_ACCESS) as sub_key:
                        for j in range(winreg.QueryInfoKey(sub_key)[1]):
                            val_name, val_data, val_type = winreg.EnumValue(sub_key, j)
                            if val_type == winreg.REG_SZ and isinstance(val_data, str):
                                if val_name in ["FriendlyName", "DeviceDesc"]:
                                    new_name = val_data
                                    if "Speakers 01" in new_name: new_name = "A-Top Sys 1"
                                    elif "Speakers 02" in new_name: new_name = "A-Top Player 2"
                                    elif "Speakers 03" in new_name: new_name = "A-Top Sys 3"
                                    elif "Speakers 04" in new_name: new_name = "A-Top Sys 4"
                                    elif "Mix 01" in new_name: new_name = "A-Top Cap 1"
                                    elif "Mix 02" in new_name: new_name = "A-Top Cap 2"
                                    elif "Mix 03" in new_name: new_name = "A-Top Cap 3"
                                    elif "Mix 04" in new_name: new_name = "A-Top Cap 4"
                                    elif "ASMRTOP" in new_name: new_name = "A-Top Engine"
                                    if new_name != val_data:
                                        winreg.SetValueEx(sub_key, val_name, 0, val_type, new_name)
                                        print(f"ENUM Fixed property")
                                        changed += 1
                        scan_enum_key(sub_key, sub_path)
                except Exception: pass
        except Exception: pass
        
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, enum_path, 0, winreg.KEY_READ) as eh:
            scan_enum_key(eh, enum_path)
    except Exception: pass
    
    print(f"Total changed: {changed}")

if __name__ == "__main__":
    run()
