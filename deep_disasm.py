"""
Deep disassembly of wdm2vst.dll to understand driver connection protocol.
Focus: IOCTL codes, shared memory, device enumeration, and the VST->Driver bridge.
"""
import pefile
import struct
import sys
import os

DLL_PATH = r"C:\Program Files\VSTPlugins\Tools\wdm2vst.dll"

def analyze_pe(path):
    pe = pefile.PE(path)
    print(f"=== PE Analysis: {os.path.basename(path)} ===")
    print(f"Machine: {hex(pe.FILE_HEADER.Machine)}")
    print(f"ImageBase: {hex(pe.OPTIONAL_HEADER.ImageBase)}")
    print(f"EntryPoint RVA: {hex(pe.OPTIONAL_HEADER.AddressOfEntryPoint)}")
    print(f"Number of sections: {pe.FILE_HEADER.NumberOfSections}")
    print()

    # Sections
    print("--- Sections ---")
    for sec in pe.sections:
        name = sec.Name.rstrip(b'\x00').decode('ascii', errors='replace')
        print(f"  {name:10s} VA={hex(sec.VirtualAddress)} Size={sec.SizeOfRawData} VSize={sec.Misc_VirtualSize} Chars={hex(sec.Characteristics)}")
    print()

    # Exports
    print("--- Exports ---")
    if hasattr(pe, 'DIRECTORY_ENTRY_EXPORT'):
        for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
            name = exp.name.decode() if exp.name else f"ord_{exp.ordinal}"
            print(f"  {name} @ RVA {hex(exp.address)} (ordinal {exp.ordinal})")
    print()

    # Imports
    print("--- Imports ---")
    if hasattr(pe, 'DIRECTORY_ENTRY_IMPORT'):
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            dll_name = entry.dll.decode()
            funcs = []
            for imp in entry.imports:
                fname = imp.name.decode() if imp.name else f"ord_{imp.ordinal}"
                funcs.append(fname)
            # Only show interesting imports
            interesting = [f for f in funcs if any(k in f.lower() for k in [
                'createfile', 'deviceio', 'setupdi', 'openfile', 'mapview',
                'createfilemapping', 'virtualalloc', 'virtualfree',
                'regopen', 'regquery', 'regclose', 'regenum',
                'regcreate', 'regset', 'loadlib', 'getproc',
                'cocreate', 'coinitial', 'getdevice', 'enum',
                'closehandle', 'waitforsingle', 'createevent',
                'setevent', 'resetevent', 'sleep', 'createthread',
                'getlasterror', 'heapalloc', 'heapfree',
                'multibytetowidechar', 'widechartomultibyte',
                'outputdebug', 'messagebox'
            ])]
            if interesting:
                print(f"  [{dll_name}]  ({len(funcs)} total imports)")
                for f in interesting:
                    print(f"    -> {f}")
            else:
                print(f"  [{dll_name}]  ({len(funcs)} total imports, no interesting ones)")
    print()
    return pe


def find_strings(pe, min_len=6):
    """Find all ASCII + Unicode strings in the PE data."""
    data = pe.__data__
    results = []

    # ASCII strings
    i = 0
    while i < len(data):
        # Check for ASCII printable sequence
        start = i
        while i < len(data) and 32 <= data[i] < 127:
            i += 1
        if i - start >= min_len:
            s = data[start:i].decode('ascii', errors='replace')
            results.append(('ASCII', start, s))
        i += 1

    # Unicode strings (UTF-16LE)
    i = 0
    while i < len(data) - 1:
        start = i
        chars = []
        while i < len(data) - 1:
            c = struct.unpack_from('<H', data, i)[0]
            if 32 <= c < 127:
                chars.append(chr(c))
                i += 2
            else:
                break
        if len(chars) >= min_len:
            s = ''.join(chars)
            # Skip if it's just the ASCII version (ASCII chars interleaved with 0x00)
            results.append(('UTF16', start, s))
        i += 2

    return results


