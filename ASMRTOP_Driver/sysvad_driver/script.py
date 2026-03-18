import os

def replace_speakertoptable():
    with open('EndpointsCommon/speakertoptable.h', 'r', encoding='utf-8') as f:
        content = f.read()
    
    # define GUIDs at the top
    guids = '''// Custom Endpoint names
DEFINE_GUID(SPEAKER_CUSTOM_NAME_1, 0xc40e9164, 0xa8ae, 0x4363, 0x80, 0xde, 0x3a, 0x39, 0x82, 0x7b, 0x7b, 0xb1);
DEFINE_GUID(SPEAKER_CUSTOM_NAME_2, 0xc40e9164, 0xa8ae, 0x4363, 0x80, 0xde, 0x3a, 0x39, 0x82, 0x7b, 0x7b, 0xb2);
DEFINE_GUID(SPEAKER_CUSTOM_NAME_3, 0xc40e9164, 0xa8ae, 0x4363, 0x80, 0xde, 0x3a, 0x39, 0x82, 0x7b, 0x7b, 0xb3);
DEFINE_GUID(SPEAKER_CUSTOM_NAME_4, 0xc40e9164, 0xa8ae, 0x4363, 0x80, 0xde, 0x3a, 0x39, 0x82, 0x7b, 0x7b, 0xb4);
'''
    content = content.replace('static\r\nKSDATARANGE', guids + '\nstatic\nKSDATARANGE')
    content = content.replace('static\nKSDATARANGE', guids + '\nstatic\nKSDATARANGE')

    # Remove the old SpeakerTopoMiniportPins
    p_start = content.find('static\\r\\nPCPIN_DESCRIPTOR SpeakerTopoMiniportPins[]')
    if p_start == -1:
        p_start = content.find('static\\nPCPIN_DESCRIPTOR SpeakerTopoMiniportPins[]')
    
    # Just replace it entirely manually with the 4 copies
    old_pins = content[content.find('static\\nPCPIN_DESCRIPTOR SpeakerTopoMiniportPins[]'):content.find('//=============================================================================\\nstatic\\nKSJACK_DESCRIPTION SpeakerJackDescBridge')]
    if len(old_pins) < 100: # try \r\n
        old_pins = content[content.find('static\\r\\nPCPIN_DESCRIPTOR SpeakerTopoMiniportPins[]'):content.find('//=============================================================================\\r\\nstatic\\r\\nKSJACK_DESCRIPTION SpeakerJackDescBridge')]

    new_pins = ''
    for i in range(1, 5):
        new_pin = old_pins.replace('SpeakerTopoMiniportPins[]', f'SpeakerTopoMiniportPins{i}[]')
        new_pin = new_pin.replace('NULL,                                             // Name', f'&SPEAKER_CUSTOM_NAME_{i},                           // Name')
        new_pins += new_pin + '\n'
        
    content = content.replace(old_pins, new_pins)
    
    old_descriptor = content[content.find('static\\nPCFILTER_DESCRIPTOR SpeakerTopoMiniportFilterDescriptor'):]
    if len(old_descriptor) < 100:
        old_descriptor = content[content.find('static\\r\\nPCFILTER_DESCRIPTOR SpeakerTopoMiniportFilterDescriptor'):]
    
    # cut before #endif
    old_descriptor = old_descriptor[:old_descriptor.find('#endif')]
    
    new_descriptors = ''
    for i in range(1, 5):
        new_desc = old_descriptor.replace('SpeakerTopoMiniportFilterDescriptor', f'SpeakerTopoMiniportFilterDescriptor{i}')
        new_desc = new_desc.replace('SpeakerTopoMiniportPins,', f'SpeakerTopoMiniportPins{i},')
        new_desc = new_desc.replace('SIZEOF_ARRAY(SpeakerTopoMiniportPins)', f'SIZEOF_ARRAY(SpeakerTopoMiniportPins{i})')
        new_descriptors += new_desc + '\n'
        
    content = content.replace(old_descriptor, new_descriptors)
    
    with open('EndpointsCommon/speakertoptable.h', 'w', encoding='utf-8') as f:
        f.write(content)

