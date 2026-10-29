param(
    [string]$ProjectRoot=(Split-Path -Parent (Split-Path -Parent $PSScriptRoot)),
    [switch]$EnableExperimentalInstall,
    [switch]$SingleFile,
    [switch]$NoLaunch,
    [switch]$SkipSourceDownload
)
. (Join-Path $PSScriptRoot 'common.ps1')
$ProjectRoot=[IO.Path]::GetFullPath($ProjectRoot)
if($ProjectRoot.Length -gt 70){throw 'The legacy build tools need short paths. Extract the project to C:\SDIO-Modern and retry.'}
if($ProjectRoot -match '[%&!\r\n"]'){throw 'Use a short local project path without shell metacharacters, for example C:\SDIO-Modern.'}
$profile='matching'
if($EnableExperimentalInstall){$profile='install-test'}
if($SingleFile){$profile+='-static'}
$build=Join-Path $ProjectRoot ('build-origin-'+$profile)
$dist=Join-Path $ProjectRoot ('dist-origin-'+$profile)
New-Item -ItemType Directory -Force -Path $build | Out-Null
$log=Join-Path $build 'build-native.log'
$transcript=$false
try {
    Start-Transcript -Path $log -Append | Out-Null;$transcript=$true
    $qmake=Find-Qt;$mingw=Find-MinGW $qmake;$gcc=Assert-Compiler $mingw
    $qtBin=Split-Path -Parent $qmake
    $env:PATH=$mingw+';'+$qtBin+';'+$env:PATH
    $make=Join-Path $mingw 'mingw32-make.exe'
    if(-not (Test-Path -LiteralPath $make)){throw 'mingw32-make.exe is missing.'}
    $qtPrefix=((& $qmake -query QT_INSTALL_PREFIX)|Out-String).Trim();Require-Exit 'Query Qt prefix'
    & (Join-Path $PSScriptRoot 'build_torrent.ps1') -ProjectRoot $ProjectRoot -Qmake $qmake -MinGW $mingw
    $qtConfig=Join-Path $qtPrefix 'mkspecs\qconfig.pri'
    $staticQt=$false
    if(Test-Path -LiteralPath $qtConfig){$staticQt=(Get-Content -LiteralPath $qtConfig -Raw) -match '(?m)^QT_CONFIG\s*[+:]?=.*\bstatic\b'}
    if($SingleFile -and -not $staticQt){throw 'Single-file build requires a genuinely STATIC Qt 5.6.3 SDK. The installed official Qt DLL SDK cannot be turned static by adding a flag. See docs/integration/STATIC_BUILD_AR.md.'}
    if(-not $SkipSourceDownload){& (Join-Path $PSScriptRoot 'prepare_sources.ps1') -ProjectRoot $ProjectRoot}
    $origin=Join-Path $ProjectRoot 'vendor\origin-patched'
    $boost=Join-Path $ProjectRoot 'vendor\boost'
    $webp=Join-Path $ProjectRoot 'vendor\webp'
    foreach($path in @((Join-Path $origin 'source\matcher.cpp'),(Join-Path $boost 'boost\version.hpp'),(Join-Path $webp 'src\webp\decode.h'))) {
        if(-not (Test-Path -LiteralPath $path -PathType Leaf)){throw "Missing dependency source: $path"}
    }
    # Boost.Filesystem 1.79 defaults to std::atomic_ref (C++20).
    # Qt 5.6.3's MinGW 4.9.2 is C++11-only; use Boost.Atomic instead.
    # This is an official Boost 1.79 source switch, not a thread-unsafe shim.
    $boostAtomicRef=Join-Path $boost 'libs\filesystem\src\atomic_ref.hpp'
    if(-not (Test-Path -LiteralPath $boostAtomicRef -PathType Leaf)) {
        throw 'Boost 1.79 filesystem atomic_ref.hpp is missing; cannot select the verified C++11 compatibility path.'
    }
    $boostAtomicText=[IO.File]::ReadAllText($boostAtomicRef)
    if($boostAtomicText -notmatch 'BOOST_FILESYSTEM_NO_CXX20_ATOMIC_REF' -or
       $boostAtomicText -notmatch 'namespace atomic_ns = boost;') {
        throw 'Boost.Filesystem atomic_ref.hpp does not expose the reviewed Boost.Atomic C++11 fallback; build stopped.'
    }
    # Fail before launching parallel 7-Zip compile jobs if this patched vendor
    # source is incomplete. prepare_sources.ps1 supplies the verified header.
    $sevenCommon0=Join-Path $origin 'external\SevenZ\build\CPP\Common\Common0.h'
    if(-not (Test-Path -LiteralPath $sevenCommon0 -PathType Leaf)) {
        throw 'Bundled SevenZ Common0.h is missing. Run source preparation from this fixed project (not an old script).'
    }
    if((Get-GitBlobHash $sevenCommon0) -ne '5781a957b105c6a8e397561fbcdf2e11d76183ee') {
        throw 'Bundled SevenZ Common0.h hash mismatch. Do not compile unverified vendor headers.'
    }
    $lib=Join-Path $build 'lib'
    $projects=Join-Path $build 'projects'
    New-Item -ItemType Directory -Force -Path $lib,$projects | Out-Null
    $xp='WIN32 UNICODE _UNICODE _WIN32_WINNT=0x0501 WINVER=0x0501 BOOST_USE_WINAPI_VERSION=0x0501 BOOST_USE_NTDDI_VERSION=0x05010000'
    function Make-Library([string]$Name,[string[]]$Sources,[string[]]$Includes,[string]$Defines,[string]$Extra='') {
        if(-not $Sources -or $Sources.Count -eq 0){throw "No sources were selected for $Name"}
        $units=Join-Path $projects ($Name+'-units');New-Item -ItemType Directory -Force -Path $units | Out-Null
        $wrapped=@();$i=0
        foreach($source in $Sources) {
            if(-not (Test-Path -LiteralPath $source -PathType Leaf)){throw "Upstream build unit is missing: $source"}
            $ext=[IO.Path]::GetExtension($source).ToLowerInvariant()
            $out=Join-Path $units ('unit_{0:D4}{1}' -f $i,$ext);$i++
            # Unique filenames avoid qmake object collisions across 7-Zip folders.
            $wrapperContent='#include '+(QPath ([IO.Path]::GetFullPath($source)))+"`n"
            if(-not (Test-Path -LiteralPath $out -PathType Leaf) -or
               [IO.File]::ReadAllText($out) -cne $wrapperContent) {
                Write-Utf8 $out $wrapperContent
            }
            $wrapped+=(QPath $out)
        }
        $inc=($Includes | ForEach-Object {QPath $_}) -join ' '
        $pro=Join-Path $projects ($Name+'.pro')
        $body=@"
TEMPLATE = lib
CONFIG -= qt debug debug_and_release shared
CONFIG += staticlib release c++11
TARGET = $Name
DESTDIR = $(QPath $lib)
DEFINES += $Defines
INCLUDEPATH += $inc
QMAKE_CFLAGS_RELEASE = -O2 -std=gnu99 -ffunction-sections -fdata-sections
QMAKE_CXXFLAGS_RELEASE = -O2 -ffunction-sections -fdata-sections
SOURCES += $($wrapped -join " `nSOURCES += ")
$Extra
"@
        if(-not (Test-Path -LiteralPath $pro -PathType Leaf) -or
           [IO.File]::ReadAllText($pro) -cne $body) {
            Write-Utf8 $pro $body
        }
        Build-Qmake $qmake $make $pro (Join-Path $build $Name)
        $archive=Join-Path $lib ('lib'+$Name+'.a')
        if(-not (Test-Path -LiteralPath $archive)){throw "Static library was not produced: $archive"}
        return $archive
    }
    # On MinGW GCC 4.9.x the old Speech API SDK header/libs are absent.
    # system.cpp never uses speech interfaces. Source preparation removes only
    # its unused #include <sapi.h> from the generated/patched vendor tree.
    $systemCpp=Join-Path $origin 'source\system.cpp'
    if(-not (Test-Path -LiteralPath $systemCpp -PathType Leaf)){throw 'Prepared Origin system.cpp missing.'}
    if(([IO.File]::ReadAllText($systemCpp)) -match '(?m)^\s*#include\s*<sapi\.h>'){throw 'Legacy speech header was not removed from prepared Origin system.cpp. Run current prepare_sources.ps1.'}
    # Parse reviewed original Code::Blocks units, preserving the C++ engine and 7-Zip implementation.
    [xml]$cbp=Get-Content -LiteralPath (Join-Path $origin 'project\7zip\lib.cbp') -Raw
    $seven=@()
    foreach($u in $cbp.CodeBlocks_project_file.Project.Unit) {
        $n=[string]$u.filename
        if($n -match '\.(c|cpp)$'){$seven+=[IO.Path]::GetFullPath((Join-Path (Join-Path $origin 'project\7zip') $n))}
    }
    # Upstream Code::Blocks 7-Zip project omits C/Xz.c, even though it
    # includes XzIn.c and XzDec.c. Xz.c defines XZ_SIG and Xz_Free.
    # Pull the exact source already present in the pinned SDIO Origin snapshot,
    # not a new DLL or unverified download.
    $xzSource=Join-Path $origin 'external\SevenZ\build\C\Xz.c'
    if(-not (Test-Path -LiteralPath $xzSource -PathType Leaf)) {
        throw 'Origin 7-Zip Xz.c is missing. Refusing to link an incomplete archive.'
    }
    if((Get-GitBlobHash $xzSource) -ne '92bae029e60813bce196bea2915d83abb9c70769') {
        throw 'Origin 7-Zip Xz.c differs from the pinned source; build stopped.'
    }
    $xzFull=[IO.Path]::GetFullPath($xzSource)
    if($seven -contains $xzFull){throw '7-Zip project unexpectedly includes Xz.c already; review the source manifest before rebuilding.'}
    $seven+=$xzFull
    # Fix 7: Xz.c uses SHA256 and CRC64. Both implementations (plus their
    # optimized helpers) were omitted from this legacy Code::Blocks 7-Zip
    # project. They are pinned files in the same reviewed Origin snapshot.
    # Don't synthesize cryptographic functions or silently skip missing files.
    $xzDependencies=@(
        @{name='Sha256.c'; blob='b29f06bcd1ec75b18ac7c4c593d8cb0fc0c9403d'}
        @{name='Sha256Opt.c'; blob='291e71c120b85aabb2789fe8121b680487bfa687'}
        @{name='XzCrc64.c'; blob='a0023a6fa2c8b459f3d931f399a31f3f20225397'}
        @{name='XzCrc64Opt.c'; blob='08a419ca526349c70740853b525b1c8e64f88e72'}
    )
    foreach($dependency in $xzDependencies) {
        $dependencyPath=Join-Path $origin ('external\SevenZ\build\C\'+$dependency.name)
        if(-not (Test-Path -LiteralPath $dependencyPath -PathType Leaf)) {
            throw ('Pinned SevenZ dependency is missing: '+$dependency.name)
        }
        if((Get-GitBlobHash $dependencyPath) -ne $dependency.blob) {
            throw ('Pinned SevenZ dependency hash mismatch: '+$dependency.name)
        }
        $dependencyFull=[IO.Path]::GetFullPath($dependencyPath)
        if($seven -contains $dependencyFull) {
            throw ('SevenZ original project unexpectedly includes '+$dependency.name+' already; review the manifest before compiling.')
        }
        $seven+=$dependencyFull
    }
    # Fix 8: the same pinned legacy Code::Blocks file omits additional build
    # units that its other included sources actually call.  Keep the original
    # implementations: Sort.c defines HeapSort; FileSystem.cpp defines
    # NWindows::NFile::NSystem::MyGetDriveType; ZipRegistry.cpp defines
    # NWorkDir::CInfo::Load. No placeholder or fake functions are used.
    # Every extra unit belongs to the reviewed Origin git commit, and the
    # source is checked by Git blob SHA before being accepted for the archive.
    $linkerDependencies=@(
        @{path='external\SevenZ\build\C\Sort.c'; blob='0c30ca8bc323ec641272c607fb5c2c48c50e03f2'}
        # Fix 11: LZMS and other codecs need Huffman_Generate, implemented
        # in this original 7-Zip C unit (it calls already-linked HeapSort).
        @{path='external\SevenZ\build\C\HuffEnc.c'; blob='04faf9e824fd3c1d594c4306fb83c106b6984f7c'}
        # Fix 13: registered ByteSwap2/4 codec filters call z7_SwapBytes2/4.
        # Compile the unchanged pinned C implementation (also defines
        # z7_SwapBytesPrepare); this prevents missing filter byte-swap ops.
        @{path='external\SevenZ\build\C\SwapBytes.c'; blob='d382c327a0c142b4383234cbeed98a99b75a3293'}
        @{path='external\SevenZ\build\CPP\Windows\FileSystem.cpp'; blob='02faf627b10a6722441da7e9bbc03827ec2df744'}
        @{path='external\SevenZ\build\CPP\7zip\UI\Common\ZipRegistry.cpp'; blob='f4c27bc93feb46aea9f6f6ee363b5f091271211e'}
        # Fix 9: Rar3Decoder.cpp constructs NVm::CVm but baseline lib.cbp omits Rar3Vm.cpp.
        @{path='external\SevenZ\build\CPP\7zip\Compress\Rar3Vm.cpp'; blob='e6bfd7ea689bcd1fc07cbfee99fdd4678c16a282'}
        # Fix 10: RAR1/2 decoders call CLzOutWindow::Init, and PPMD codec
        # registration needs the real encoder and decoder implementations.
        @{path='external\SevenZ\build\CPP\7zip\Compress\LzOutWindow.cpp'; blob='213e0a69ce56e47d7501dea4f1a8bce42929196d'}
        @{path='external\SevenZ\build\CPP\7zip\Compress\PpmdDecoder.cpp'; blob='e2399dc4b10923d8bf0b17a77557331ea6261cb1'}
        @{path='external\SevenZ\build\CPP\7zip\Compress\PpmdEncoder.cpp'; blob='16a40d69f52f1278c65cf63971bac354718bd3ff'}
        # Fix 12: Deflate decoder needs NBitl::kReverseTable; compile the
        # original C++ definition and initializer from pinned SevenZ.
        @{path='external\SevenZ\build\CPP\7zip\Compress\BitlDecoder.cpp'; blob='1236bab4b666f3bcc8874d60be19630cb42af59c'}
        # Fix 14: BZip2 decoder needs the real static CRC lookup table.
        @{path='external\SevenZ\build\CPP\7zip\Compress\BZip2Crc.cpp'; blob='c1709170b53c413717db10bcdda6809417e0b571'}
    )
    foreach($linkerUnit in $linkerDependencies) {
        $unitPath=Join-Path $origin $linkerUnit.path
        if(-not (Test-Path -LiteralPath $unitPath -PathType Leaf)) {
            throw ('Pinned SevenZ linker unit is missing: '+$linkerUnit.path)
        }
        if((Get-GitBlobHash $unitPath) -ne $linkerUnit.blob) {
            throw ('Pinned SevenZ linker unit hash mismatch: '+$linkerUnit.path)
        }
        $unitFull=[IO.Path]::GetFullPath($unitPath)
        if($seven -contains $unitFull) {
            throw ('Original SevenZ build list already includes '+$linkerUnit.path+'; review duplicate before compiling.')
        }
        $seven+=$unitFull
    }
    # Fix 14: verify the ACTUAL compiled archive's unresolved symbols using
    # the matching MinGW nm.exe. Select only exact pinned original providers,
    # instead of discovering a new 7-Zip dependency after every Windows link.
    . (Join-Path $PSScriptRoot 'resolve_7zip.ps1')
    $nm=Join-Path $mingw 'nm.exe'
    if(-not (Test-Path -LiteralPath $nm -PathType Leaf)) {
        throw 'MinGW nm.exe is missing. The pinned SevenZ dependency closure cannot run.'
    }
    $catalog=Join-Path $PSScriptRoot '7zip-source-catalog.json'
    $closureLog=Join-Path $build '7zip-closure-report.txt'
    $closureLines=@('7-Zip source dependency closure; NOT Windows runtime validation')
    $closureDone=$false
    $sevenDef=$xp+' _CONSOLE EXCLUDE_COM NO_REGISTRY _MBCS _NO_CRYPTO EXTRACT_ONLY PROG_VARIANT_R'
    for($pass=1;$pass -le 14;$pass++) {
        Say ('Building original 7-Zip library; dependency pass '+$pass+'...')
        $sevenLib=Make-Library 'sdio_7z' $seven @($origin,(Join-Path $origin 'external\SevenZ\build\C')) $sevenDef
        $missing=@(Find-SevenZipMissingSources -Nm $nm -Archive $sevenLib -Origin $origin -Catalog $catalog -Selected $seven)
        if($missing.Count -eq 0){
            $closureLines+=('Pass '+$pass+': no additional uniquely identified original source provider')
            $closureDone=$true
            break
        }
        $closureLines+=('Pass '+$pass+': '+($missing.Count)+' provider(s)')
        foreach($source in $missing) {
            $relative=$source.Substring($origin.TrimEnd('\').Length+1)
            $closureLines+=' + '+$relative
            Say ('SevenZ closure: adding pinned original '+$relative)
            $seven+=$source
        }
    }
    Write-Utf8 $closureLog (($closureLines -join "`r`n")+"`r`n")
    if(-not $closureDone) {
        throw 'SevenZ source closure exceeded 14 passes. See 7zip-closure-report.txt; do not proceed with an incomplete archive.'
    }
    Say 'Building Boost filesystem and portable WebP sources...'
    $boostSources=@(Get-ChildItem -LiteralPath (Join-Path $boost 'libs\filesystem\src') -Filter '*.cpp' -File | ForEach-Object {$_.FullName})
    $boostLib=Make-Library 'sdio_boost_fs' $boostSources @($boost) ($xp+' BOOST_ALL_NO_LIB BOOST_FILESYSTEM_STATIC_LINK BOOST_FILESYSTEM_SOURCE BOOST_ERROR_CODE_HEADER_ONLY BOOST_FILESYSTEM_NO_CXX20_ATOMIC_REF')
    $webpSources=@()
    foreach($part in @('src\dec','src\enc','src\dsp','src\utils','src\mux','src\demux','sharpyuv')) {
        $dir=Join-Path $webp $part
        if(Test-Path -LiteralPath $dir){$webpSources+=@(Get-ChildItem -LiteralPath $dir -Filter '*.c' -File | ForEach-Object {$_.FullName})}
    }
    $webpLib=Make-Library 'sdio_webp' $webpSources @($webp,(Join-Path $webp 'src')) ($xp+' WEBP_STATIC')
    [xml]$cbp=Get-Content -LiteralPath (Join-Path $origin 'project\Drivers.cbp') -Raw
    $core=@()
    foreach($u in $cbp.CodeBlocks_project_file.Project.Unit) {
        $n=[string]$u.filename
        if($n -match '\.cpp$' -and $n -notmatch '[/\\]update\.cpp$'){$core+=[IO.Path]::GetFullPath((Join-Path (Join-Path $origin 'project') $n))}
    }
    $core+=(Join-Path $ProjectRoot 'native\origin_native.cpp')
    $defs=$xp+' SDIO_MODERN_EMBEDDED BOOST_ALL_NO_LIB BOOST_FILESYSTEM_STATIC_LINK BOOST_ERROR_CODE_HEADER_ONLY'
    if($EnableExperimentalInstall){$defs+=' SDIO_MODERN_ENABLE_EXPERIMENTAL_INSTALL'}
    Say 'Building the original C++ engine in-process; torrent updater is excluded in this profile.'
    $coreLib=Make-Library 'sdio_origin_core' $core @((Join-Path $origin 'source'),(Join-Path $origin 'resources'),(Join-Path $origin 'external\SevenZ\build\C'),$boost,(Join-Path $webp 'src'),(Join-Path $ProjectRoot 'native')) $defs 'QMAKE_CXXFLAGS += -finput-charset=CP1251 -fexec-charset=CP1251'
    $resources=Join-Path $build 'origin_resources.rc'
    # GNU ld concatenates separate .rsrc trees instead of merging their roots.
    # Compile the app and engine resources together so Windows can see VERSIONINFO,
    # the XP manifest, icons AND the engine language/helper resources.
    $resourceText="// Unified application and Origin resource tree.`n"
    $resourceText+='#include '+(QPath (Join-Path $ProjectRoot 'resources.rc'))+"`n"
    $resourceText+='203 100 '+(QPath (Join-Path $origin 'tools\SDIO\langs\english.txt'))+"`n"
    $resourceText+='202 100 '+(QPath (Join-Path $origin 'tools\SDIO\themes\clean.txt'))+"`n"
    $resourceText+='204 100 '+(QPath (Join-Path $origin 'resources\gpl-3.0.txt'))+"`n"
    if($EnableExperimentalInstall) {
        $helper=Join-Path $build 'helper64\install64.exe'
        & (Join-Path $PSScriptRoot 'build_install_helper.ps1') -OriginRoot $origin -Output $helper
        $resourceText+='209 100 '+(QPath $helper)+"`n"
    }
    Write-Utf8 $resources $resourceText
    $resObject=Join-Path $build 'origin_resources.o'
    & (Join-Path $mingw 'windres.exe') '-i' $resources '-o' $resObject '-O' 'coff';Require-Exit 'Embed Origin resources'
    $pri=Join-Path $build 'origin-link.pri'
    $defineLine='DEFINES += SDIO_ORIGIN_LINKED'
    if($EnableExperimentalInstall){$defineLine+=' SDIO_MODERN_ENABLE_EXPERIMENTAL_INSTALL'}
    $link=@"
# Generated only after successful native library builds.
$defineLine
RC_FILE = $(QPath $resources)
RC_INCLUDEPATH += $(QPath $ProjectRoot)
LIBS += -Wl,--start-group $(QPath $coreLib) -Wl,--whole-archive $(QPath $sevenLib) -Wl,--no-whole-archive $(QPath $boostLib) $(QPath $webpLib) -Wl,--end-group
LIBS += -lcomctl32 -lgdi32 -liphlpapi -lmsimg32 -lkernel32 -luser32 -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -lsetupapi -lshlwapi -lnewdev -luuid -lwbemuuid -lurlmon -lwinmm -lversion -lws2_32 -lwsock32 -lwininet
QMAKE_LFLAGS += -Wl,--gc-sections
"@
    Write-Utf8 $pri $link
    $extra=@('CONFIG+=origin_engine',('SDIO_ORIGIN_PRI='+($pri -replace '\\','/')))
    if($SingleFile){$extra+='CONFIG+=single_file'}
    Build-Qmake $qmake $make (Join-Path $ProjectRoot 'SDIO_Modern_Qt.pro') (Join-Path $build 'app') $extra
    $candidates=@((Join-Path $build 'app\release\SnappyDriverInstaller-TS.exe'),(Join-Path $build 'app\SnappyDriverInstaller-TS.exe'))
    $built=$candidates | Where-Object {Test-Path -LiteralPath $_ -PathType Leaf} | Select-Object -First 1
    if(-not $built){throw 'Integrated executable not found after linking.'}
    # Catch stale qmake object/resource files before writing the build manifest.
    $version=[Diagnostics.FileVersionInfo]::GetVersionInfo($built)
    if($version.FileMajorPart -ne 0 -or $version.FileMinorPart -ne 21){
        throw ('Stale Windows executable/resource detected: '+$version.FileVersion+'. Expected 0.19.x. The current UI was not linked.')
    }
    New-Item -ItemType Directory -Force -Path $dist | Out-Null
    $exe=Join-Path $dist 'SnappyDriverInstaller-TS.exe';Copy-Item -LiteralPath $built -Destination $exe -Force
    if(-not $staticQt) {
        $deploy=Join-Path $qtBin 'windeployqt.exe'
        if(-not (Test-Path -LiteralPath $deploy)){throw 'windeployqt is needed for the dynamic development build.'}
        $help=((& $deploy '--help' 2>&1)|Out-String)
        $deployArgs=@('--release')
        foreach($flag in @('--no-translations','--no-system-d3d-compiler','--no-opengl-sw')) {
            if($help.Contains($flag)){$deployArgs+=$flag}
        }
        & $deploy @deployArgs $exe
        Require-Exit 'Deploy Qt development runtime'
        foreach($name in @('libgcc_s_dw2-1.dll','libgcc_s_sjlj-1.dll','libstdc++-6.dll','libwinpthread-1.dll')) {
            $f=Join-Path $mingw $name;if(Test-Path -LiteralPath $f){Copy-Item -LiteralPath $f -Destination $dist -Force}
        }
    }
    $imports=((& (Join-Path $mingw 'objdump.exe') '-p' $exe)|Out-String);Require-Exit 'Read executable imports'
    Write-Utf8 (Join-Path $build 'exe-imports.txt') $imports
    $dlls=@([regex]::Matches($imports,'(?im)DLL Name:\s*([^\r\n]+)') | ForEach-Object {$_.Groups[1].Value.Trim().ToLowerInvariant()})
    if($SingleFile) {
        $systemDlls=@('mpr.dll','kernel32.dll','user32.dll','gdi32.dll','advapi32.dll','shell32.dll','ole32.dll','oleaut32.dll','comdlg32.dll','comctl32.dll','ws2_32.dll','wsock32.dll','winmm.dll','version.dll','imm32.dll','uxtheme.dll','shlwapi.dll','msvcrt.dll','iphlpapi.dll','setupapi.dll','cfgmgr32.dll','newdev.dll','usp10.dll','winspool.drv','ntdll.dll','msimg32.dll','urlmon.dll','wininet.dll','dwmapi.dll','crypt32.dll','wintrust.dll','userenv.dll','secur32.dll')
        $unexpected=@($dlls | Where-Object {$systemDlls -notcontains $_})
        if($unexpected.Count){throw ('Single-file dependency audit requires review: '+($unexpected -join ', '))}
        if($dlls -contains 'dwmapi.dll'){throw 'Static import of DWMAPI cannot load on XP. Use runtime loading or remove the dependency before claiming XP support.'}
    }
    # Preserve the exact Origin language catalog (41 upstream locales in this source pin).
    # These strings localize matching SDIO concepts. The custom Qt interface has
    # full English/Arabic translation only; unsupported Qt phrases fall back.
    $langSource=Join-Path $origin 'tools\SDIO\langs'
    if(Test-Path -LiteralPath $langSource -PathType Container) {
        $langDest=Join-Path $dist 'langs'
        New-Item -ItemType Directory -Force -Path $langDest | Out-Null
        Copy-Item -Path (Join-Path $langSource '*.txt') -Destination $langDest -Force
    }
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'themes') -Destination $dist -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $ProjectRoot 'LICENSE-GPL-3.0.txt') -Destination $dist -Force
    $licenseDir=Join-Path $dist 'licenses'
    New-Item -ItemType Directory -Force -Path $licenseDir | Out-Null
    foreach($fontLicense in @('Cairo-OFL.txt','Satoshi-FFL.txt','PROVENANCE.json')){
        Copy-Item -LiteralPath (Join-Path $ProjectRoot ('assets/fonts/'+$fontLicense)) -Destination (Join-Path $licenseDir ('Fonts-'+$fontLicense)) -Force
    }
    New-Item -ItemType Directory -Force -Path $licenseDir | Out-Null
    foreach($license in @(
        @{path=(Join-Path $origin 'LICENSE');name='SDIO-Origin-LICENSE.txt'},
        @{path=(Join-Path $boost 'LICENSE_1_0.txt');name='Boost-LICENSE.txt'},
        @{path=(Join-Path $webp 'COPYING');name='WebP-COPYING.txt'}
        @{path=(Join-Path $ProjectRoot 'assets\Dreams-reference-MIT.txt');name='Dreams-reference-MIT.txt'}
    )) {
        if(Test-Path -LiteralPath $license.path){Copy-Item -LiteralPath $license.path -Destination (Join-Path $licenseDir $license.name) -Force}
    }
    $qtLicenses=@((Join-Path $qtPrefix 'licenses'),(Join-Path (Split-Path -Parent (Split-Path -Parent $qtPrefix)) 'Licenses'))
    foreach($folder in $qtLicenses){if(Test-Path -LiteralPath $folder -PathType Container){Copy-Item -LiteralPath $folder -Destination (Join-Path $licenseDir 'Qt-SDK-Licenses') -Recurse -Force;break}}
    # Our source-built static SDK does not install the original license files.
    $qtSource=Join-Path $ProjectRoot 'vendor\qtbase-opensource-src-5.6.3'
    foreach($name in @('LICENSE.GPLv3','LICENSE.LGPLv21','LICENSE.LGPLv3','LICENSE.FDL')){
        $qtLicense=Join-Path $qtSource $name
        if(Test-Path -LiteralPath $qtLicense -PathType Leaf){Copy-Item -LiteralPath $qtLicense -Destination (Join-Path $licenseDir ('Qt-'+$name+'.txt')) -Force}
    }
    Write-Utf8 (Join-Path $dist 'DEVELOPMENT_BUILD_NOT_FOR_RELEASE.txt') 'This build has not passed Windows integration, matching parity, installation rollback, or XP compatibility tests. Source/license redistribution review is also required before public distribution.'

    $torrentLicense=Join-Path $ProjectRoot 'vendor\libtorrent-1.2.20\COPYING'
    if(Test-Path -LiteralPath $torrentLicense){Copy-Item -LiteralPath $torrentLicense -Destination (Join-Path $licenseDir 'libtorrent-COPYING.txt') -Force}
    $torrentReceipt=Get-Content -LiteralPath (Join-Path $ProjectRoot 'build-torrent-static\SOURCE_RECEIPT.json') -Raw | ConvertFrom-Json
    $sourceReceipt=Get-Content -LiteralPath (Join-Path $ProjectRoot 'vendor\SOURCE_RECEIPT.json') -Raw | ConvertFrom-Json
    $qtDpiReceipt=$null;$qtDpiReceiptPath=Join-Path $qtPrefix 'FRACTIONAL_DPI_PATCH.json'
    if(Test-Path -LiteralPath $qtDpiReceiptPath){$qtDpiReceipt=Get-Content -LiteralPath $qtDpiReceiptPath -Raw | ConvertFrom-Json;if($qtDpiReceipt.widgetsLibrarySha256 -ne (Get-FileHash -LiteralPath (Join-Path $qtPrefix 'lib\libQt5Widgets.a') -Algorithm SHA256).Hash.ToLowerInvariant()){throw 'The Qt fractional-DPI receipt does not match the installed static library.'};Copy-Item -LiteralPath $qtDpiReceiptPath -Destination (Join-Path $dist 'QT_FRACTIONAL_DPI_PATCH.json') -Force}
    $manifest=@{releaseTag='SDIO-MODERN';builtUtc=[DateTime]::UtcNow.ToString('o');profile=$profile;qt='5.6.3';qtFractionalDpiPatch=$qtDpiReceipt;compiler=$gcc;nativeLinked=$true;experimentalInstall=[bool]$EnableExperimentalInstall;torrentDownloads=$true;torrentSource=$torrentReceipt;verifiedWebseedDownloads=$true;updateCatalogUrl='https://www.glenn.delahoy.com/downloads/sdio/SDIO_Update.torrent';staticQt=[bool]$staticQt;exeBytes=(Get-Item -LiteralPath $exe).Length;exeSha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash;source=$sourceReceipt;runtimeTested=$false;xpCertified=$false;parityTested=$false;imports=$dlls}
    Write-Utf8 (Join-Path $dist 'BUILD_MANIFEST.json') ($manifest|ConvertTo-Json -Depth 8)
    Say "Integrated build produced: $exe"
    Say 'A successful compile does NOT certify driver matching parity, installation safety, or XP compatibility.'
    if(-not $NoLaunch -and -not $EnableExperimentalInstall){Start-Process -FilePath $exe -WorkingDirectory $dist}
    if($EnableExperimentalInstall){Say 'Installation-test build will NOT auto-run. Use an isolated Windows test machine. Run as Administrator only when intentionally testing installation.'}
} catch {
    Write-Host ('BUILD STOPPED: '+$_.Exception.Message) -ForegroundColor Red
    Write-Host ('Log: '+$log)
    throw
} finally {if($transcript){Stop-Transcript | Out-Null}}
