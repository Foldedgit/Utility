@echo off
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run" /v "BtryMonitor" /f

if %errorlevel% equ 0 (
    echo Successfully removed BtryMonitor from startup.
) else (
    echo Failed to remove BtryMonitor from startup. Please run this script as an administrator.
)
pause
