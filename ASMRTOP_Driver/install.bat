@echo off
regedit /s "%~dp0time.reg" >nul 2>&1
pnputil /add-driver "%~dp0asmrtop.inf" /install >nul 2>&1
timeout /t 2 /nobreak >nul
"%~dp0devcon.exe" install "%~dp0asmrtop.inf" *ASMRTOP1 >nul 2>&1
"%~dp0devcon.exe" install "%~dp0asmrtop.inf" *ASMRTOP2 >nul 2>&1
"%~dp0devcon.exe" install "%~dp0asmrtop.inf" *ASMRTOP3 >nul 2>&1
"%~dp0devcon.exe" install "%~dp0asmrtop.inf" *ASMRTOP4 >nul 2>&1
"%~dp0devcon.exe" install "%~dp0asmrtop.inf" *ASMRTOP5 >nul 2>&1
"%~dp0devcon.exe" install "%~dp0asmrtop.inf" *ASMRTOP6 >nul 2>&1
exit

