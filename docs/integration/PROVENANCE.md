# Source provenance and build boundaries

## Original C++ baseline
Repository: `IDisposable/sdio-go`
Commit: `25468bdb37d46a72ab4e8bf037b4e1a28ffec70e`
Subdirectory: `SDIO-baseline`
Declared version: 2.0.3.

This is a community mirror, not a verified checkout of official SVN r889 and not a claim about the latest official version. The Go rewrite in the same repository is excluded. Source archives are downloaded on the Windows build machine; they are not embedded in this source ZIP.

The lock file records critical Git blob IDs. The preparation script checks these before applying fail-closed patches to a separate copy. These Git/SHA checks are integrity checks, not publisher code signing.

## Source modifications
`patches.json` provides exact replacements and occurrence counts:
- allow a narrow friend adapter to access original candidate arrays/instance IDs;
- rename the legacy WinMain so Qt owns the only application window;
- skip legacy widget callbacks in headless installation;
- prevent automatic clicks on Windows/security dialogs;
- preserve SetupAPI logs;
- let the adapter own cleanup of its private extraction directory;
- correct the x64 helper's CommandLineToArgvW declaration include.

The original `matcher.cpp` and `indexing.cpp` remain byte-for-byte unchanged. This does NOT by itself prove behavioural parity: initialization, filters, installed-driver ordering, UI mapping and installer preconditions must be tested on Windows.

## Dependencies
- Boost 1.79.0 archive: checksum from the official release history.
- libwebp 1.6.0 source: tag commit `4fa21912338357f89e4fd51cf2368325b59e9bd9`.
- 7-Zip: the sources already referenced by the pinned baseline's `project/7zip/lib.cbp`.
- Qt: the developer's exact 5.6.3 toolkit; not redistributed in this archive.

A successful build emits SOURCE_RECEIPT.json, BUILD_MANIFEST.json, import listings and source/build logs. These files must never change runtimeTested/parityTested/xpCertified to true merely because compilation succeeded.

## Primary references
- https://www.glenn.delahoy.com/snappy-driver-installer-origin/
- https://www.glenn.delahoy.com/community/snappy-driver-installer-origin/sdio-v2-0-0-beta/
- https://github.com/IDisposable/sdio-go/tree/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline
- https://live.boost.org/users/history/version_1_79_0.html
- https://github.com/webmproject/libwebp/tree/4fa21912338357f89e4fd51cf2368325b59e9bd9
- https://download.qt.io/new_archive/qt/5.6/5.6.3/
- https://wiki.qt.io/Build_Standalone_Qt_Application_for_Windows

No binaries from the user-supplied commercial UI reference are executed or incorporated.

Fix 6 pins original `source/baseboard.cpp` (6f021396b96ab543f4c54a7c0eb2cca3f7b2f59c) and 7-Zip `C/Xz.c` (92bae029e60813bce196bea2915d83abb9c70769); only the generated WMI source is adapted; original 7-Zip Xz.c is compiled unchanged.

Fix 7 pins the exact unmodified 7-Zip Sha256.c/Sha256Opt.c/XzCrc64.c/XzCrc64Opt.c units from Origin mirror commit 25468bdb. These close the dependencies of Xz.c; no cryptographic stubs are used.

Fix 8 pins the three original unchanged 7-Zip Sort.c, Windows/FileSystem.cpp and UI/Common/ZipRegistry.cpp units to resolve linker references from the reviewed pinned Origin source. The original matcher and INF parsers were not altered.

Fix 9 pins original unchanged 7-Zip Rar3Vm.cpp to resolve CVm constructor and destructor symbols required by the original Rar3Decoder. No modifications to matcher or other algorithm files.

Fix 10 includes the three unchanged original 7-Zip LzOutWindow and PPMd implementation units as SHA-1-pinned sources; no matcher or other runtime algorithms were modified.

Fix 11 pins and compiles original, unchanged SevenZ `C/HuffEnc.c` for `Huffman_Generate`. Its prerequisite `Sort.c` was pinned and included in Fix 8.

Fix 12 pins and compiles unchanged SevenZ `CPP/7zip/Compress/BitlDecoder.cpp` for `NBitl::kReverseTable` in the 32-bit Deflate decoder, with source-lock SHA `1236bab4b666f3bcc8874d60be19630cb42af59c`.

Fix 13 adds the unchanged pinned SevenZ `C/SwapBytes.c` (Git blob `d382c327a0c142b4383234cbeed98a99b75a3293`) to the 7-Zip static archive, defining `z7_SwapBytes2`, `z7_SwapBytes4` and `z7_SwapBytesPrepare`.

Fix 14 adds pinned BZip2Crc.cpp (Git blob c1709170b53c413717db10bcdda6809417e0b571) and 82 candidate module names/blobs from the pinned Git tree (not compiled blindly).
