@echo off
set ExePath=""C:\Users\imans\source\repos\Utility\x64\Debug\BtryMngr.exe""
reg add "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run" /v "BtryMonitor" /t REG_SZ /d %ExePath% /f

if %errorlevel% equ 0 (
    echo Successfully added BtryMonitor to startup.
) else (
    echo Failed to add BtryMonitor to startup. Please run this script as an administrator.
)
pause