def find_interesting_strings(pe):
    """Find strings related to driver communication."""
    print("--- Interesting Strings ---")
    keywords = [
        'wdm2vst', 'vst2wdm', 'asiovadpro', 'odeus', 'Global\\',
        'SharedMemory', 'Shared', '{', 'wave', 'KSCATEGORY',
        'NumDevice', 'NumChannel', 'SampleRate', 'BufferSize',
        'Attach', 'Detach', 'Start', 'Stop', 'Transfer',
        'IOCTL', 'ioctl', 'DeviceIoControl',
        'CreateFile', 'SetupDi', 'registry', 'Registry',
        'Software\\', 'SYSTEM\\', 'HKEY',
        'magic', 'Magic', '0131CA08',
        'processaudio', 'audio', 'buffer', 'ring',
        'bridge', 'Bridge',
        'error', 'Error', 'fail', 'Fail',
        'debug', 'Debug', 'log', 'Log',
        'version', 'Version',
        'channel', 'Channel',
        'render', 'Render', 'capture', 'Capture',
        'poll', 'Poll', 'wait', 'Wait',
        'event', 'Event',
        'mutex', 'Mutex', 'semaphore',
        'thread', 'Thread',
        'dll', 'DLL', 'driver', 'Driver',
        '6994AD04', 'GUID',
        'sample', 'Sample', 'format', 'Format',
        'WaveFormat', 'WAVEFORMAT',
    ]

    strings = find_strings(pe, min_len=4)
    found = set()
    for encoding, offset, s in strings:
        s_lower = s.lower()
        for kw in keywords:
            if kw.lower() in s_lower and s not in found:
                found.add(s)
                # Find which section this offset is in
                sec_name = "?"
                for sec in pe.sections:
                    if sec.PointerToRawData <= offset < sec.PointerToRawData + sec.SizeOfRawData:
                        sec_name = sec.Name.rstrip(b'\x00').decode('ascii', errors='replace')
                        break
                print(f"  [{encoding}] @0x{offset:06X} ({sec_name}): {s[:120]}")
                break
    print()
    return found


def find_constants(pe):
    """Search for known IOCTL codes and magic numbers in the binary."""
    data = pe.__data__
    print("--- IOCTL Codes & Magic Numbers ---")
    
    targets = {
        0x9C402410: "IOCTL_ATTACH",
        0x9C402414: "IOCTL_DETACH",
        0x9C402420: "IOCTL_START",
        0x9C402424: "IOCTL_STOP",
        0x9C402428: "IOCTL_TRANSFER",
        0x0131CA08: "MAGIC_VALUE",
        0x9C402400: "IOCTL_BASE?",
        0x9C40240C: "IOCTL_INIT?",
        0x9C402418: "IOCTL_?_18",
        0x9C40241C: "IOCTL_?_1C",
        0x9C40242C: "IOCTL_?_2C",
        0x9C402430: "IOCTL_?_30",
    }
    
    for val, name in targets.items():
        # Search for 32-bit LE
        needle = struct.pack('<I', val)
        idx = 0
        locations = []
        while True:
            idx = data.find(needle, idx)
            if idx == -1:
                break
            # Determine section
            sec_name = "?"
            rva = 0
            for sec in pe.sections:
                if sec.PointerToRawData <= idx < sec.PointerToRawData + sec.SizeOfRawData:
                    sec_name = sec.Name.rstrip(b'\x00').decode('ascii', errors='replace')
                    rva = sec.VirtualAddress + (idx - sec.PointerToRawData)
                    break
            locations.append((idx, sec_name, rva))
            idx += 1
        
        if locations:
            print(f"  {name} (0x{val:08X}):")
            for off, sec, rva in locations:
                print(f"    FileOff=0x{off:06X} Section={sec} RVA=0x{rva:06X}")
                # Show surrounding bytes for context
                ctx_start = max(0, off - 16)
                ctx_end = min(len(data), off + 20)
                ctx = data[ctx_start:ctx_end]
                hex_str = ' '.join(f'{b:02X}' for b in ctx)
                print(f"    Context: ...{hex_str}...")
    print()


