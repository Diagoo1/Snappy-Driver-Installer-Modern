# Fix 10 - Add original LzOutWindow and PPMd codec implementation sources

## Observed error (Windows 11, Qt 5.6.3, MinGW 4.9.2)

Fix 9 completed native compilation but stopped while linking
`SDIO-Modern-Qt.exe` with these exact unresolved symbols:

```text
CLzOutWindow::Init(bool)
NCompress::NPpmd::CEncoder::CEncoder()
vtable for NCompress::NPpmd::CDecoder
```

The original Code::Blocks project `project/7zip/lib.cbp` lists RAR1/2
decoder sources, plus PpmdRegister.cpp, but omits the source files
implementing their dependencies. The original files and expected Git
blob SHA-1 fingerprints in the pinned repository snapshot are:

| Original 7-Zip source file | Pinned Git blob SHA-1 |
| --- | --- |
| `CPP/7zip/Compress/LzOutWindow.cpp` | `213e0a69ce56e47d7501dea4f1a8bce42929196d` |
| `CPP/7zip/Compress/PpmdDecoder.cpp` | `e2399dc4b10923d8bf0b17a77557331ea6261cb1` |
| `CPP/7zip/Compress/PpmdEncoder.cpp` | `16a40d69f52f1278c65cf63971bac354718bd3ff` |

Their full on-disk paths start with `external/SevenZ/build/` in the
pinned `IDisposable/sdio-go` SDIO-baseline source tree. Fix 10 adds
these exact files to the `$linkerDependencies` array in
`tools/integration/build_native_engine.ps1`, checks their SHA-1 Git
object hashes before use, avoids duplicates, and compiles the original
unchanged C++ implementation into the existing `libsdio_7z.a` archive.

No shim, fake vtable, or algorithm change was used. The original
matching and indexing sources are untouched. Real driver installation
remains disabled in the default matching build profile.

## Apply above your existing Fix 9 project

1. Extract `SDIO_Native_LzOut_PPMd_Fix10_Hotfix.zip` directly inside
   `SDIO_Modern_Origin_Integration_Candidate`, preserving directories.
2. Allow replacement of matching files.
3. Keep `vendor`, `.source-cache`, and `build-origin-matching` intact.
4. Run `01_BUILD_ENGINE_MATCHING.cmd` again, **not** installation test.
5. If linking fails, send the first `undefined reference` plus the last
   ~40 lines from `build-origin-matching/build-native.log`.

Verification performed in this delivery: Python structural checks,
2 small C++ policy tests on Linux, ZIP integrity and manifest hashes.
Native Windows linking/running, matching parity, real installs, static
Qt one-file packaging, and XP-11 testing remain unverified.

References (source pin):
- https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build/CPP/7zip/Compress/LzOutWindow.cpp
- https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build/CPP/7zip/Compress/PpmdDecoder.cpp
- https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build/CPP/7zip/Compress/PpmdEncoder.cpp
