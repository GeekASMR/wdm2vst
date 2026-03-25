"""
Deep disassembly of wdm2vst.dll (x64) - focus on driver connection protocol.
"""
import pefile
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

DLL_PATH = r"C:\Program Files\VSTPlugins\Tools\wdm2vst.dll"

def get_text_section(pe):
    for sec in pe.sections:
        if sec.Characteristics & 0x20000000:  # executable
            return sec
    return None

def rva_to_fileoff(pe, rva):
    for sec in pe.sections:
        if sec.VirtualAddress <= rva < sec.VirtualAddress + sec.SizeOfRawData:
            return sec.PointerToRawData + (rva - sec.VirtualAddress)
    return None

def fileoff_to_rva(pe, foff):
    for sec in pe.sections:
        if sec.PointerToRawData <= foff < sec.PointerToRawData + sec.SizeOfRawData:
            return sec.VirtualAddress + (foff - sec.PointerToRawData)
    return None

def disasm_function(pe, md, rva, name, max_insns=300):
    """Disassemble a function from RVA."""
    foff = rva_to_fileoff(pe, rva)
    if foff is None:
        return []
    data = pe.__data__
    va = pe.OPTIONAL_HEADER.ImageBase + rva
    chunk = data[foff:foff + 4096]
    
    insns = []
    for insn in md.disasm(chunk, va):
        insns.append(insn)
        if len(insns) >= max_insns:
            break
        # Stop at ret (but allow multiple returns in function)
        if insn.mnemonic == 'ret' and len(insns) > 10:
            # Check if there's more code after (could be another basic block)
            pass  # Keep going to capture full function
    return insns


