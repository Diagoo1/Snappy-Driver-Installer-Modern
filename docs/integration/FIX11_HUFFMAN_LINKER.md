# Fix 11 - Original SevenZ Huffman implementation added to link closure

## Observed on Windows 11 / Qt 5.6.3 / MinGW 4.9.2

The Fix 10 compilation finished, but linking the application still reported:

```text
libsdio_7z.a(unit_0063.o): undefined reference to `Huffman_Generate'
```

The referenced LZMS decoder header `CPP/7zip/Compress/LzmsDecoder.h`
calls `Huffman_Generate`, but the original 7-Zip Code::Blocks project
`project/7zip/lib.cbp` omits the defining unit `C/HuffEnc.c`.

The implementation exists **unchanged** at the reviewed Origin baseline:

`external/SevenZ/build/C/HuffEnc.c`

Pinned Git blob SHA-1: `04faf9e824fd3c1d594c4306fb83c106b6984f7c`.

The added source calls `HeapSort`, whose original implementation was
already included by Fix 8 (`external/SevenZ/build/C/Sort.c`). The only
other external calls identified in that file are standard C memory calls.

This fix adds HuffEnc.c to the SHA-pinned `$linkerDependencies` in
`tools/integration/build_native_engine.ps1`, so the original file is
compiled into `libsdio_7z.a`. The on-disk source tree is not changed.
The matcher, indexing algorithm and installation default remain unchanged.

## Apply above your existing Fix 10 folder

1. Unzip the hotfix directly into your existing
   `SDIO_Modern_Origin_Integration_Candidate` folder, replacing same-path files.
2. Do not delete `vendor`, `.source-cache` or `build-origin-matching`.
3. Run `01_BUILD_ENGINE_MATCHING.cmd` (not the installation test).
4. Only if the build completes, test app launch and read-only offline scan.
5. Do not try driver installation or one-file packaging yet.

Evidence: Windows log pasted on 2026-10-28, lines showing the missing symbol;
source existence, implementation name, and pin reviewed against
`IDisposable/sdio-go` at
`25468bdb37d46a72ab4e8bf037b4e1a28ffec70e`.

This delivery is **not** Windows-compiled, run, or installation-validated.
Python structural tests and local C++ policy tests do not substitute for
Windows execution and driver match parity checks.

Upstream snapshot:
https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build/C/HuffEnc.c
