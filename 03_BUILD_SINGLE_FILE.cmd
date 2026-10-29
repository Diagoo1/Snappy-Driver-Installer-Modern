@echo off
setlocal
cd /d "%~dp0"
if not defined QT_QMAKE if exist "%~dp0vendor\qt-static\bin\qmake.exe" set "QT_QMAKE=%~dp0vendor\qt-static\bin\qmake.exe"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\integration\build_native_engine.ps1" -SingleFile -EnableExperimentalInstall -NoLaunch
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" echo Build stopped. See build-origin-*\build-native.log.
pause
exit /b %RESULT%