def find_ioctl_functions(pe, md):
    """Find and disassemble functions containing IOCTL codes."""
    data = pe.__data__
    base = pe.OPTIONAL_HEADER.ImageBase
    
    ioctls = {
        0x9C402410: "ATTACH",
        0x9C402414: "DETACH", 
        0x9C402420: "START",
        0x9C402424: "STOP",
        0x9C402428: "TRANSFER",
    }
    
    text_sec = get_text_section(pe)
    if not text_sec:
        print("No text section!")
        return
    
    code_start = text_sec.PointerToRawData
    code_end = code_start + text_sec.SizeOfRawData
    code_va = base + text_sec.VirtualAddress
    
    code_data = data[code_start:code_end]
    
    # Disassemble entire text section
    print("Disassembling entire .text section in x64 mode...")
    all_insns = list(md.disasm(code_data, code_va))
    print(f"Total instructions: {len(all_insns)}")
    
    # Find all CALL instructions that reference IAT entries  
    # In x64, calls to imported functions are: CALL [rip+disp32] or CALL rax etc.
    # DeviceIoControl: IAT at RVA 0x2F328 -> VA 0x18002F328
    
    # First find all calls to DeviceIoControl
    dio_iat_va = base + 0x2F328  # from previous analysis
    cfa_iat_va = base + 0x2F338  # CreateFileA
    cfw_iat_va = base + 0x2F320  # CreateFileW
    
    print(f"\nDeviceIoControl IAT VA: 0x{dio_iat_va:X}")
    print(f"CreateFileA IAT VA: 0x{cfa_iat_va:X}")
    print(f"CreateFileW IAT VA: 0x{cfw_iat_va:X}")
    
    # In x64, CALL [rip+disp] = FF 15 xx xx xx xx
    # The effective address = rip_after_insn + disp32
    
    dio_calls = []
    cfa_calls = []
    cfw_calls = []
    
    for insn in all_insns:
        if insn.mnemonic == 'call' and 'rip' in insn.op_str:
            # Compute the effective address
            # insn bytes: FF 15 <disp32>
            if len(insn.bytes) >= 6 and insn.bytes[0] == 0xFF and insn.bytes[1] == 0x15:
                disp = struct.unpack_from('<i', bytes(insn.bytes), 2)[0]
                target = insn.address + insn.size + disp
                
                if target == dio_iat_va:
                    dio_calls.append(insn)
                elif target == cfa_iat_va:
                    cfa_calls.append(insn)
                elif target == cfw_iat_va:
                    cfw_calls.append(insn)
    
    print(f"\nDeviceIoControl calls found: {len(dio_calls)}")
    print(f"CreateFileA calls found: {len(cfa_calls)}")
    print(f"CreateFileW calls found: {len(cfw_calls)}")
    
    # Build instruction index for fast lookup
    insn_idx = {insn.address: i for i, insn in enumerate(all_insns)}
    
    # =====================================================
    # Analyze each DeviceIoControl call site
    # =====================================================
    print("\n" + "=" * 70)
    print("DEVICEIOCONTROL CALL SITE ANALYSIS")
    print("=" * 70)
    
    for call_num, dio_insn in enumerate(dio_calls):
        idx = insn_idx.get(dio_insn.address, -1)
        if idx < 0:
            continue
        
        print(f"\n--- DeviceIoControl Call #{call_num+1} at 0x{dio_insn.address:X} ---")
        
        # x64 calling convention (Microsoft):
        # rcx = hDevice
        # rdx = dwIoControlCode  
        # r8  = lpInBuffer
        # r9  = nInBufferSize
        # [rsp+0x20] = lpOutBuffer
        # [rsp+0x28] = nOutBufferSize  
        # [rsp+0x30] = lpBytesReturned
        # [rsp+0x38] = lpOverlapped
        
        # Look backwards to find register setup
        start = max(0, idx - 40)
        end = min(len(all_insns), idx + 5)
        
        ioctl_code = "?"
        for j in range(idx - 1, start, -1):
            prev = all_insns[j]
            # Look for: mov edx, 0x9C40XXXX  or  mov rdx, xxx
            op = prev.op_str.lower()
            if prev.mnemonic == 'mov' and (op.startswith('edx,') or op.startswith('rdx,')):
                parts = op.split(',')
                if len(parts) == 2:
                    val_str = parts[1].strip()
                    if val_str.startswith('0x'):
                        try:
                            val = int(val_str, 16)
                            if (val & 0xFFFFFF00) == 0x9C402400:
                                ioctl_name = ioctls.get(val, "UNKNOWN")
                                ioctl_code = f"0x{val:08X} ({ioctl_name})"
                            else:
                                ioctl_code = f"0x{val:X}"
                        except:
                            ioctl_code = val_str
                    else:
                        ioctl_code = val_str
                break  # Found the rdx setup
            # Check for lea rdx that might load from a variable
            if prev.mnemonic == 'lea' and (op.startswith('edx,') or op.startswith('rdx,')):
                ioctl_code = f"[from memory: {prev.op_str}]"
                break
        
        print(f"  IOCTL Code (rdx): {ioctl_code}")
        
        # Print context
        for j in range(start, end):
            ins = all_insns[j]
            marker = " >>>" if j == idx else "    "
            hexb = ' '.join(f'{b:02X}' for b in ins.bytes[:10])
            print(f"{marker} 0x{ins.address:X}: {hexb:30s} {ins.mnemonic:8s} {ins.op_str}")
    
    # =====================================================
    # Analyze CreateFileA calls
    # =====================================================
    print("\n" + "=" * 70)
    print("CREATEFILEA CALL SITE ANALYSIS")
    print("=" * 70)
    
    for call_num, cf_insn in enumerate(cfa_calls):
        idx = insn_idx.get(cf_insn.address, -1)
        if idx < 0:
            continue
        
        print(f"\n--- CreateFileA Call #{call_num+1} at 0x{cf_insn.address:X} ---")
        
        # x64: rcx = lpFileName
        start = max(0, idx - 30)
        end = min(len(all_insns), idx + 5)
        
        for j in range(start, end):
            ins = all_insns[j]
            marker = " >>>" if j == idx else "    "
            hexb = ' '.join(f'{b:02X}' for b in ins.bytes[:10])
            print(f"{marker} 0x{ins.address:X}: {hexb:30s} {ins.mnemonic:8s} {ins.op_str}")
    
    # =====================================================
    # Now find the FUNCTIONS that contain IOCTL calls
    # =====================================================
    print("\n" + "=" * 70)
    print("FUNCTION-LEVEL ANALYSIS (functions containing IOCTLs)")
    print("=" * 70)
    
    # For each IOCTL code location found in binary, disassemble around it
    for ioctl_val, ioctl_name in ioctls.items():
        needle = struct.pack('<I', ioctl_val)
        
        offset = 0
        while True:
            pos = data.find(needle, offset)
            if pos == -1:
                break
            offset = pos + 1
            
            # Only care about .text section occurrences
            rva = fileoff_to_rva(pe, pos)
            if rva is None:
                continue
            if not (text_sec.VirtualAddress <= rva < text_sec.VirtualAddress + text_sec.SizeOfRawData):
                continue
            
            va = base + rva
            
            # Find function start by scanning backwards for common prologue
            # Look for the instruction containing this constant
            func_start_va = None
            for insn in all_insns:
                if insn.address <= va <= insn.address + insn.size:
                    # Found the instruction. Now scan backwards for function start
                    insn_i = insn_idx.get(insn.address, -1)
                    if insn_i >= 0:
                        # Scan backwards for sub rsp or push rbx/rbp pattern
                        for k in range(insn_i, max(0, insn_i - 200), -1):
                            prev = all_insns[k]
                            if prev.mnemonic == 'sub' and 'rsp' in prev.op_str:
                                # Likely function start near here
                                func_start_va = all_insns[max(0, k-3)].address
                                break
                            if prev.mnemonic == 'int3' or (prev.mnemonic == 'ret' and k < insn_i - 1):
                                func_start_va = all_insns[k+1].address
                                break
                    break
            
            print(f"\n{'='*60}")
            print(f"Function containing {ioctl_name} (0x{ioctl_val:08X})")
            print(f"IOCTL at VA=0x{va:X}, estimated function start=0x{func_start_va:X}" if func_start_va else f"IOCTL at VA=0x{va:X}")
            print(f"{'='*60}")
            
            # Print the function
            if func_start_va:
                start_idx = insn_idx.get(func_start_va, 0)
            else:
                # Find instruction containing va
                start_idx = 0
                for insn in all_insns:
                    if insn.address >= va - 128:
                        start_idx = insn_idx[insn.address]
                        break
            
            printed = 0
            ret_count = 0
            for k in range(start_idx, min(len(all_insns), start_idx + 200)):
                ins = all_insns[k]
                # Highlight IOCTL-related instructions
                highlight = ""
                if any(f'0x{ioctl_val & 0xFF:x}' in ins.op_str.lower() and '0x9c' in ins.op_str.lower()
                       for _ in [1]):
                    highlight = " <-- IOCTL CODE"
                elif '0x131ca08' in ins.op_str.lower() or '0x10100' in ins.op_str.lower():
                    highlight = " <-- MAGIC"
                elif 'deviceiocontrol' in ins.op_str.lower() or (ins.mnemonic == 'call' and 'rip' in ins.op_str):
                    # Check if this is a call to DeviceIoControl
                    if len(ins.bytes) >= 6 and ins.bytes[0] == 0xFF and ins.bytes[1] == 0x15:
                        disp = struct.unpack_from('<i', bytes(ins.bytes), 2)[0]
                        target = ins.address + ins.size + disp
                        if target == dio_iat_va:
                            highlight = " <-- CALL DeviceIoControl"
                        elif target == cfa_iat_va:
                            highlight = " <-- CALL CreateFileA"
                        elif target == cfw_iat_va:
                            highlight = " <-- CALL CreateFileW"
                
                hexb = ' '.join(f'{b:02X}' for b in ins.bytes[:10])
                print(f"  0x{ins.address:X}: {hexb:30s} {ins.mnemonic:8s} {ins.op_str}{highlight}")
                
                if ins.mnemonic == 'ret':
                    ret_count += 1
                    if ret_count >= 2:
                        break
                printed += 1

    # =====================================================
    # Find the GUID {5B722BF8-...} and "wave%d" strings usage
    # =====================================================
    print("\n" + "=" * 70)
    print("DEVICE ENUMERATION ANALYSIS")
    print("=" * 70)
    
    # The GUID string {5B722BF8-F0AB-47EE-B9C8-8D61D31375A3} at offset 0x035B20
    # "WDM2VST Driver" at offset 0x035F88  
    # "wave%d" at offset 0x035FA8
    # These are UTF-16 strings in .rdata
    
    # Search for cross-references to these string addresses
    # In x64, LEA instructions typically load string addresses
    guid_rva = fileoff_to_rva(pe, 0x035B20)
    driver_name_rva = fileoff_to_rva(pe, 0x035F88)
    wave_fmt_rva = fileoff_to_rva(pe, 0x035FA8)
    shm_name_rva = fileoff_to_rva(pe, 0x0365B8)
    reg_path_rva = fileoff_to_rva(pe, 0x036610)
    
    print(f"  GUID string RVA: 0x{guid_rva:X}" if guid_rva else "  GUID not found")
    print(f"  'WDM2VST Driver' RVA: 0x{driver_name_rva:X}" if driver_name_rva else "  driver name not found")
    print(f"  'wave%d' RVA: 0x{wave_fmt_rva:X}" if wave_fmt_rva else "  wave%d not found")
    print(f"  SHM name RVA: 0x{shm_name_rva:X}" if shm_name_rva else "  SHM not found")
    print(f"  Registry path RVA: 0x{reg_path_rva:X}" if reg_path_rva else "  Reg path not found")
    
    # Find LEA instructions that reference these RVAs
    target_rvas = {
        guid_rva: "GUID {5B722BF8-...}",
        driver_name_rva: "WDM2VST Driver",
        wave_fmt_rva: "wave%d",
        shm_name_rva: "SharedMemory Name",
        reg_path_rva: "Registry Path",
    }
    
    for insn in all_insns:
        if insn.mnemonic == 'lea' and 'rip' in insn.op_str:
            # LEA reg, [rip+disp32]
            # Need to compute effective address
            # Common encoding: 48 8D xx xx xx xx xx
            raw = bytes(insn.bytes)
            # Find the displacement (last 4 bytes typically for rip+disp32)
            if len(raw) >= 7:
                # The disp32 is at the end of the instruction
                disp = struct.unpack_from('<i', raw, len(raw) - 4)[0]
                effective_va = insn.address + insn.size + disp
                effective_rva = effective_va - base
                
                for trva, tname in target_rvas.items():
                    if trva and abs(effective_rva - trva) < 4:
                        idx2 = insn_idx.get(insn.address, -1)
                        if idx2 >= 0:
                            print(f"\n  Reference to '{tname}' at 0x{insn.address:X}:")
                            st = max(0, idx2 - 5)
                            en = min(len(all_insns), idx2 + 20)
                            for k in range(st, en):
                                ins2 = all_insns[k]
                                m = " >>>" if k == idx2 else "    "
                                hx = ' '.join(f'{b:02X}' for b in ins2.bytes[:10])
                                print(f"  {m} 0x{ins2.address:X}: {hx:30s} {ins2.mnemonic:8s} {ins2.op_str}")


def main():
    pe = pefile.PE(DLL_PATH)
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    md.detail = False
    
    find_ioctl_functions(pe, md)


if __name__ == '__main__':
    main()
