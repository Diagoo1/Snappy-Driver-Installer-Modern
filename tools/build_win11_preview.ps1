param(
    [ValidateSet('demo','real')][string]$Mode = 'demo',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Build = Join-Path $ProjectRoot 'build-win11-preview'
$Dist  = Join-Path $ProjectRoot 'dist-win11-preview'
$ExeName = 'SDIO-Modern-Qt.exe'
$QtDownload = 'https://download.qt.io/new_archive/qt/5.6/5.6.3/qt-opensource-windows-x86-mingw492-5.6.3.exe'

function Say([string]$Message) { Write-Host ('[SDIO Qt] ' + $Message) }
function Assert-Ok([string]$Step) {
    if ($LASTEXITCODE -ne 0) { throw ("$Step failed with exit code $LASTEXITCODE. See build-win11-preview\build-win11.log") }
}
function Find-QtQmake {
    if ($env:QT_QMAKE -and (Test-Path -LiteralPath $env:QT_QMAKE -PathType Leaf)) {
        return (Resolve-Path -LiteralPath $env:QT_QMAKE).Path
    }
    $locations = New-Object 'System.Collections.Generic.List[string]'
    $baseDirs = @("$env:SystemDrive\Qt", (Join-Path $env:USERPROFILE 'Qt'), 'D:\Qt')
    foreach ($base in $baseDirs) {
        if (-not (Test-Path -LiteralPath $base -PathType Container)) { continue }
        foreach ($suffix in @(
            '5.6\mingw49_32\bin\qmake.exe',
            '5.6.3\mingw49_32\bin\qmake.exe',
            'Qt5.6.3\5.6\mingw49_32\bin\qmake.exe',
            'Qt5.6.3\5.6.3\mingw49_32\bin\qmake.exe'
        )) {
            $test = Join-Path $base $suffix
            if (Test-Path -LiteralPath $test -PathType Leaf) { $locations.Add($test) }
        }
        if ($locations.Count -eq 0) {
            $found = Get-ChildItem -LiteralPath $base -Filter 'qmake.exe' -File -Recurse -ErrorAction SilentlyContinue |
                Where-Object { $_.FullName -match '[\\/]mingw49_32[\\/]bin[\\/]qmake\.exe$' } |
                Select-Object -First 1
            if ($found) { $locations.Add($found.FullName) }
        }
    }
    if ($locations.Count -gt 0) { return $locations[0] }
    $fallback = Get-Command 'qmake.exe' -ErrorAction SilentlyContinue
    if ($fallback) { return $fallback.Source }
    return $null
}
function Find-MinGW {
    if ($env:QT_MINGW_BIN -and (Test-Path (Join-Path $env:QT_MINGW_BIN 'mingw32-make.exe'))) {
        return (Resolve-Path -LiteralPath $env:QT_MINGW_BIN).Path
    }
    $qtBin = Split-Path -Parent $script:Qmake
    $qtHome = (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $qtBin)))
    $bases = @("$env:SystemDrive\Qt", (Join-Path $env:USERPROFILE 'Qt'), $qtHome, 'D:\Qt')
    foreach ($base in $bases) {
        if (-not (Test-Path -LiteralPath $base -PathType Container)) { continue }
        foreach ($suffix in @('Tools\mingw492_32\bin', 'Qt5.6.3\Tools\mingw492_32\bin',
                              'Tools\mingw49_32\bin', 'Qt5.6.3\Tools\mingw49_32\bin')) {
            $path = Join-Path $base $suffix
            if (Test-Path -LiteralPath (Join-Path $path 'mingw32-make.exe') -PathType Leaf) { return $path }
        }
        $found = Get-ChildItem -LiteralPath $base -Filter 'mingw32-make.exe' -File -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match 'mingw(492|49)_32[\\/]bin[\\/]mingw32-make\.exe$' } |
            Select-Object -First 1
        if ($found) { return $found.DirectoryName }
    }
    return $null
}

New-Item -ItemType Directory -Path $Build -Force | Out-Null
$Log = Join-Path $Build 'build-win11.log'
$transcriptStarted = $false
try {
    Start-Transcript -Path $Log -Append -ErrorAction Stop | Out-Null
    $transcriptStarted = $true
} catch { Say 'Could not enable transcript; console errors will still be shown.' }

