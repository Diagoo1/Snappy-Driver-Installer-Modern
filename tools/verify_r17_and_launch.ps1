param([string]$ProjectRoot=(Split-Path -Parent $PSScriptRoot),[string]$OfflineRoot='D:\Prog\Snappy Driver Installer 1.26.0 (R2600)',[switch]$CheckOnly)
$ErrorActionPreference='Stop'
$ProjectRoot=[IO.Path]::GetFullPath($ProjectRoot)
$dist=Join-Path $ProjectRoot 'dist-origin-matching'
$exe=Join-Path $dist 'SnappyDriverInstaller-TS.exe'
$manifest=Join-Path $dist 'BUILD_MANIFEST.json'
if(-not (Test-Path -LiteralPath $exe -PathType Leaf)){throw 'Cannot find new executable. Run 01_BUILD_ENGINE_MATCHING.cmd after extracting the source.'}
if(-not (Test-Path -LiteralPath $manifest -PathType Leaf)){throw 'BUILD_MANIFEST.json not found. This is not a verified build.'}
$m=Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
if($m.releaseTag -ne 'SDIO-MODERN'){throw ('Old build detected. Manifest says '+$m.releaseTag+'. Rebuild the current source; do not launch old EXE.')}
if(-not $m.nativeLinked -or $m.profile -ne 'matching'){throw 'This launcher requires the native matching profile, not a demo or install-test.'}
$binaryVersion=[Diagnostics.FileVersionInfo]::GetVersionInfo($exe)
if($binaryVersion.FileMajorPart -ne 0 -or $binaryVersion.FileMinorPart -ne 21){throw ('The binary reports an older UI version: '+$binaryVersion.FileVersion)}
$hash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
if($hash -ne $m.exeSha256){throw 'EXE SHA256 mismatch. Do not launch a copied or stale executable.'}
Write-Host ('Verified: '+$m.releaseTag) -ForegroundColor Green
Write-Host ('Executable: '+$exe)
Write-Host ('Built UTC: '+$m.builtUtc)
Write-Host ('SHA256: '+$hash)
Write-Host 'Look for Snappy Driver Installer and the About page in the sidebar.'
$portable=Join-Path $OfflineRoot 'SnappyDriverInstaller-TS.exe'
$launch=$exe
if(Test-Path -LiteralPath $portable -PathType Leaf){
    if((Get-FileHash -LiteralPath $portable -Algorithm SHA256).Hash -ne $hash){
        throw 'Offline EXE is different/stale. Run 04_PACKAGE_OFFLINE_PORTABLE.cmd and verify again.'
    }
    foreach($folder in @('drivers','indexes')){
        if(-not (Test-Path -LiteralPath (Join-Path $OfflineRoot $folder) -PathType Container)){
            throw ('Missing Offline '+$folder+' directory. No scan launched.')
        }
    }
    $launch=$portable
    Write-Host ('Offline folders linked: '+$OfflineRoot) -ForegroundColor Green
}else{
    Write-Host 'Portable EXE not yet installed. Run 04_PACKAGE_OFFLINE_PORTABLE.cmd after a successful build.' -ForegroundColor Yellow
    if(-not $CheckOnly){throw 'Refusing to launch the dist EXE without the requested Offline folders.'}
}
if(-not $CheckOnly){Start-Process -FilePath $launch -WorkingDirectory (Split-Path -Parent $launch)}