def disasm_around(pe, file_offset, count=80):
    """Disassemble instructions around a file offset."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    
    data = pe.__data__
    
    # Find section and compute VA
    va_base = pe.OPTIONAL_HEADER.ImageBase
    rva = 0
    for sec in pe.sections:
        if sec.PointerToRawData <= file_offset < sec.PointerToRawData + sec.SizeOfRawData:
            rva = sec.VirtualAddress + (file_offset - sec.PointerToRawData)
            break
    
    va = va_base + rva
    
    # Start disassembling from a few bytes before to find instruction boundaries
    start = max(0, file_offset - 64)
    chunk = data[start:file_offset + count]
    start_va = va - (file_offset - start)
    
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    
    lines = []
    for insn in md.disasm(chunk, start_va):
        lines.append(insn)
    
    # Find the instruction at or near our target VA
    target_idx = 0
    for i, insn in enumerate(lines):
        if insn.address >= va:
            target_idx = i
            break
    
    # Print surrounding context
    start_idx = max(0, target_idx - 10)
    end_idx = min(len(lines), target_idx + 30)
    
    result = []
    for insn in lines[start_idx:end_idx]:
        marker = " >>>" if insn.address == va else "    "
        hex_bytes = ' '.join(f'{b:02X}' for b in insn.bytes)
        result.append(f"{marker} 0x{insn.address:08X}: {hex_bytes:30s} {insn.mnemonic:8s} {insn.op_str}")
    
    return '\n'.join(result)


def find_deviceiocontrol_calls(pe):
    """Find all calls to DeviceIoControl and analyze the IOCTL codes used."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    
    data = pe.__data__
    
    print("--- DeviceIoControl Call Analysis ---")
    
    # Find import address of DeviceIoControl
    dio_thunk = None
    if hasattr(pe, 'DIRECTORY_ENTRY_IMPORT'):
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            for imp in entry.imports:
                if imp.name and b'DeviceIoControl' in imp.name:
                    dio_thunk = imp.address  # This is the IAT VA
                    print(f"  DeviceIoControl IAT entry at VA: 0x{dio_thunk:08X}")
                    break
    
    if not dio_thunk:
        print("  DeviceIoControl not found in imports!")
        return
    
    # Disassemble the .text section
    text_sec = None
    for sec in pe.sections:
        name = sec.Name.rstrip(b'\x00').decode('ascii', errors='replace')
        if '.text' in name or 'CODE' in name:
            text_sec = sec
            break
    
    if not text_sec:
        # Try the first executable section
        for sec in pe.sections:
            if sec.Characteristics & 0x20000000:  # IMAGE_SCN_MEM_EXECUTE
                text_sec = sec
                break
    
    if not text_sec:
        print("  No executable section found!")
        return
    
    sec_name = text_sec.Name.rstrip(b'\x00').decode('ascii', errors='replace')
    print(f"  Scanning section: {sec_name}")
    
    code_data = data[text_sec.PointerToRawData:text_sec.PointerToRawData + text_sec.SizeOfRawData]
    code_va = pe.OPTIONAL_HEADER.ImageBase + text_sec.VirtualAddress
    
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    
    instructions = list(md.disasm(code_data, code_va))
    print(f"  Total instructions: {len(instructions)}")
    
    # Find CALL [DeviceIoControl] patterns
    # In 32-bit Delphi/MSVC, the IAT is called via: CALL DWORD PTR [IAT_addr]
    # Or via a thunk: CALL thunk_addr -> thunk: JMP DWORD PTR [IAT_addr]
    
    # First, find any JMP [IAT] thunks
    thunk_addrs = set()
    thunk_addrs.add(dio_thunk)
    
    for insn in instructions:
        if insn.mnemonic == 'jmp' and f'0x{dio_thunk:x}' in insn.op_str:
            thunk_addrs.add(insn.address)
            print(f"  Found thunk at 0x{insn.address:08X}: {insn.mnemonic} {insn.op_str}")
    
    # Now find all CALL instructions that reference DeviceIoControl
    call_sites = []
    for i, insn in enumerate(instructions):
        if insn.mnemonic == 'call':
            # Check if calling the IAT entry or a thunk
            is_dio = False
            for t in thunk_addrs:
                if f'0x{t:x}' in insn.op_str:
                    is_dio = True
                    break
            
            if is_dio:
                call_sites.append((i, insn))
    
    print(f"\n  Found {len(call_sites)} DeviceIoControl call sites:\n")
    
    for idx, (i, call_insn) in enumerate(call_sites):
        print(f"  === Call Site #{idx+1} at 0x{call_insn.address:08X} ===")
        
        # Look backwards to find the IOCTL code being pushed
        # DeviceIoControl(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, 
        #                 lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped)
        # In stdcall, args pushed right-to-left:
        # push lpOverlapped
        # push lpBytesReturned
        # push nOutBufferSize  
        # push lpOutBuffer
        # push nInBufferSize
        # push lpInBuffer
        # push dwIoControlCode  <-- this is what we want (2nd arg, 7th push back)
        # push hDevice
        # call DeviceIoControl
        
        # Print context: 20 instructions before, 5 after
        start_ctx = max(0, i - 25)
        end_ctx = min(len(instructions), i + 5)
        
        push_count = 0
        ioctl_code = None
        for j in range(i - 1, max(0, i - 30), -1):
            prev = instructions[j]
            if prev.mnemonic == 'push':
                push_count += 1
                if push_count == 7:  # 7th push back = dwIoControlCode
                    # Try to extract immediate value
                    if prev.op_str.startswith('0x') or prev.op_str.startswith('-'):
                        try:
                            ioctl_code = int(prev.op_str, 16) if prev.op_str.startswith('0x') else int(prev.op_str)
                        except:
                            ioctl_code = prev.op_str
                    else:
                        ioctl_code = prev.op_str
        
        if ioctl_code is not None:
            if isinstance(ioctl_code, int):
                known = {
                    0x9C402410: "ATTACH", 0x9C402414: "DETACH",
                    0x9C402420: "START", 0x9C402424: "STOP",
                    0x9C402428: "TRANSFER"
                }
                label = known.get(ioctl_code & 0xFFFFFFFF, "UNKNOWN")
                print(f"  IOCTL Code: 0x{ioctl_code & 0xFFFFFFFF:08X} ({label})")
            else:
                print(f"  IOCTL Code: {ioctl_code} (from register/memory)")
        
        # Print the assembly context
        for j in range(start_ctx, end_ctx):
            ins = instructions[j]
            marker = " >>>" if j == i else "    "
            hex_bytes = ' '.join(f'{b:02X}' for b in ins.bytes[:8])
            print(f"{marker} 0x{ins.address:08X}: {hex_bytes:24s} {ins.mnemonic:8s} {ins.op_str}")
        print()


