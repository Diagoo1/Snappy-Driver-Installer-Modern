@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\integration\build_native_engine.ps1" -NoLaunch
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" echo Build stopped. See build-origin-*\build-native.log.
pause
exit /b %RESULT%
