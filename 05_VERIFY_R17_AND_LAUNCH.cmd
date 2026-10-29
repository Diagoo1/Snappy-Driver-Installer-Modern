@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0toolserify_r17_and_launch.ps1"
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" echo Release 17 verification failed. Do not use an old EXE.
pause
exit /b %RESULT%