def find_createfile_calls(pe):
    """Find all calls to CreateFileA/W to understand what device paths are opened."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    
    data = pe.__data__
    
    print("--- CreateFile Call Analysis ---")
    
    # Find import addresses
    cf_thunks = {}
    if hasattr(pe, 'DIRECTORY_ENTRY_IMPORT'):
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            for imp in entry.imports:
                if imp.name:
                    name = imp.name.decode()
                    if 'CreateFile' in name:
                        cf_thunks[name] = imp.address
                        print(f"  {name} IAT at VA: 0x{imp.address:08X}")
    
    if not cf_thunks:
        print("  No CreateFile imports found!")
        return
    
    # Find executable section
    text_sec = None
    for sec in pe.sections:
        if sec.Characteristics & 0x20000000:
            text_sec = sec
            break
    
    if not text_sec:
        return
    
    code_data = data[text_sec.PointerToRawData:text_sec.PointerToRawData + text_sec.SizeOfRawData]
    code_va = pe.OPTIONAL_HEADER.ImageBase + text_sec.VirtualAddress
    
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    instructions = list(md.disasm(code_data, code_va))
    
    # Find thunks
    all_thunks = set()
    for name, addr in cf_thunks.items():
        all_thunks.add(addr)
        for insn in instructions:
            if insn.mnemonic == 'jmp' and f'0x{addr:x}' in insn.op_str:
                all_thunks.add(insn.address)
                print(f"  Found thunk for {name} at 0x{insn.address:08X}")
    
    # Find calls
    for i, insn in enumerate(instructions):
        if insn.mnemonic == 'call':
            is_cf = False
            for t in all_thunks:
                if f'0x{t:x}' in insn.op_str:
                    is_cf = True
                    break
            
            if is_cf:
                print(f"\n  CreateFile call at 0x{insn.address:08X}:")
                start_ctx = max(0, i - 20)
                end_ctx = min(len(instructions), i + 5)
                for j in range(start_ctx, end_ctx):
                    ins = instructions[j]
                    marker = " >>>" if j == i else "    "
                    hex_bytes = ' '.join(f'{b:02X}' for b in ins.bytes[:8])
                    print(f"{marker} 0x{ins.address:08X}: {hex_bytes:24s} {ins.mnemonic:8s} {ins.op_str}")
    print()


def find_setupdi_calls(pe):
    """Find SetupDi calls for device enumeration."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    
    data = pe.__data__
    print("--- SetupDi (Device Enumeration) Call Analysis ---")
    
    setupdi_thunks = {}
    if hasattr(pe, 'DIRECTORY_ENTRY_IMPORT'):
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            for imp in entry.imports:
                if imp.name and b'SetupDi' in imp.name:
                    name = imp.name.decode()
                    setupdi_thunks[name] = imp.address
                    print(f"  {name} IAT at VA: 0x{imp.address:08X}")
    
    if not setupdi_thunks:
        print("  No SetupDi imports found - driver uses alternative enumeration!")
        return
    print()


