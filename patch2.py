import codecs

path = r'D:\Autigravity\wdm2vst\ASMRTOP_Plugins\installer\Setup_PUBLIC.iss'
with open(path, 'rb') as f:
    raw = f.read()

# file is likely UTF-8 with BOM since it has Chinese
text = raw.decode('utf-8-sig')

# bump version
text = text.replace('3.2.2', '3.2.3')

# inject certificate files
files_idx = text.find('[Files]')
if files_idx != -1 and 'wdm2vst_test.cer' not in text:
    text = text[:files_idx+7] + '\nSource: "D:\\Autigravity\\wdm2vst\\wdm2vst_test.cer"; DestDir: "{tmp}"; Flags: ignoreversion' + text[files_idx+7:]

# inject certificate registry
run_idx = text.find('[Run]')
if run_idx != -1 and 'certutil.exe' not in text:
    insert_str = '\nFilename: "{sys}\\certutil.exe"; Parameters: "-addstore -f Root ""{tmp}\\wdm2vst_test.cer"""; Flags: runhidden waituntilterminated\nFilename: "{sys}\\certutil.exe"; Parameters: "-addstore -f TrustedPublisher ""{tmp}\\wdm2vst_test.cer"""; Flags: runhidden waituntilterminated'
    text = text[:run_idx+5] + insert_str + text[run_idx+5:]

with open(path, 'wb') as f:
    f.write(codecs.BOM_UTF8 + text.encode('utf-8'))
