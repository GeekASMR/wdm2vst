import glob

for f in glob.glob('build/**/*_rc_lib.vcxproj', recursive=True):
    with open(f, 'r', encoding='utf-8') as file:
        c = file.read()
    c = c.replace('juce::juceaide', r'"D:\虚拟通道开发\ASMRTOP_Plugins\build\JUCE\extras\Build\juceaide\juceaide_artefacts\Release\juceaide.exe"')
    with open(f, 'w', encoding='utf-8') as file:
        file.write(c)
