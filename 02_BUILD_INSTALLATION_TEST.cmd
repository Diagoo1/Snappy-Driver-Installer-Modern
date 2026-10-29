@echo off
setlocal
cd /d "%~dp0"
echo This enables UNVALIDATED native driver installation for controlled test machines.
echo Do not test on your only working computer. No installation runs during this build.
choice /C YN /N /M "Build experimental installer? [Y/N] "
if errorlevel 2 exit /b 0
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\integration\build_native_engine.ps1" -EnableExperimentalInstall -NoLaunch
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" echo Build stopped. See build-origin-*\build-native.log.
pause
exit /b %RESULT%
