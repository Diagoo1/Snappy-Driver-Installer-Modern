# Fix 12 - 7-Zip Deflate bit-reversal table implementation

## Evidence from the supplied Windows 11 build log

The Fix 11 source compiled under Qt 5.6.3 / MinGW GCC 4.9.2,
but final executable linking failed with:

```text
libsdio_7z.a(unit_0054.o): undefined reference to `NBitl::kReverseTable'
```

That symbol is a 256-byte lookup table used by Deflate's bit reader.
The original pinned SevenZ 26.01 `CPP/7zip/Compress/BitlDecoder.h`
enables `Z7_BITL_USE_REVERSE_BITS_TABLE` for 32-bit Windows x86,
which requires the original `CPP/7zip/Compress/BitlDecoder.cpp`.
The latter *defines and initializes* `NBitl::kReverseTable`.

The legacy Code::Blocks project at `project/7zip/lib.cbp` does not
list `BitlDecoder.cpp` as a translation unit. Fix 12 adds the upstream
file (unchanged) to the verified `$linkerDependencies` list in
`tools/integration/build_native_engine.ps1`.

Pinned upstream source file:
`external/SevenZ/build/CPP/7zip/Compress/BitlDecoder.cpp`

Verified Git blob SHA-1: `1236bab4b666f3bcc8874d60be19630cb42af59c`.

This change does NOT introduce a synthetic bit table, change Deflate,
change SDIO's matcher or indexing, or enable driver installation.
The original SDIO source remains untouched; the source-lock validation
and source re-preparation continue to apply.

## Apply without discarding existing build/dependency caches

1. Unzip the hotfix into the existing `SDIO_Modern_Origin_Integration_Candidate`
   directory, preserving the folder paths within the archive.
2. Allow overwrite for the same-path source/configuration files only.
3. Keep `vendor`, `.source-cache`, and `build-origin-matching` intact.
4. Run `01_BUILD_ENGINE_MATCHING.cmd` again (not installation testing).
5. If an error occurs, inspect the first `undefined reference` or
   `fatal error` in `build-origin-matching/build-native.log`.

## Remaining uncertainty

This source patch has not been compiled or run with a Windows Qt SDK
in the delivery environment. Other 7-Zip units may still be absent
from the older upstream Code::Blocks manifest. The successful
Linux structural and component-policy tests do not establish WinXP
through Windows 11 compatibility, real driver matching parity,
or safe installation.

Reviewed upstream source:
https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build/CPP/7zip/Compress/BitlDecoder.cpp
