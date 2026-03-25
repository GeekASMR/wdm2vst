@echo off
mkdir "E:\Personal\Desktop\qudong" 2>nul
copy /Y "d:\Autigravity\wdm2vst\ASMRTOP_Driver_V3\x64\Release\package\VirtualAudioDriver.sys" "E:\Personal\Desktop\qudong\"
copy /Y "d:\Autigravity\wdm2vst\ASMRTOP_Driver_V3\x64\Release\package\VirtualAudioDriver.inf" "E:\Personal\Desktop\qudong\"
copy /Y "d:\Autigravity\wdm2vst\ASMRTOP_Driver_V3\x64\Release\package\asmrtopvirtualaudio.cat" "E:\Personal\Desktop\qudong\"
echo Done!
