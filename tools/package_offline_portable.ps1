param(
    [string]$ProjectRoot=(Split-Path -Parent $PSScriptRoot),
    [string]$OfflineRoot='D:\Prog\Snappy Driver Installer 1.26.0 (R2600)'
)
$ErrorActionPreference='Stop'
$ProjectRoot=[IO.Path]::GetFullPath($ProjectRoot)
$OfflineRoot=[IO.Path]::GetFullPath($OfflineRoot)
foreach($name in @('drivers','indexes')){
    if(-not (Test-Path -LiteralPath (Join-Path $OfflineRoot $name) -PathType Container)) {
        throw "Missing Offline folder: $name ($OfflineRoot). No files copied."
    }
}
$buildDir=Join-Path $ProjectRoot 'dist-origin-matching'
$exe=Join-Path $buildDir 'SnappyDriverInstaller-TS.exe'
$manifest=Join-Path $buildDir 'BUILD_MANIFEST.json'
if(-not (Test-Path -LiteralPath $exe -PathType Leaf) -or -not (Test-Path -LiteralPath $manifest -PathType Leaf)){
    throw 'Build first with 01_BUILD_ENGINE_MATCHING.cmd. This script cannot manufacture an executable.'
}
$m=Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
if($m.releaseTag -ne 'SDIO-MODERN'){throw 'Old build detected. Rebuild with the Release 17 source before packaging.'}
if(-not $m.nativeLinked -or $m.profile -ne 'matching'){
    throw 'This packaging flow accepts only a real linked matching-only build, never demo or install-test.'
}
$hash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
if($hash -ne $m.exeSha256){throw 'EXE changed since native build manifest. Rebuild first.'}
# Different filename from original SDI_R2600.exe / SDI_x64_R2600.exe.
$dest=Join-Path $OfflineRoot 'SnappyDriverInstaller-TS.exe'
Copy-Item -LiteralPath $exe -Destination $dest -Force
# Preserve the exact working Qt/MinGW DLLs and plugins from the build output.
Get-ChildItem -LiteralPath $buildDir -File -Filter '*.dll' | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $OfflineRoot $_.Name) -Force
}
foreach($pluginDir in @('platforms','imageformats','iconengines','styles','bearer')) {
    $sourcePlugin=Join-Path $buildDir $pluginDir
    if(Test-Path -LiteralPath $sourcePlugin -PathType Container){
        Copy-Item -LiteralPath $sourcePlugin -Destination $OfflineRoot -Recurse -Force
    }
}
# Qt may be dynamic: use windeployqt in the same SDK when available; static Qt
# builds need no Qt DLL deployment. A missing runtime dependency is a hard error.
$qtBin='C:\Qt\Qt5.6.3\5.6.3\mingw49_32\bin'
$qtDeploy=Join-Path $qtBin 'windeployqt.exe'
$mingw='C:\Qt\Qt5.6.3\Tools\mingw492_32\bin'
$objdump=Join-Path $mingw 'objdump.exe'
$imports=@()
if(Test-Path -LiteralPath $objdump -PathType Leaf){
    $imports=@(& $objdump -p $dest 2>$null | Select-String -Pattern '^\s*DLL Name:\s*(.+)$' | ForEach-Object {$_.Matches[0].Groups[1].Value})
}
if($imports | Where-Object {$_ -match '^Qt5.*\.dll$'}){
    if(-not (Test-Path -LiteralPath $qtDeploy -PathType Leaf)){
        throw 'Dynamic Qt runtime detected, but windeployqt.exe is missing. EXE may not be portable yet.'
    }
    & $qtDeploy --release $dest
    if($LASTEXITCODE -ne 0){throw 'windeployqt failed. Review runtime before testing.'}
}
$sourceLang=Join-Path $buildDir 'langs'
if(Test-Path -LiteralPath $sourceLang -PathType Container){
    $destLang=Join-Path $OfflineRoot 'langs'
    New-Item -ItemType Directory -Force -Path $destLang | Out-Null
    Copy-Item -Path (Join-Path $sourceLang '*.txt') -Destination $destLang -Force
}
$old=Join-Path $OfflineRoot 'SnappyDriverInstaller-TS.ini'
# The application migrates existing preferences to HKCU; never create a sidecar.
if(Test-Path -LiteralPath $old -PathType Leaf){
    Write-Host 'Existing language/theme preferences preserved for migration on next launch.'
}
Copy-Item -LiteralPath (Join-Path $ProjectRoot 'LICENSE-GPL-3.0.txt') -Destination (Join-Path $OfflineRoot 'SnappyDriverInstaller-TS-GPL-3.0.txt') -Force
$sourceLicenses=Join-Path $buildDir 'licenses'
if(Test-Path -LiteralPath $sourceLicenses -PathType Container){
    $destLicenses=Join-Path $OfflineRoot 'SnappyDriverInstaller-TS-Licenses'
    New-Item -ItemType Directory -Force -Path $destLicenses | Out-Null
    Get-ChildItem -LiteralPath $sourceLicenses | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $destLicenses -Recurse -Force
    }
}
Copy-Item -LiteralPath $manifest -Destination (Join-Path $OfflineRoot 'SnappyDriverInstaller-TS-BUILD_MANIFEST.json') -Force
Write-Host 'Portable MATCHING build copied. No SDIO indexes, packs, or original executables were moved or duplicated.'
Write-Host ('Executable: '+$dest)
Write-Host 'Test scan only; installation is disabled in this build.'
