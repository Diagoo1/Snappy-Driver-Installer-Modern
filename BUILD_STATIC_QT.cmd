@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build.ps1" -PrepareStaticQt %*
exit /b %ERRORLEVEL%
