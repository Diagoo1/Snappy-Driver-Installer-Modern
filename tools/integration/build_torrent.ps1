param(
    [string]$ProjectRoot=(Split-Path -Parent (Split-Path -Parent $PSScriptRoot)),
    [string]$Qmake='',
    [string]$MinGW=''
)
. (Join-Path $PSScriptRoot 'common.ps1')
$ProjectRoot=[IO.Path]::GetFullPath($ProjectRoot)
if(-not $Qmake){$Qmake=Find-Qt}
if(-not $MinGW){$MinGW=Find-MinGW $Qmake}
$env:PATH=$MinGW+';'+(Split-Path -Parent $Qmake)+';'+$env:PATH
$review=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'torrent-patches.json') -Raw | ConvertFrom-Json
$archive=Join-Path $ProjectRoot '.source-cache\libtorrent-1.2.20.zip'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $archive) | Out-Null
Fetch-Archive 'https://codeload.github.com/arvidn/libtorrent/zip/refs/tags/v1.2.20' $archive $review.archiveSha256
$original=Join-Path $ProjectRoot 'vendor\libtorrent-1.2.20'
$patched=Join-Path $ProjectRoot 'vendor\libtorrent-patched'
if(-not (Test-Path -LiteralPath (Join-Path $original 'src\session.cpp'))){Expand-SafeZip $archive $original -StripTop}
# Reconstruct the reviewed source bytes; do not touch unchanged file timestamps.
foreach($folder in @('src','include','ed25519\src')){
    foreach($file in Get-ChildItem -LiteralPath (Join-Path $original $folder) -File -Recurse){
        $relative=$file.FullName.Substring($original.TrimEnd('\').Length+1)
        $destination=Join-Path $patched $relative
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
        $changes=@($review.changes | Where-Object {$_.file.Replace('/','\') -eq $relative})
        if($changes.Count){
            $expected=[IO.File]::ReadAllText($file.FullName).Replace("`r`n","`n")
            foreach($change in $changes){
                $count=([regex]::Matches($expected,[regex]::Escape([string]$change.old))).Count
                if($count -ne [int]$change.count){throw "Pinned torrent compatibility context changed: $relative"}
                $expected=$expected.Replace([string]$change.old,[string]$change.new)
            }
            if(-not (Test-Path -LiteralPath $destination) -or [IO.File]::ReadAllText($destination) -cne $expected){Write-Utf8 $destination $expected}
        }elseif(-not (Test-Path -LiteralPath $destination) -or (Get-FileHash -LiteralPath $destination).Hash -ne (Get-FileHash -LiteralPath $file.FullName).Hash){Copy-Item -LiteralPath $file.FullName -Destination $destination -Force}
    }
}
$work=Join-Path $ProjectRoot 'build-torrent-static'
New-Item -ItemType Directory -Force -Path $work | Out-Null
$definitions='WIN32 _WIN32_WINNT=0x0501 WINVER=0x0501 BOOST_USE_WINAPI_VERSION=0x0501 BOOST_USE_NTDDI_VERSION=0x05010200 NTDDI_VERSION=0x05010200 TORRENT_USE_ICONV=0 TORRENT_USE_I2P=0 TORRENT_DISABLE_LOGGING TORRENT_DISABLE_ENCRYPTION TORRENT_NO_DEPRECATE TORRENT_DISABLE_MUTABLE_TORRENTS BOOST_ALL_NO_LIB BOOST_ERROR_CODE_HEADER_ONLY BOOST_ASIO_ENABLE_CANCELIO BOOST_ASIO_HAS_STD_CHRONO BOOST_EXCEPTION_DISABLE _FILE_OFFSET_BITS=64'
$sources=@()
foreach($folder in @('src','src\kademlia','ed25519\src')){$sources+=@(Get-ChildItem -LiteralPath (Join-Path $patched $folder) -Filter '*.cpp' -File | ForEach-Object {$_.FullName})}
$units=($sources | ForEach-Object {'SOURCES += '+(QPath $_)}) -join "`n"
$project=@"
TEMPLATE = lib
TARGET = sdio_torrent
CONFIG += staticlib release c++11
CONFIG -= qt debug debug_and_release
QMAKE_CXXFLAGS_RELEASE = -O2 -w -ffunction-sections -fdata-sections
DEFINES += $definitions TORRENT_BUILDING_LIBRARY
INCLUDEPATH += $(QPath (Join-Path $patched 'include')) $(QPath (Join-Path $ProjectRoot 'vendor\boost'))
$units
"@
$pro=Join-Path $work 'torrent.pro'
if(-not (Test-Path -LiteralPath $pro) -or [IO.File]::ReadAllText($pro) -cne $project){Write-Utf8 $pro $project}
Push-Location $work
try{
    & $Qmake $pro '-o' 'Makefile';Require-Exit 'Configure static torrent library'
    & (Join-Path $MinGW 'mingw32-make.exe') '-j4';Require-Exit 'Build static torrent library'
}finally{Pop-Location}
$library=Join-Path $work 'libsdio_torrent.a'
if(-not (Test-Path -LiteralPath $library -PathType Leaf)){throw 'Static torrent library was not produced.'}
$pri=@"
DEFINES += SDIO_TORRENT_LINKED $definitions
INCLUDEPATH += $(QPath (Join-Path $patched 'include')) $(QPath (Join-Path $ProjectRoot 'vendor\boost'))
LIBS += $(QPath $library) -liphlpapi -lws2_32 -lwsock32 -ladvapi32 -lcrypt32
PRE_TARGETDEPS += $(QPath $library)
"@
Write-Utf8 (Join-Path $work 'torrent-link.pri') $pri
Write-Utf8 (Join-Path $work 'SOURCE_RECEIPT.json') (@{version='1.2.20';archiveSha256=$review.archiveSha256;patchesApplied=$review.changes.Count;static=$true;xpRuntimeTested=$false;patchManifestSha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'torrent-patches.json')).Hash}|ConvertTo-Json)
Say 'Static peer transport built; compatibility still requires Windows runtime testing.'
