param([string]$ProjectRoot=(Split-Path -Parent (Split-Path -Parent $PSScriptRoot)))
. (Join-Path $PSScriptRoot 'common.ps1')
$ProjectRoot=[IO.Path]::GetFullPath($ProjectRoot)
$mingw=Find-MinGW (Find-Qt)
$null=Assert-Compiler $mingw
$env:PATH=$mingw+';'+$env:PATH
$make=Join-Path $mingw 'mingw32-make.exe'
$prefix=Join-Path $ProjectRoot 'vendor\qt-static'
$modules=@(
    @{name='qtbase';hash='fef48529a6fc2617a30d75d952cb327c6be341fd104154993922184b3b3b4da1'},
    @{name='qtsvg';hash='100f183517b46554079beabd8d2cabe3070a74dd0a2e64b6a304eac71cfadcec'}
)
foreach($module in $modules){
    $name=$module.name+'-opensource-src-5.6.3'
    $archive=Join-Path $ProjectRoot ('.source-cache\'+$name+'.tar.xz')
    Fetch-Archive ('https://download.qt.io/archive/qt/5.6/5.6.3/submodules/'+$name+'.tar.xz') $archive $module.hash
    $source=Join-Path $ProjectRoot ('vendor\'+$name)
    if(-not (Test-Path -LiteralPath $source)){
        & tar.exe '-xf' $archive '-C' (Join-Path $ProjectRoot 'vendor');Require-Exit 'Extract official Qt source'
    }
    if($module.name -eq 'qtbase'){
        & (Join-Path $PSScriptRoot 'patch_qt_fractional_dpi.ps1') -QtSource $source
    }
    Push-Location $source
    try {
        if($module.name -eq 'qtbase'){
            if(-not (Test-Path -LiteralPath 'Makefile')){
                & .\configure.bat '-static' '-release' '-opensource' '-confirm-license' '-platform' 'win32-g++' '-prefix' ($prefix -replace '\\','/') '-no-opengl' '-no-icu' '-no-openssl' '-no-dbus' '-no-audio-backend' '-no-wmf-backend' '-no-directwrite' '-no-angle' '-nomake' 'examples' '-nomake' 'tests' '-qt-zlib' '-qt-libpng' '-qt-libjpeg' '-no-sse2'
                Require-Exit 'Configure static Qt for XP'
            }
        } else {
            & (Join-Path $prefix 'bin\qmake.exe') 'qtsvg.pro';Require-Exit 'Configure static QtSvg'
        }
        & $make '-j4';Require-Exit 'Build static Qt module'
        & $make 'install';Require-Exit 'Install static Qt module'
    } finally {Pop-Location}
}
$dpiReceipt=Get-Content -LiteralPath (Join-Path $ProjectRoot 'vendor\qtbase-opensource-src-5.6.3\SDIO_FRACTIONAL_DPI_PATCH.json') -Raw | ConvertFrom-Json
$dpiReceipt | Add-Member -NotePropertyName widgetsLibrarySha256 -NotePropertyValue (Get-FileHash -LiteralPath (Join-Path $prefix 'lib\libQt5Widgets.a') -Algorithm SHA256).Hash.ToLowerInvariant() -Force
[IO.File]::WriteAllText((Join-Path $prefix 'FRACTIONAL_DPI_PATCH.json'),($dpiReceipt|ConvertTo-Json -Depth 6),(New-Object Text.UTF8Encoding($false)))
Say ('Static SDK ready: '+$prefix)