try {
    if ($SkipBuild) {
        $exe = Join-Path $Dist $ExeName
        if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
            throw 'No built EXE found. First run 01_BUILD_AND_RUN_DEMO.cmd.'
        }
    } else {
        Say 'Looking for Qt 5.6.3 (MinGW 4.9.2 32-bit)...'
        $script:Qmake = Find-QtQmake
        if (-not $Qmake) { throw ('Qt qmake.exe not found. Install the OFFICIAL Qt package: ' + $QtDownload) }
        $QtVersion = ((& $Qmake -query QT_VERSION) | Out-String).Trim()
        Assert-Ok 'Read Qt version'
        $QtSpec = ((& $Qmake -query QMAKE_XSPEC) | Out-String).Trim()
        Assert-Ok 'Read Qt toolchain'
        Say ("Qt: $QtVersion | spec: $QtSpec | qmake: $Qmake")
        if ($QtVersion -ne '5.6.3') {
            throw "Wrong Qt version: $QtVersion. This project preview expects Qt 5.6.3. Set QT_QMAKE to its qmake.exe."
        }
        if ($QtSpec -notmatch 'win32-g\+\+') {
            throw "Wrong Qt compiler '$QtSpec'. Install the Qt 5.6.3 MinGW 4.9.2 32-bit package; do not mix it with MSVC Qt libraries."
        }
        $MinGwBin = Find-MinGW
        if (-not $MinGwBin) { throw 'MinGW 4.9.2 not found. Install the bundled MinGW compiler, or set QT_MINGW_BIN to its bin folder.' }
        $Make = Join-Path $MinGwBin 'mingw32-make.exe'
        $Compiler = Join-Path $MinGwBin 'g++.exe'
        if (-not (Test-Path -LiteralPath $Compiler -PathType Leaf)) { throw "g++.exe missing in $MinGwBin" }
        $GccVersion = ((& $Compiler -dumpversion) | Out-String).Trim()
        Say "Compiler GCC: $GccVersion | $MinGwBin"
        if (-not $GccVersion.StartsWith('4.9')) {
            throw "Expected MinGW GCC 4.9.2, but found $GccVersion. QT_MINGW_BIN must point to Qt's bundled compiler."
        }
        $QtBin = Split-Path -Parent $Qmake
        $env:PATH = "$MinGwBin;$QtBin;" + $env:PATH
        Push-Location $Build
        try {
            Say 'Generating Makefile with qmake...'
            & $Qmake (Join-Path $ProjectRoot 'SDIO_Modern_Qt.pro') 'CONFIG+=release' 'CONFIG-=debug'
            Assert-Ok 'qmake'
            Say 'Compiling with mingw32-make (Release)...'
            & $Make '-j4'
            Assert-Ok 'mingw32-make'
        } finally { Pop-Location }
        $possible = @((Join-Path $Build ('release\' + $ExeName)), (Join-Path $Build $ExeName))
        $built = $possible | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
        if (-not $built) { throw 'Compilation ended but EXE was not found in build-win11-preview.' }
        New-Item -ItemType Directory -Path $Dist -Force | Out-Null
        $exe = Join-Path $Dist $ExeName
        Copy-Item -LiteralPath $built -Destination $exe -Force
        Say 'Copying Qt DLLs and plugins for a portable PREVIEW folder...'
        $deploy = Join-Path $QtBin 'windeployqt.exe'
        $deployed = $false
        if (Test-Path -LiteralPath $deploy -PathType Leaf) {
            & $deploy '--release' $exe
            if ($LASTEXITCODE -eq 0) { $deployed = $true }
            else { Say 'windeployqt did not finish successfully. Trying manual Qt DLL copy.' }
        }
        if (-not $deployed) {
            foreach ($dll in @('Qt5Core.dll','Qt5Gui.dll','Qt5Widgets.dll','Qt5Svg.dll', 'libEGL.dll', 'libGLESv2.dll', 'icudt54.dll', 'icuin54.dll', 'icuuc54.dll', 'icudt56.dll', 'icuin56.dll', 'icuuc56.dll')) {
                $source = Join-Path $QtBin $dll
                if (Test-Path -LiteralPath $source -PathType Leaf) { Copy-Item -LiteralPath $source -Destination $Dist -Force }
            }
            $qtPlugins = Join-Path (Split-Path -Parent $QtBin) 'plugins\platforms\qwindows.dll'
            if (Test-Path -LiteralPath $qtPlugins -PathType Leaf) {
                $platDir = Join-Path $Dist 'platforms'
                New-Item -ItemType Directory -Path $platDir -Force | Out-Null
                Copy-Item -LiteralPath $qtPlugins -Destination $platDir -Force
            }
        }
        foreach ($dll in @('libgcc_s_dw2-1.dll','libgcc_s_sjlj-1.dll','libstdc++-6.dll','libwinpthread-1.dll')) {
            $source = Join-Path $MinGwBin $dll
            if (Test-Path -LiteralPath $source -PathType Leaf) { Copy-Item -LiteralPath $source -Destination $Dist -Force }
        }
        Set-Content -LiteralPath (Join-Path $Dist 'qt.conf') -Value @('[Paths]', 'Plugins = .') -Encoding Ascii
        Copy-Item -LiteralPath (Join-Path $ProjectRoot 'LICENSE-GPL-3.0.txt') -Destination (Join-Path $Dist 'LICENSE-GPL-3.0.txt') -Force
        if (-not (Test-Path -LiteralPath (Join-Path $Dist 'platforms\qwindows.dll'))) {
            Say 'WARNING: qwindows.dll missing. If the EXE reports a Qt platform plugin error, send the build log.'
        }
    }
    Say ("Built EXE: $exe")
    Say 'UI-only developer build. Native matching and installation are NOT linked. Use 01_BUILD_ENGINE_MATCHING.cmd for the integrated build.'
    Say ("Launching in $Mode mode...")
    if ($Mode -eq 'demo') {
        Start-Process -FilePath $exe -ArgumentList '--demo' -WorkingDirectory $Dist
    } else {
        # Do not pass an empty ArgumentList; Windows PowerShell 5.1 may reject it.
        Start-Process -FilePath $exe -WorkingDirectory $Dist
    }
    Say 'Application started. Open dist-win11-preview to launch it again.'
    exit 0
} catch {
    Write-Host ''
    Write-Host ('ERROR: ' + $_.Exception.Message) -ForegroundColor Red
    Say ("Send me the screen text or this log: $Log")
    exit 1
} finally {
    if ($transcriptStarted) { try { Stop-Transcript | Out-Null } catch {} }
}
