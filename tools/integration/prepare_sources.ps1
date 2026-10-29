param([string]$ProjectRoot=(Split-Path -Parent (Split-Path -Parent $PSScriptRoot)))
. (Join-Path $PSScriptRoot 'common.ps1')
$ProjectRoot=[IO.Path]::GetFullPath($ProjectRoot)
$vendor=Join-Path $ProjectRoot 'vendor'
$cache=Join-Path $ProjectRoot '.source-cache'
New-Item -ItemType Directory -Force -Path $vendor,$cache | Out-Null
$lock=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'source-lock.json') -Raw | ConvertFrom-Json
$origin=Join-Path $vendor 'origin-upstream'
$patched=Join-Path $vendor 'origin-patched'
$boost=Join-Path $vendor 'boost'
$webp=Join-Path $vendor 'webp'
if(-not (Test-Path -LiteralPath $origin -PathType Container)) {
    $archive=Join-Path $cache ('origin-'+$lock.origin.commit+'.zip')
    Fetch-Archive $lock.origin.url $archive
    $unpack=Join-Path $cache ('uo-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
    try {
        Expand-SafeZip $archive $unpack -StripTop -Under $lock.origin.subdirectory
        if(-not (Test-Path -LiteralPath (Join-Path $unpack 'source\matcher.cpp'))){throw 'C++ Origin baseline missing from source ZIP.'}
        Move-Item -LiteralPath $unpack -Destination $origin
    } finally {if(Test-Path -LiteralPath $unpack){Remove-Item -LiteralPath $unpack -Recurse -Force}}
}
foreach($entry in $lock.origin.gitBlobs.PSObject.Properties) {
    $path=Join-Path $origin $entry.Name
    if(-not (Test-Path -LiteralPath $path -PathType Leaf)){throw "Missing source file: $($entry.Name)"}
    if((Get-GitBlobHash $path) -ne $entry.Value){throw "Source differs from the reviewed baseline: $($entry.Name). No patches applied."}
}
Say 'Reviewed Origin source fingerprints match the pinned baseline.'
if(-not (Test-Path -LiteralPath (Join-Path $boost 'boost\version.hpp'))) {
    $archive=Join-Path $cache 'boost_1_79_0.zip'
    Fetch-Archive $lock.boost.url $archive $lock.boost.sha256
    $unpack=Join-Path $cache ('ub-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
    try {Expand-SafeZip $archive $unpack -StripTop;Move-Item -LiteralPath $unpack -Destination $boost}
    finally {if(Test-Path -LiteralPath $unpack){Remove-Item -LiteralPath $unpack -Recurse -Force}}
}
if(-not (Test-Path -LiteralPath (Join-Path $webp 'src\webp\decode.h'))) {
    $archive=Join-Path $cache ('webp-'+$lock.webp.commit+'.zip')
    Fetch-Archive $lock.webp.url $archive
    $unpack=Join-Path $cache ('uw-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
    try {
        Expand-SafeZip $archive $unpack -StripTop
        if(-not (Test-Path -LiteralPath (Join-Path $unpack 'src\webp\decode.h'))){throw 'Unexpected WebP source layout.'}
        Move-Item -LiteralPath $unpack -Destination $webp
    } finally {if(Test-Path -LiteralPath $unpack){Remove-Item -LiteralPath $unpack -Recurse -Force}}
}
# A generated copy, never patch the reviewed pristine source or user-installed SDIO.
if(Test-Path -LiteralPath $patched){Remove-Item -LiteralPath $patched -Recurse -Force}
Copy-Item -LiteralPath $origin -Destination $patched -Recurse
# Repair the incomplete SevenZ vendor snapshot without altering pinned/pristine Origin.
# The mirror bundles 7-Zip 26.01 source but omits CPP/Common/Common0.h,
# which is required by CPP/Common/Common.h. It is NOT an include-path issue.
$sevenFix=$lock.sevenZipBootstrap
$sevenVersion=Join-Path $origin 'external\SevenZ\build\C\7zVersion.h'
if(-not (Test-Path -LiteralPath $sevenVersion -PathType Leaf) -or
   (Get-Content -LiteralPath $sevenVersion -Raw) -notmatch '#define\s+MY_VERSION_NUMBERS\s+"26\.01"') {
    throw 'The bundled SevenZ is not the reviewed 26.01 version. No external header was added.'
}
$sevenRel=([string]$sevenFix.relativePath).Replace('/','\')
$sevenTarget=Join-Path $patched $sevenRel
$sevenCache=Join-Path $cache 'sevenzip-26.01-Common0.h'
if(-not (Test-Path -LiteralPath $sevenTarget -PathType Leaf)) {
    # This is an upstream SOURCE HEADER (not an executable); fetched from a pinned commit.
    Fetch-Archive ([string]$sevenFix.url) $sevenCache
    if((Get-Item -LiteralPath $sevenCache).Length -gt [int]$sevenFix.maxBytes) {
        throw 'Official 7-Zip Common0.h exceeded the expected header size.'
    }
    if((Get-GitBlobHash $sevenCache) -ne [string]$sevenFix.gitBlob) {
        throw 'Official 7-Zip Common0.h fingerprint differs from the pinned Git blob. Build stopped.'
    }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $sevenTarget) | Out-Null
    Copy-Item -LiteralPath $sevenCache -Destination $sevenTarget -Force
    Say 'Restored missing 7-Zip 26.01 Common0.h from official upstream (verified Git blob).'
}
if((Get-GitBlobHash $sevenTarget) -ne [string]$sevenFix.gitBlob) {
    throw 'SevenZ Common0.h is not the pinned official header. Build stopped.'
}
$changes=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'patches.json') -Raw | ConvertFrom-Json
$latin=[Text.Encoding]::GetEncoding(28591) # Preserve all original CP1251 bytes.
foreach($group in ($changes.changes | Group-Object file)) {
    $file=Join-Path $patched $group.Name
    $text=$latin.GetString([IO.File]::ReadAllBytes($file)).Replace("`r`n","`n")
    foreach($change in $group.Group) {
        $count=[regex]::Matches($text,[regex]::Escape([string]$change.old)).Count
        if($count -ne [int]$change.count){
            # Include the offending pattern so another upstream mismatch can be diagnosed from a log.
            $snippet=([string]$change.old -replace '\s+',' ').Trim()
            if($snippet.Length -gt 110){$snippet=$snippet.Substring(0,110)+'...'}
            throw "Patch precondition failed in $($group.Name) [pattern: $snippet]: expected $($change.count) occurrences, found $count. Build stopped."
        }
        $text=$text.Replace([string]$change.old,[string]$change.new)
    }
    [IO.File]::WriteAllBytes($file,$latin.GetBytes($text))
}
# Ranking remains byte-for-byte unchanged. The reviewed index-loader patches
# rebind the cached archive identity to the current local path; parsing is unchanged.
foreach($file in @('source\matcher.cpp')) {
    if((Get-FileHash -LiteralPath (Join-Path $origin $file)).Hash -ne (Get-FileHash -LiteralPath (Join-Path $patched $file)).Hash) {
        throw "Unexpected algorithm modification: $file"
    }
}
$rev=@'
#ifndef SVNREV_H
#define SVNREV_H
#define SVN_REV 0
#define SVN_REV_STR "0"
#define SVN_REV_D "00"
#define SVN_REV_M "00"
#define SVN_REV_Y 2026
#define SVN_BUILD_NOTE "SDIO Modern integration candidate - upstream mirror 25468bdb - NOT AN OFFICIAL RELEASE"
#define SVN_BUILD_REV "Pinned community mirror 25468bdb37d46a72ab4e8bf037b4e1a28ffec70e"
#define SVN_BUILD_DATE "Reproducible source profile; see build manifest"
#define COPYRIGHT_DATE "Original project copyrights retained"
#endif
'@
Write-Utf8 (Join-Path $patched 'source\svnrev.h') $rev
$receipt=@{preparedUtc=[DateTime]::UtcNow.ToString('o');originCommit=$lock.origin.commit;originVersion=$lock.origin.version;boost=$lock.boost.version;webpCommit=$lock.webp.commit;patchesApplied=$changes.changes.Count;matcherUnchanged=$true;indexingUnchanged=$false;indexingParserUnchanged=$true;sourceLockSha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'source-lock.json')).Hash;patchManifestSha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'patches.json')).Hash;windowsTested=$false}
Write-Utf8 (Join-Path $vendor 'SOURCE_RECEIPT.json') ($receipt|ConvertTo-Json -Depth 5)
Say 'Source prepared. This is not a compiled or Windows-validated application yet.'
