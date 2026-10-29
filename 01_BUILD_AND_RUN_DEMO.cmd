@echo off
setlocal
chcp 65001 >nul
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build_win11_preview.ps1" -Mode demo
if errorlevel 1 (
  echo.
  echo Build failed. Please send a screenshot or the build log.
  pause
  exit /b 1
)
exit /b 0
