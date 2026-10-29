# Original SevenZ dependency closure for Qt 5.6.3 / MinGW 4.9.2.
# Only identifies code units omitted by the pinned Code::Blocks project.
# Does NOT compile every unrelated 7-Zip executable / UI module.
# Does NOT alter original SDIO matching, archive code, or install policy.

function Find-SevenZipMissingSources {
    param(
        [Parameter(Mandatory=$true)][string]$Nm,
        [Parameter(Mandatory=$true)][string]$Archive,
        [Parameter(Mandatory=$true)][string]$Origin,
        [Parameter(Mandatory=$true)][string]$Catalog,
        [Parameter(Mandatory=$true)][string[]]$Selected
    )
    $catalogData=Get-Content -LiteralPath $Catalog -Raw | ConvertFrom-Json
    if([int]$catalogData.schema -ne 1 -or
       [string]$catalogData.repository -ne 'IDisposable/sdio-go' -or
       [string]$catalogData.commit -ne '25468bdb37d46a72ab4e8bf037b4e1a28ffec70e' -or
       [string]$catalogData.root -ne 'external/SevenZ/build') {
        throw 'The SevenZ source catalog does not match the pinned source snapshot.'
    }
    # GNU nm reports symbols from the built static archive, including all
    # codec units currently forced by --whole-archive in the Qt link profile.
    $symbols=@(& $Nm '--extern-only' '--demangle' $Archive 2>&1)
    if($LASTEXITCODE -ne 0){throw 'MinGW nm could not inspect the 7-Zip archive.'}
    $defined=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    $undefined=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach($entry in $symbols) {
        $line=[string]$entry
        if($line -match '^\s*(?:[0-9a-fA-F]+\s+)?([A-Za-z])\s+(.+?)\s*$') {
            $kind=$Matches[1]
            $symbol=$Matches[2].Trim()
            if($kind -eq 'U') { [void]$undefined.Add($symbol) }
            elseif($kind -cmatch '^[A-Z]$') { [void]$defined.Add($symbol) }
        }
    }
    $selectedPaths=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach($source in $Selected) { [void]$selectedPaths.Add([IO.Path]::GetFullPath($source)) }
    $candidates=New-Object 'System.Collections.Generic.List[object]'
    $seen=New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach($item in @($catalogData.files)) {
        $rel=[string]$item.path
        if(-not $seen.Add($rel)){throw ('Duplicate path in SevenZ candidate catalog: '+$rel)}
        if($rel -notmatch '^(?:C|CPP)/[A-Za-z0-9_./-]+\.(?:c|cpp)$' -or $rel.Contains('..')) {
            throw ('Invalid source path in pinned catalog: '+$rel)
        }
        # These are separate executables / DLL stubs / duplicate precompiled
        # headers, not suitable for the in-process static archive.
        if($rel -match '(?:^|/)(?:StdAfx|DllExports\w*|CodecExports|DllSecur|LzmaLib)\.(?:c|cpp)$' -or
           $rel -match '/UI/Console/' -or $rel -match '/Windows/Control/' -or
           $rel -match '(?:Register|Reg)\.cpp$') { continue }
        $path=Join-Path (Join-Path $Origin 'external\SevenZ\build') ($rel.Replace('/','\'))
        $full=[IO.Path]::GetFullPath($path)
        if($selectedPaths.Contains($full)){continue}
        if(-not (Test-Path -LiteralPath $full -PathType Leaf)) {
            throw ('Pinned SevenZ candidate is missing: '+$rel)
        }
        $actual=Get-GitBlobHash $full
        if($actual -ne [string]$item.blob) {
            throw ('Pinned SevenZ candidate differs from reviewed Git commit: '+$rel)
        }
        # Confirm only the generated vendor is used. User-installed driver
        # folders and the pristine upstream source are never modified.
        $sourceOriginal=Join-Path (Join-Path $Origin '..\origin-upstream\external\SevenZ\build') ($rel.Replace('/','\'))
        if(-not (Test-Path -LiteralPath $sourceOriginal -PathType Leaf) -or
           (Get-GitBlobHash $sourceOriginal) -ne $actual) {
            throw ('SevenZ original-vs-generated source comparison failed: '+$rel)
        }
        $body=[IO.File]::ReadAllText($full)
        $candidates.Add([pscustomobject]@{Path=$full;Relative=$rel;Body=$body})
    }
    # Only choose original sources with a UNIQUE, explicit definition match.
    # Never fabricate a missing symbol, disable a codec, or pull arbitrary
    # unrelated Windows GUI / DLL modules into the engine.
    $add=New-Object 'System.Collections.Generic.List[string]'
    foreach($sym in ($undefined | Sort-Object)) {
        if($defined.Contains($sym)){continue}
        if($sym -match '^(?:__|_imp|std::|boost::|typeinfo |vtable |VTT |operator |Q[A-Z])') {continue}
        if($sym -notmatch '::' -and $sym -notmatch '^(?:z7_|Huffman_|Ppmd|Lzma|Xz|Sha|Crc|Bwt|Bcj|Aes|Blake|Mt|Sort|BZip)') {continue}
        $simple=($sym -replace '\(.*$','').Trim()
        if($simple.Length -lt 4 -or $simple -notmatch '^[A-Za-z_][A-Za-z0-9_:~]*$') {continue}
        $parts=@($simple -split '::')
        $tail=$parts[$parts.Length-1]
        $found=New-Object 'System.Collections.Generic.List[string]'
        foreach($candidate in $candidates) {
            # Fully qualified symbol definition (or its last Class::member
            # pair when the file is already inside C++ namespaces).
            if($parts.Length -ge 2) {
                $pair=$parts[$parts.Length-2]+'::'+$tail
                $exact='(?m)^\s*(?!//|#).{0,155}\b'+[regex]::Escape($pair)+'\s*(?:\(|\[|=|;)'
                $match=[regex]::IsMatch($candidate.Body,$exact)
                # Some headers declare global namespace variables while the
                # implementation uses namespace N {...}; match that safely.
                if(-not $match -and $parts.Length -eq 2 -and $tail -cmatch '^k[A-Z]' -and
                   $candidate.Body.Contains('namespace '+$parts[0])) {
                    $match=[regex]::IsMatch($candidate.Body,'(?m)^\s*(?!//|#).{0,75}\b'+[regex]::Escape($tail)+'\s*\[')
                }
            } else {
                # Definition-like line for unmangled C functions. A mere
                # call in another .cpp file is NOT a sufficient match.
                $exact='(?m)^\s*(?!if\b|for\b|while\b|return\b|switch\b|//).{0,140}\b'+[regex]::Escape($tail)+'\s*\([^;{}]*\)\s*(?:throw\(\)\s*)?\{'
                $match=[regex]::IsMatch($candidate.Body,$exact)
            }
            if($match){$found.Add($candidate.Path)}
        }
        if($found.Count -eq 1) {
            if(-not $add.Contains($found[0])){$add.Add($found[0])}
        }
    }
    return $add.ToArray()
}
