# Fix 13 - original SevenZ ByteSwap filter implementations

## Evidence from the Windows build log supplied after Fix 12

The 7-Zip static archive and original SDIO C++ core compiled, but final
application linking stopped with:

```text
libsdio_7z.a(unit_0051.o): undefined reference to `z7_SwapBytes2'
libsdio_7z.a(unit_0051.o): undefined reference to `z7_SwapBytes4'
```

Both functions (also `z7_SwapBytesPrepare`) are defined by the unmodified
pinned SevenZ 26.01 file `external/SevenZ/build/C/SwapBytes.c`, and are
called by `CPP/7zip/Compress/ByteSwap.cpp` already in the old Code::Blocks
project. The same original `project/7zip/lib.cbp` omits `SwapBytes.c`.

The fix pins `SwapBytes.c` in `tools/integration/source-lock.json` and
adds exactly one checked entry to `$linkerDependencies` inside
`tools/integration/build_native_engine.ps1`.

Pinned Git blob SHA1: `d382c327a0c142b4383234cbeed98a99b75a3293`.

The pre-existing checks abort the build if the pinned source is missing,
modified, or already present in the original project manifest. No
source-level substitute, codec modification, or changed matching logic.

## Apply as a hotfix over your present project

1. Extract the hotfix ZIP inside your existing `SDIO_Modern_Origin_Integration_Candidate` directory.
2. Allow overwrite, retaining the folder structure.
3. Preserve `vendor`, `.source-cache`, and `build-origin-matching`.
4. Run `01_BUILD_ENGINE_MATCHING.cmd` again, *not* `02_BUILD_INSTALLATION_TEST.cmd`.
5. If build still fails, send the first `undefined reference` or `fatal error` and
   final result from `build-origin-matching/build-native.log`.

## Verification scope and limitations

Local structural tests and C++ policy/component tests are not a Windows
linker test. This hotfix addresses the reported two symbols; the old 7-Zip
manifest may still omit other translation units. Real driver installation,
matching parity, a self-contained EXE, or XP-through-11 support are not
certified at this stage.

Official pinned snapshot source:
https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build/C/SwapBytes.c