def replace_micintoptable():
    with open('TabletAudioSample/micintoptable.h', 'r', encoding='utf-8') as f:
        content = f.read()
    
    # replace old mic guid with 4 new mic guids
    old_guid_def = '''//
// {d48deb08-fd1c-4d1e-b821-9064d49ae96e}
DEFINE_GUID(MICIN_CUSTOM_NAME, 
0xd48deb08, 0xfd1c, 0x4d1e, 0xb8, 0x21, 0x90, 0x64, 0xd4, 0x9a, 0xe9, 0x6e);'''
    
    new_guids = '''DEFINE_GUID(MICIN_CUSTOM_NAME_1, 0xd48deb08, 0xfd1c, 0x4d1e, 0xb8, 0x21, 0x90, 0x64, 0xd4, 0x9a, 0xe9, 0x61);
DEFINE_GUID(MICIN_CUSTOM_NAME_2, 0xd48deb08, 0xfd1c, 0x4d1e, 0xb8, 0x21, 0x90, 0x64, 0xd4, 0x9a, 0xe9, 0x62);
DEFINE_GUID(MICIN_CUSTOM_NAME_3, 0xd48deb08, 0xfd1c, 0x4d1e, 0xb8, 0x21, 0x90, 0x64, 0xd4, 0x9a, 0xe9, 0x63);
DEFINE_GUID(MICIN_CUSTOM_NAME_4, 0xd48deb08, 0xfd1c, 0x4d1e, 0xb8, 0x21, 0x90, 0x64, 0xd4, 0x9a, 0xe9, 0x64);
'''
    content = content.replace(old_guid_def, new_guids)
    
    # Remove the old MicInTopoMiniportPins
    old_pins = content[content.find('static\\r\\nPCPIN_DESCRIPTOR MicInTopoMiniportPins[]'):content.find('//=============================================================================\\r\\n// Only return a KSJACK_DESCRIPTION for the physical bridge pin.')]
    if len(old_pins) < 10:
        old_pins = content[content.find('static\\nPCPIN_DESCRIPTOR MicInTopoMiniportPins[]'):content.find('//=============================================================================\\n// Only return a KSJACK_DESCRIPTION for the physical bridge pin.')]
    
    new_pins = ''
    for i in range(1, 5):
        new_pin = old_pins.replace('MicInTopoMiniportPins[]', f'MicInTopoMiniportPins{i}[]')
        new_pin = new_pin.replace('&MICIN_CUSTOM_NAME,', f'&MICIN_CUSTOM_NAME_{i},')
        new_pins += new_pin + '\n'
        
    content = content.replace(old_pins, new_pins)
    
    old_descriptor = content[content.find('static\\r\\nPCFILTER_DESCRIPTOR MicInTopoMiniportFilterDescriptor'):]
    if len(old_descriptor) < 10:
        old_descriptor = content[content.find('static\\nPCFILTER_DESCRIPTOR MicInTopoMiniportFilterDescriptor'):]
    
    old_descriptor = old_descriptor[:old_descriptor.find('#endif')]
    
    new_descriptors = ''
    for i in range(1, 5):
        new_desc = old_descriptor.replace('MicInTopoMiniportFilterDescriptor', f'MicInTopoMiniportFilterDescriptor{i}')
        new_desc = new_desc.replace('MicInTopoMiniportPins,', f'MicInTopoMiniportPins{i},')
        new_desc = new_desc.replace('SIZEOF_ARRAY(MicInTopoMiniportPins)', f'SIZEOF_ARRAY(MicInTopoMiniportPins{i})')
        new_descriptors += new_desc + '\n'
        
    content = content.replace(old_descriptor, new_descriptors)
    
    with open('TabletAudioSample/micintoptable.h', 'w', encoding='utf-8') as f:
        f.write(content)

def replace_minipairs():
    with open('TabletAudioSample/minipairs.h', 'r', encoding='utf-8') as f:
        content = f.read()

    # speakers
    content = content.replace('&SpeakerTopoMiniportFilterDescriptor,', '&SpeakerTopoMiniportFilterDescriptor1,', 1)
    content = content.replace('&SpeakerTopoMiniportFilterDescriptor,', '&SpeakerTopoMiniportFilterDescriptor2,', 1)
    content = content.replace('&SpeakerTopoMiniportFilterDescriptor,', '&SpeakerTopoMiniportFilterDescriptor3,', 1)
    content = content.replace('&SpeakerTopoMiniportFilterDescriptor,', '&SpeakerTopoMiniportFilterDescriptor4,', 1)
    
    # mics
    content = content.replace('&MicInTopoMiniportFilterDescriptor,', '&MicInTopoMiniportFilterDescriptor1,', 1)
    content = content.replace('&MicInTopoMiniportFilterDescriptor,', '&MicInTopoMiniportFilterDescriptor2,', 1)
    content = content.replace('&MicInTopoMiniportFilterDescriptor,', '&MicInTopoMiniportFilterDescriptor3,', 1)
    content = content.replace('&MicInTopoMiniportFilterDescriptor,', '&MicInTopoMiniportFilterDescriptor4,', 1)
    
    with open('TabletAudioSample/minipairs.h', 'w', encoding='utf-8') as f:
        f.write(content)

