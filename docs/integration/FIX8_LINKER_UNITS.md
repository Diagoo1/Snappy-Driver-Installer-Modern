# Fix 8 - Linker closure for original 7-Zip sources

## Windows evidence (2026-10-28, Fix 7 build)

The original engine's C++ objects and local libraries compiled. The final
executable link was still blocked by three unresolved **original 7-Zip**
symbols:

```text
undefined reference to `HeapSort'
undefined reference to `NWindows::NFile::NSystem::MyGetDriveType(wchar_t const*)'
undefined reference to `NWorkDir::CInfo::Load()'
```

The legacy Code::Blocks `project/7zip/lib.cbp` contains references to these
functions, but does not compile the corresponding implementation files:

| Original source unit | Git blob SHA1 | Resolves |
|---|---|---|
| `external/SevenZ/build/C/Sort.c` | `0c30ca8bc323ec641272c607fb5c2c48c50e03f2` | `HeapSort` |
| `external/SevenZ/build/CPP/Windows/FileSystem.cpp` | `02faf627b10a6722441da7e9bbc03827ec2df744` | `MyGetDriveType` |
| `external/SevenZ/build/CPP/7zip/UI/Common/ZipRegistry.cpp` | `f4c27bc93feb46aea9f6f6ee363b5f091271211e` | `NWorkDir::CInfo::Load` |

`build_native_engine.ps1` now appends these three unmodified, git-blob-verified
build units to the same original archive, after the previous Fix 6/7 units.
The same source-lock file checks all three against the pinned Origin baseline
**before source patching**, and a separate check ensures they are not silently
compiled twice.

`ZipRegistry.cpp` calls the project's original Registry API, already built
through the original Code::Blocks 7-Zip project. No fake registry APIs or
replacement implementations were created.

## Install without deleting downloaded dependencies

1. Unzip the small `SDIO_Native_7Zip_Linker_Closure_Fix8_Hotfix.zip` directly
   into the EXISTING `SDIO_Modern_Origin_Integration_Candidate` project folder.
2. Allow merging and overwriting the matching source/build script files.
3. Preserve `vendor`, `.source-cache`, and `build-origin-matching`.
4. Run **only** `01_BUILD_ENGINE_MATCHING.cmd` (do not run the install test).
5. Check the end of `build-origin-matching/build-native.log` for either
   `Integrated build produced:` or the first real compiler/linker error.

The successful local source-structure and C++ policy tests DO NOT validate
Windows compilation, real hardware matching, installation, XP compatibility,
or single-file static packaging. No prebuilt executable is supplied here.

## Source references

- https://github.com/IDisposable/sdio-go/tree/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build
- https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/project/7zip/lib.cbp
