@echo off
setlocal
set "BUILD_MODE="
if "%~1"=="" goto menu
if /i "%~1"=="-Help" goto help
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build.ps1" %*
exit /b %ERRORLEVEL%
:menu
echo Snappy Driver Installer Modern - Build
echo 1. Build single EXE with installation support
echo 2. Prepare patched static Qt SDK
echo 3. Build matching-only EXE
echo 4. Validate build configuration
echo 5. Run source regression checks
choice /c 12345 /n /m "Choose [1-5]: "
if errorlevel 5 goto tests
if errorlevel 4 goto validate
if errorlevel 3 goto matching
if errorlevel 2 goto sdk
:execute
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build.ps1" %BUILD_MODE%
exit /b %ERRORLEVEL%
:validate
set "BUILD_MODE=-ValidateOnly"
goto execute
:matching
set "BUILD_MODE=-MatchingOnly"
goto execute
:sdk
set "BUILD_MODE=-PrepareStaticQt"
goto execute
:tests
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build.ps1" -RunTests
exit /b %ERRORLEVEL%
:help
echo BUILD.cmd -PrepareStaticQt
echo BUILD.cmd -MatchingOnly [-SkipSourceDownload]
echo BUILD.cmd -SkipSourceDownload
echo BUILD.cmd -ValidateOnly
echo BUILD.cmd -RunTests
exit /b 0