def replace_inx():
    inxs = ['TabletAudioSample/ComponentizedAudioSample.inx', 'TabletAudioSample/ComponentizedAudioSampleExtension.inx', 'TabletAudioSample/ComponentizedAudioSample_utf8.inx', 'TabletAudioSample/ASMRTOP.inx']
    for fpath in inxs:
        if not os.path.exists(fpath): continue
        with open(fpath, 'r', encoding='utf-8') as f:
            content = f.read()
            
        old_hkrs = '''HKR,%MEDIA_CATEGORIES%\\%MicArray1CustomNameGUID%,Name,,%MicArray1CustomName%
HKR,%MEDIA_CATEGORIES%\\%MicArray2CustomNameGUID%,Name,,%MicArray2CustomName%
HKR,%MEDIA_CATEGORIES%\\%MicArray3CustomNameGUID%,Name,,%MicArray3CustomName%
HKR,%MEDIA_CATEGORIES%\\%MicInCustomNameGUID%,Name,,%MicInCustomName%'''
        new_hkrs = '''HKR,%MEDIA_CATEGORIES%\\%SpeakerCustomName1GUID%,Name,,%SpeakerCustomName1%
HKR,%MEDIA_CATEGORIES%\\%SpeakerCustomName2GUID%,Name,,%SpeakerCustomName2%
HKR,%MEDIA_CATEGORIES%\\%SpeakerCustomName3GUID%,Name,,%SpeakerCustomName3%
HKR,%MEDIA_CATEGORIES%\\%SpeakerCustomName4GUID%,Name,,%SpeakerCustomName4%
HKR,%MEDIA_CATEGORIES%\\%MicArray1CustomNameGUID%,Name,,%MicArray1CustomName%
HKR,%MEDIA_CATEGORIES%\\%MicArray2CustomNameGUID%,Name,,%MicArray2CustomName%
HKR,%MEDIA_CATEGORIES%\\%MicArray3CustomNameGUID%,Name,,%MicArray3CustomName%
HKR,%MEDIA_CATEGORIES%\\%MicInCustomName1GUID%,Name,,%MicInCustomName1%
HKR,%MEDIA_CATEGORIES%\\%MicInCustomName2GUID%,Name,,%MicInCustomName2%
HKR,%MEDIA_CATEGORIES%\\%MicInCustomName3GUID%,Name,,%MicInCustomName3%
HKR,%MEDIA_CATEGORIES%\\%MicInCustomName4GUID%,Name,,%MicInCustomName4%'''
        content = content.replace(old_hkrs, new_hkrs)
        
        old_guids = '''MicArray1CustomNameGUID = {6ae81ff4-203e-4fe1-88aa-f2d57775cd4a}
MicArray2CustomNameGUID = {3fe0e3e1-ad16-4772-8382-4129169018ce}
MicArray3CustomNameGUID = {c04bdb7c-2138-48da-9dd4-2af9ff2e58c2}
MicInCustomNameGUID = {d48deb08-fd1c-4d1e-b821-9064d49ae96e}'''
        new_guids = '''SpeakerCustomName1GUID = {c40e9164-a8ae-4363-80de-3a39827b7bb1}
SpeakerCustomName2GUID = {c40e9164-a8ae-4363-80de-3a39827b7bb2}
SpeakerCustomName3GUID = {c40e9164-a8ae-4363-80de-3a39827b7bb3}
SpeakerCustomName4GUID = {c40e9164-a8ae-4363-80de-3a39827b7bb4}
MicArray1CustomNameGUID = {6ae81ff4-203e-4fe1-88aa-f2d57775cd4a}
MicArray2CustomNameGUID = {3fe0e3e1-ad16-4772-8382-4129169018ce}
MicArray3CustomNameGUID = {c04bdb7c-2138-48da-9dd4-2af9ff2e58c2}
MicInCustomName1GUID = {d48deb08-fd1c-4d1e-b821-9064d49ae961}
MicInCustomName2GUID = {d48deb08-fd1c-4d1e-b821-9064d49ae962}
MicInCustomName3GUID = {d48deb08-fd1c-4d1e-b821-9064d49ae963}
MicInCustomName4GUID = {d48deb08-fd1c-4d1e-b821-9064d49ae964}'''
        content = content.replace(old_guids, new_guids)
        
        old_names = '''MicArray1CustomName= "Internal Microphone Array - Front"
MicArray2CustomName= "Internal Microphone Array - Rear"
MicArray3CustomName= "Internal Microphone Array - Front/Rear"
MicInCustomName= "ASMR MIC 01"'''
        new_names = '''SpeakerCustomName1 = "ASMR Audio 01"
SpeakerCustomName2 = "ASMR Audio 02"
SpeakerCustomName3 = "ASMR Audio 03"
SpeakerCustomName4 = "ASMR Audio 04"
MicArray1CustomName= "Internal Microphone Array - Front"
MicArray2CustomName= "Internal Microphone Array - Rear"
MicArray3CustomName= "Internal Microphone Array - Front/Rear"
MicInCustomName1= "ASMR MIC 01"
MicInCustomName2= "ASMR MIC 02"
MicInCustomName3= "ASMR MIC 03"
MicInCustomName4= "ASMR MIC 04"'''
        content = content.replace(old_names, new_names)
        
        with open(fpath, 'w', encoding='utf-8') as f:
            f.write(content)

replace_speakertoptable()
replace_micintoptable()
replace_minipairs()
replace_inx()
print('Done!')
