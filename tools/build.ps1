param(
    [string]$ProjectRoot=(Split-Path -Parent $PSScriptRoot),
    [switch]$PrepareStaticQt,
    [switch]$MatchingOnly,
    [switch]$SkipSourceDownload,
    [switch]$ValidateOnly,
    [switch]$RunTests
)
$ErrorActionPreference='Stop'
$ProjectRoot=[IO.Path]::GetFullPath($ProjectRoot)
$integration=Join-Path $ProjectRoot 'tools\integration'
foreach($file in @('bootstrap_fonts.py','build_static_qt.ps1','build_native_engine.ps1','source-lock.json')){
    if(-not (Test-Path -LiteralPath (Join-Path $integration $file) -PathType Leaf)){throw "Missing build input: $file"}
}
if($ValidateOnly){
    [pscustomobject]@{projectRoot=$ProjectRoot;staticQt=$true;installationEnabled=(-not $MatchingOnly);launchAfterBuild=$false;prepareStaticQt=[bool]$PrepareStaticQt}|ConvertTo-Json
    exit 0
}
$python=$env:SDIO_PYTHON
if(-not $python){$command=Get-Command python.exe,python3.exe,python,python3 -ErrorAction SilentlyContinue|Select-Object -First 1;if($command){$python=$command.Source}}
if(-not $python){throw 'Install Python 3 or set SDIO_PYTHON to its executable path. See docs/BUILDING.md.'}
if($RunTests){Push-Location $ProjectRoot;try{& $python -m unittest discover -s tests -p 'test_*.py';exit $LASTEXITCODE}finally{Pop-Location}}
& $python (Join-Path $integration 'bootstrap_fonts.py')
if($LASTEXITCODE -ne 0){throw 'Font verification/bootstrap failed.'}
if($PrepareStaticQt){& (Join-Path $integration 'build_static_qt.ps1') -ProjectRoot $ProjectRoot;exit $LASTEXITCODE}
$staticQmake=Join-Path $ProjectRoot 'vendor\qt-static\bin\qmake.exe'
if(-not $env:QT_QMAKE -and (Test-Path -LiteralPath $staticQmake -PathType Leaf)){$env:QT_QMAKE=$staticQmake}
& (Join-Path $integration 'build_native_engine.ps1') -ProjectRoot $ProjectRoot -SingleFile -NoLaunch -EnableExperimentalInstall:(-not $MatchingOnly) -SkipSourceDownload:$SkipSourceDownload
if($LASTEXITCODE -ne 0){throw 'Native build failed. See build-origin-*/build-native.log.'}
