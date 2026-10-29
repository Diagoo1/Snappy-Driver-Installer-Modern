# Windows PowerShell 5.1. No administrator rights are needed to build.
Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
function Say([string]$Message) { Write-Host ('[SDIO native] ' + $Message) }
function Require-Exit([string]$Operation) {
    if ($LASTEXITCODE -ne 0) { throw "$Operation failed, exit code $LASTEXITCODE." }
}
function Write-Utf8([string]$Path,[string]$Text) {
    $parent=Split-Path -Parent $Path
    if($parent){New-Item -ItemType Directory -Force -Path $parent | Out-Null}
    [IO.File]::WriteAllText($Path,$Text,(New-Object Text.UTF8Encoding($false)))
}
function QPath([string]$Path) { return ('"' + ($Path -replace '\\','/') + '"') }
function Get-GitBlobHash([string]$Path) {
    [byte[]]$data=[IO.File]::ReadAllBytes($Path)
    [byte[]]$head=[Text.Encoding]::ASCII.GetBytes(('blob '+$data.LongLength+[char]0))
    $sha=[Security.Cryptography.SHA1]::Create()
    try {
        [void]$sha.TransformBlock($head,0,$head.Length,$head,0)
        [void]$sha.TransformFinalBlock($data,0,$data.Length)
        return ([BitConverter]::ToString($sha.Hash).Replace('-','').ToLowerInvariant())
    } finally { $sha.Dispose() }
}
function Fetch-Archive([string]$Url,[string]$Destination,[string]$Sha256='') {
    if(-not $Url.StartsWith('https://')){throw 'Only HTTPS source URLs are allowed.'}
    if(-not (Test-Path -LiteralPath $Destination -PathType Leaf)) {
        Say "Downloading source: $Url"
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        $client=New-Object Net.WebClient
        $client.Headers.Add('User-Agent','SDIO-Modern-Source-Builder/0.10')
        $partial=$Destination+'.partial'
        try {$client.DownloadFile($Url,$partial);Move-Item -LiteralPath $partial -Destination $Destination -Force}
        finally {$client.Dispose();if(Test-Path -LiteralPath $partial){Remove-Item -LiteralPath $partial -Force}}
    }
    if((Get-Item -LiteralPath $Destination).Length -gt 600MB){throw 'Source archive exceeds the build download size limit.'}
    if($Sha256) {
        $actual=(Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash.ToLowerInvariant()
        if($actual -ne $Sha256.ToLowerInvariant()) {throw "SHA256 mismatch for $Destination. Delete the bad cache file and retry."}
    }
}
function Expand-SafeZip([string]$Archive,[string]$Destination,[switch]$StripTop,[string]$Under='') {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    $root=[IO.Path]::GetFullPath($Destination).TrimEnd('\')+'\'
    $zip=[IO.Compression.ZipFile]::OpenRead($Archive)
    [long]$expanded=0
    try {
        foreach($entry in $zip.Entries) {
            $name=$entry.FullName.Replace('/','\')
            if($StripTop){
                $first=$name.IndexOf('\')
                if($first -lt 0){continue}
                $name=$name.Substring($first+1)
                if(-not $name){continue}
            }
            if($Under){
                $prefix=$Under.TrimEnd('\','/')+'\'
                if(-not $name.StartsWith($prefix,[StringComparison]::Ordinal)){continue}
                $name=$name.Substring($prefix.Length)
                if(-not $name){continue}
            }
            if($name -match '(^[\\/]|^[a-zA-Z]:|(^|[\\/])\.\.([\\/]|$))') {throw 'Unsafe path in downloaded ZIP.'}
            if((($entry.ExternalAttributes -shr 16) -band 0xF000) -eq 0xA000){throw 'Symbolic links are not accepted in source ZIP files.'}
            $target=[IO.Path]::GetFullPath((Join-Path $root $name))
            if(-not $target.StartsWith($root,[StringComparison]::OrdinalIgnoreCase)){throw 'ZIP entry escapes the destination.'}
            $expanded+=$entry.Length
            if($expanded -gt 3GB){throw 'Source ZIP exceeds expanded size limit.'}
            if(-not $entry.Name){New-Item -ItemType Directory -Force -Path $target | Out-Null;continue}
            $parent=Split-Path -Parent $target
            New-Item -ItemType Directory -Force -Path $parent | Out-Null
            [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target,$true)
        }
    } finally {$zip.Dispose()}
}
function Find-Qt {
    $candidates=@()
    if($env:QT_QMAKE){$candidates+=$env:QT_QMAKE}
    $candidates+=@('C:\Qt\Qt5.6.3\5.6.3\mingw49_32\bin\qmake.exe','C:\Qt\5.6.3\mingw49_32\bin\qmake.exe','D:\Qt\Qt5.6.3\5.6.3\mingw49_32\bin\qmake.exe')
    $q=$candidates | Where-Object {Test-Path -LiteralPath $_ -PathType Leaf} | Select-Object -First 1
    if(-not $q){$cmd=Get-Command qmake.exe -ErrorAction SilentlyContinue;if($cmd){$q=$cmd.Source}}
    if(-not $q){throw 'Qt 5.6.3 qmake.exe not found. Set QT_QMAKE to the full path to the installed MinGW qmake.exe.'}
    $version=((& $q -query QT_VERSION)|Out-String).Trim();Require-Exit 'Query Qt'
    $spec=((& $q -query QMAKE_XSPEC)|Out-String).Trim();Require-Exit 'Query Qt compiler'
    if($version -ne '5.6.3' -or $spec -notmatch 'win32-g\+\+') {throw "Expected Qt 5.6.3 MinGW; found $version / $spec."}
    return [IO.Path]::GetFullPath($q)
}
function Find-MinGW([string]$Qmake) {
    $candidates=@()
    if($env:QT_MINGW_BIN){$candidates+=$env:QT_MINGW_BIN}
    $candidates+=@('C:\Qt\Qt5.6.3\Tools\mingw492_32\bin','C:\Qt\Tools\mingw492_32\bin','D:\Qt\Qt5.6.3\Tools\mingw492_32\bin')
    $qRoot=Split-Path -Parent (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $Qmake)))
    $candidates+=(Join-Path $qRoot 'Tools\mingw492_32\bin')
    foreach($candidate in $candidates){if(Test-Path -LiteralPath (Join-Path $candidate 'g++.exe') -PathType Leaf){return [IO.Path]::GetFullPath($candidate)}}
    throw 'The matching MinGW compiler was not found. Set QT_MINGW_BIN to the compiler bin directory.'
}
function Assert-Compiler([string]$Bin) {
    $g=Join-Path $Bin 'g++.exe'
    $version=((& $g -dumpversion)|Out-String).Trim();Require-Exit 'Query GCC'
    $target=((& $g -dumpmachine)|Out-String).Trim();Require-Exit 'Query GCC architecture'
    if(-not $version.StartsWith('4.9.') -or $target -notmatch 'i[3-6]86.*mingw') {
        throw "Do not mix compiler ABIs. This build profile expects the Qt MinGW 4.9.x x86 compiler, found $version / $target."
    }
    return $version
}
function Build-Qmake([string]$Qmake,[string]$Make,[string]$Project,[string]$Build,[string[]]$Extra=@()) {
    New-Item -ItemType Directory -Force -Path $Build | Out-Null
    Push-Location $Build
    try {
        & $Qmake $Project 'CONFIG+=release' 'CONFIG-=debug' 'CONFIG-=debug_and_release' @Extra | Out-Host
        Require-Exit ('qmake '+$Project)
        & $Make '-j2' | Out-Host
        Require-Exit ('Compile '+$Project)
    } finally {Pop-Location}
}
