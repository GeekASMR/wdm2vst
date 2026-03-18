@echo off
"%~dp0devcon.exe" remove *ASMRTOP* >nul 2>&1
for /f "tokens=1" %%i in ('pnputil /enum-drivers 2^>nul ^| findstr /C:"asmrtop.inf"') do pnputil /delete-driver %%i /uninstall /force >nul 2>&1
exit

