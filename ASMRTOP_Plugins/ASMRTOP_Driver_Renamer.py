import winreg
import sys
import ctypes

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

def rename_audio_devices(target_word, replacement_word):
    print(f"[*] 开始全面扫描系统音频设备，寻找 '{target_word}' 并替换为 '{replacement_word}'...")
    
    # 我们需要在系统注册表中扫描渲染（Render - 扬声器）和捕获（Capture - 麦克风）设备
    # 还需要扫描更底层的枚举设备名 (Enum)
    base_paths = [
        r"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render",
        r"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture"
    ]
    enum_path = r"SYSTEM\CurrentControlSet\Enum"
    
    changed_count = 0

    for base_path in base_paths:
        try:
            # 打开音频设备根目录
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, base_path, 0, winreg.KEY_READ) as hkey:
                num_subkeys = winreg.QueryInfoKey(hkey)[0]
                
                # 遍历所有音频端点 (Endpoint GUIDs)
                for i in range(num_subkeys):
                    guid_key_name = winreg.EnumKey(hkey, i)
                    properties_path = f"{base_path}\\{guid_key_name}\\Properties"
                    
                    try:
                        # 尝试打开设备的 Properties 属性列表 (需要以读写/最高权限打开)
                        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, properties_path, 0, winreg.KEY_ALL_ACCESS) as prop_key:
                            num_values = winreg.QueryInfoKey(prop_key)[1]
                            
                            # 遍历设备的所有属性 (寻找包含了名字的那一行)
                            for j in range(num_values):
                                val_name, val_data, val_type = winreg.EnumValue(prop_key, j)
                                
                                # 音频设备 FriendlyName 通常是以字符串(REG_SZ)存储
                                if val_type == winreg.REG_SZ and isinstance(val_data, str):
                                    if target_word in val_data:
                                        new_name = val_data.replace(target_word, replacement_word)
                                        # 核心替换代码：写入新的名字
                                        winreg.SetValueEx(prop_key, val_name, 0, val_type, new_name)
                                        print(f"[+] 成功重命名设备项: '{val_data}' -> '{new_name}'")
                                        changed_count += 1
                                        
                    except OSError:
                        pass
        except OSError as e:
            # print(f"[-] 无法访问路径 {base_path}: {e}")
            pass
            
    # 特别扫描 Enum 设备管理器名称
    try:
        def scan_enum_key(hkey, current_path):
            nonlocal changed_count
            try:
                num_subkeys = winreg.QueryInfoKey(hkey)[0]
                for i in range(num_subkeys):
                    sub_name = winreg.EnumKey(hkey, i)
                    sub_path = f"{current_path}\\{sub_name}"
                    try:
                        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, sub_path, 0, winreg.KEY_ALL_ACCESS) as sub_key:
                            # 检查当前键的值
                            num_values = winreg.QueryInfoKey(sub_key)[1]
                            for j in range(num_values):
                                val_name, val_data, val_type = winreg.EnumValue(sub_key, j)
                                if val_type == winreg.REG_SZ and isinstance(val_data, str):
                                    if target_word in val_data and val_name in ["FriendlyName", "DeviceDesc"]:
                                        new_name = val_data.replace(target_word, replacement_word)
                                        winreg.SetValueEx(sub_key, val_name, 0, val_type, new_name)
                                        print(f"[+] 成功重命名底层枚举设备: '{val_data}' -> '{new_name}'")
                                        changed_count += 1
                            # 递归扫描子键
                            scan_enum_key(sub_key, sub_path)
                    except OSError:
                        pass
            except OSError:
                pass
                
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, enum_path, 0, winreg.KEY_READ) as enum_hkey:
            scan_enum_key(enum_hkey, enum_path)
    except OSError as e:
        # print(f"[-] 无法访问 Enum 路径: {e}")
        pass
            
    if changed_count > 0:
        print(f"\n[!] 恭喜，已成功修改 {changed_count} 处底层驱动名称！")
        print("[!] 请重启电脑，或在设备管理器中禁用再启用声卡，以使全新的系统驱动名称生效！")
    else:
        print(f"\n[-] 未在系统中寻找到包含 '{target_word}' 的底层驱动。可能是目标声卡尚未安装。")

if __name__ == "__main__":
    print("="*50)
    print("ASMRTOP 底层虚拟声卡驱动伪装器 - 极暗系统直发版")
    print("="*50)
    
    if not is_admin():
        print("[!] 权限不足：修改底层声卡显示名称必须使用管理员权限！")
        print("[!] 请右键选择 '以管理员身份运行'。")
        input("按回车键退出...")
        sys.exit(1)
        
    print("[*] 正在接管 Windows MMDevices 音频子系统...\n")
    
    # 在这里指定您的旧名字（您截图中显示的底层驱动名）和新名字
    # 您可以根据需要修改这里的内容
    rename_audio_devices("ASIOVADPRO Driver", "ASMRTOP")
    rename_audio_devices("Mix 01 (ASMRTOP)", "ASMRTOP - 纯净捕获通道 01")
    rename_audio_devices("Mix 02 (ASMRTOP)", "ASMRTOP - 纯净捕获通道 02")
    rename_audio_devices("Mix 03 (ASMRTOP)", "ASMRTOP - 纯净捕获通道 03")
    rename_audio_devices("Mix 04 (ASMRTOP)", "ASMRTOP - 纯净捕获通道 04")
    
    rename_audio_devices("Speakers 01 (ASMRTOP)", "ASMRTOP - 无损系统出口 01")
    rename_audio_devices("Speakers 01 系统内放 (ASMRTOP)", "ASMRTOP - 无损系统内放 01")
    rename_audio_devices("Speakers 02 系统内放 (ASMRTOP)", "ASMRTOP - 无损系统内放 02")
    rename_audio_devices("Speakers 02 播放器 (ASMRTOP)", "ASMRTOP - 无损播放通道 02")
    rename_audio_devices("Speakers 03 (ASMRTOP)", "ASMRTOP - 无损系统出口 03")
    rename_audio_devices("Speakers 04 (ASMRTOP)", "ASMRTOP - 无损系统出口 04")
    
    # Clean up redundant namings in case the user clicked multiple times
    rename_audio_devices("ASMRTOP 系统内放 系统内放 (ASMRTOP 专属虚拟引擎)", "ASMRTOP - 无损系统内放 01")
    rename_audio_devices("ASMRTOP - 纯净捕获通道 (ASMRTOP 专属虚拟引擎)", "ASMRTOP - 纯净捕获通道 01")
    
    print("\n" + "="*50)
    input("按回车键退出安装程序...")