def find_shared_memory_ops(pe):
    """Find shared memory creation/opening."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    
    data = pe.__data__
    print("--- Shared Memory Operations ---")
    
    shm_funcs = ['CreateFileMappingA', 'CreateFileMappingW', 
                 'OpenFileMappingA', 'OpenFileMappingW',
                 'MapViewOfFile', 'UnmapViewOfFile']
    
    found_funcs = {}
    if hasattr(pe, 'DIRECTORY_ENTRY_IMPORT'):
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            for imp in entry.imports:
                if imp.name:
                    name = imp.name.decode()
                    if name in shm_funcs:
                        found_funcs[name] = imp.address
                        print(f"  {name} IAT at VA: 0x{imp.address:08X}")
    
    if not found_funcs:
        print("  No shared memory imports found!")
    print()


def find_registry_ops(pe):
    """Find registry operations."""
    print("--- Registry Operations ---")
    
    reg_funcs = []
    if hasattr(pe, 'DIRECTORY_ENTRY_IMPORT'):
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            for imp in entry.imports:
                if imp.name:
                    name = imp.name.decode()
                    if name.startswith('Reg') and any(k in name for k in ['Open', 'Query', 'Create', 'Set', 'Close', 'Enum']):
                        reg_funcs.append((name, imp.address))
                        print(f"  {name} IAT at VA: 0x{imp.address:08X}")
    
    if not reg_funcs:
        print("  No registry imports found!")
    print()


def scan_all_dwords(pe):
    """Scan for DWORD patterns that look like IOCTL codes (0x9C4024XX)."""
    data = pe.__data__
    print("--- Full IOCTL Code Scan (0x9C4024XX pattern) ---")
    
    for i in range(0, len(data) - 3):
        val = struct.unpack_from('<I', data, i)[0]
        if (val & 0xFFFFFF00) == 0x9C402400:
            sec_name = "?"
            rva = 0
            for sec in pe.sections:
                if sec.PointerToRawData <= i < sec.PointerToRawData + sec.SizeOfRawData:
                    sec_name = sec.Name.rstrip(b'\x00').decode('ascii', errors='replace')
                    rva = sec.VirtualAddress + (i - sec.PointerToRawData)
                    break
            print(f"  0x{val:08X} at FileOff=0x{i:06X} ({sec_name}) RVA=0x{rva:06X}")
    print()


def analyze_function_at(pe, rva, name="unknown"):
    """Disassemble a function starting at the given RVA."""
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    
    data = pe.__data__
    va_base = pe.OPTIONAL_HEADER.ImageBase
    
    # Convert RVA to file offset
    file_off = None
    for sec in pe.sections:
        if sec.VirtualAddress <= rva < sec.VirtualAddress + sec.SizeOfRawData:
            file_off = sec.PointerToRawData + (rva - sec.VirtualAddress)
            break
    
    if file_off is None:
        print(f"  Cannot find file offset for RVA 0x{rva:X}")
        return
    
    chunk = data[file_off:file_off + 2048]
    start_va = va_base + rva
    
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    
    print(f"\n--- Function: {name} at RVA=0x{rva:06X} VA=0x{start_va:08X} ---")
    
    line_count = 0
    for insn in md.disasm(chunk, start_va):
        hex_bytes = ' '.join(f'{b:02X}' for b in insn.bytes[:10])
        print(f"  0x{insn.address:08X}: {hex_bytes:30s} {insn.mnemonic:8s} {insn.op_str}")
        line_count += 1
        # Stop at RET or after reasonable number of instructions
        if insn.mnemonic == 'ret' and line_count > 5:
            break
        if line_count > 200:
            print("  ... (truncated)")
            break
    print()


def main():
    pe = analyze_pe(DLL_PATH)
    
    # Find interesting strings
    find_interesting_strings(pe)
    
    # Find IOCTL codes in binary
    find_constants(pe)
    
    # Scan for all IOCTL patterns
    scan_all_dwords(pe)
    
    # Find shared memory operations
    find_shared_memory_ops(pe)
    
    # Find registry operations
    find_registry_ops(pe)
    
    # Find SetupDi calls
    find_setupdi_calls(pe)
    
    # Find CreateFile calls  
    find_createfile_calls(pe)
    
    # Find DeviceIoControl calls
    find_deviceiocontrol_calls(pe)
    
    # Also analyze the exports (VST entry points)
    if hasattr(pe, 'DIRECTORY_ENTRY_EXPORT'):
        for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
            if exp.name:
                name = exp.name.decode()
                print(f"\n{'='*60}")
                analyze_function_at(pe, exp.address, name)


if __name__ == '__main__':
    main()
