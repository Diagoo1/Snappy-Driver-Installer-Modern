@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\package_offline_portable.ps1" -OfflineRoot "D:\Prog\Snappy Driver Installer 1.26.0 (R2600)"
if errorlevel 1 (echo PORTABLE PACKAGE FAILED. See errors above.) else (echo Portable files copied next to original offline folders.)
pause
