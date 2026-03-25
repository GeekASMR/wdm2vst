import winreg
import sys
import ctypes

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

def run():
    print("="*50)
    print("ASMRTOP 底层设备名称长度修复工具 (DirectSound 兼容版)")
    print("="*50)

    if not is_admin():
        print("[!] 权限不足：修改注册表声卡名称需要管理员权限。")
        print("[!] 请右键此文件，选择 '以管理员身份运行'。")
        input("按回车键退出...")
        return

    changed = 0
    paths = [
        r"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render",
        r"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture"
    ]
    
    # 我们采用一套极简纯原生的命名来彻底解决 DirectSound 长度溢出崩溃的问题
    for p in paths:
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, p, 0, winreg.KEY_READ) as h:
                n_sub = winreg.QueryInfoKey(h)[0]
                for i in range(n_sub):
                    sub = winreg.EnumKey(h, i)
                    try:
                        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, f"{p}\\{sub}\\Properties", 0, winreg.KEY_ALL_ACCESS) as ph:
                            n_val = winreg.QueryInfoKey(ph)[1]
                            for j in range(n_val):
                                val_name, val_data, val_type = winreg.EnumValue(ph, j)
                                if val_type == winreg.REG_SZ and isinstance(val_data, str):
                                    new_name = val_data
                                    
                                    # 针对太长的"专属虚拟引擎"以及"系统内放"缩短化处理
                                    if "ASMRTOP" in new_name or "专属虚拟引擎" in new_name:
                                        new_name = "ASMR Engine"
                                        
                                    if "Speakers 01" in new_name: new_name = "ASMR Sys 01"
                                    elif "Speakers 02" in new_name: new_name = "ASMR Player"
                                    elif "Speakers 03" in new_name: new_name = "ASMR Sys 03"
                                    elif "Speakers 04" in new_name: new_name = "ASMR Sys 04"
                                    elif "Mix 01" in new_name: new_name = "ASMR Cap 01"
                                    elif "Mix 02" in new_name: new_name = "ASMR Cap 02"
                                    elif "Mix 03" in new_name: new_name = "ASMR Cap 03"
                                    elif "Mix 04" in new_name: new_name = "ASMR Cap 04"
                                    
                                    if new_name != val_data:
                                        winreg.SetValueEx(ph, val_name, 0, val_type, new_name)
                                        changed += 1
                                        print(f"[MMDevices 修改] {val_data} -> {new_name}")
                    except Exception as e:
                        print(f"[-] 遍历出错 (端点): {e}")
        except Exception as e:
            print(f"[-] 遍历出错 (根): {e}")

    try:
        enum_path = r"SYSTEM\CurrentControlSet\Enum"
        def scan_enum_key(hkey, current_path):
            nonlocal changed
            try:
                num_subkeys = winreg.QueryInfoKey(hkey)[0]
                for i in range(num_subkeys):
                    sub_name = winreg.EnumKey(hkey, i)
                    sub_path = f"{current_path}\\{sub_name}"
                    try:
                        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, sub_path, 0, winreg.KEY_ALL_ACCESS) as sub_key:
                            num_values = winreg.QueryInfoKey(sub_key)[1]
                            for j in range(num_values):
                                val_name, val_data, val_type = winreg.EnumValue(sub_key, j)
                                if val_type == winreg.REG_SZ and isinstance(val_data, str):
                                    if val_name in ["FriendlyName", "DeviceDesc"]:
                                        new_name = val_data
                                        if "ASMRTOP" in new_name or "专属虚拟引擎" in new_name:
                                            new_name = "ASMR Engine"
                                            
                                        if "Speakers 01" in new_name: new_name = "ASMR Sys 01"
                                        elif "Speakers 02" in new_name: new_name = "ASMR Player"
                                        elif "Speakers 03" in new_name: new_name = "ASMR Sys 03"
                                        elif "Speakers 04" in new_name: new_name = "ASMR Sys 04"
                                        elif "Mix 01" in new_name: new_name = "ASMR Cap 01"
                                        elif "Mix 02" in new_name: new_name = "ASMR Cap 02"
                                        elif "Mix 03" in new_name: new_name = "ASMR Cap 03"
                                        elif "Mix 04" in new_name: new_name = "ASMR Cap 04"
                                        
                                        if new_name != val_data:
                                            winreg.SetValueEx(sub_key, val_name, 0, val_type, new_name)
                                            print(f"[ENUM 修改] {val_data} -> {new_name}")
                                            changed += 1
                            scan_enum_key(sub_key, sub_path)
                    except Exception:
                        pass
            except Exception:
                pass
                
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, enum_path, 0, winreg.KEY_READ) as eh:
            scan_enum_key(eh, enum_path)
    except Exception as e:
        print(f"[-] 遍历出错 (ENUM): {e}")

    print(f"\n[!] 修复完成，共计修正了 {changed} 处过长的注册表项。")
    print("[!] 请 重启电脑 或者 在设备管理器中右键禁用并重新启用“ASMRTOP”虚拟声卡，让修改彻底生效！")
    input("按回车键退出...")

if __name__ == "__main__":
    run()
