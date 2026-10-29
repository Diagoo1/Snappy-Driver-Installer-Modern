# Fix 9 - Complete RAR3 VM symbols in pinned original 7-Zip

## Evidence: user Windows 11, Qt 5.6.3, MinGW 4.9.2 (2026-10-28)

Fix 8 built all native library objects but linking `SDIO-Modern-Qt.exe` failed:

```text
undefined reference to `NCompress::NRar3::NVm::CVm::CVm()'
undefined reference to `NCompress::NRar3::NVm::CVm::~CVm()'
```

The upstream `project/7zip/lib.cbp` includes Rar3Decoder.cpp but not
`external/SevenZ/build/CPP/7zip/Compress/Rar3Vm.cpp`, which contains
`CVm::CVm()` and `CVm::~CVm()`. Both the source file and its original
Git blob SHA-1 are verified against the pinned mirror source commit.

| Original source file | Git blob SHA-1 | Purpose |
|---|---|---|
| `external/SevenZ/build/CPP/7zip/Compress/Rar3Vm.cpp` | `e6bfd7ea689bcd1fc07cbfee99fdd4678c16a282` | Original RAR3 decoder virtual machine implementation |

`build_native_engine.ps1` appends the unchanged, pinned 7-Zip unit to
`$linkerDependencies`, after checking the file exists, checking its exact
Git blob fingerprint, and refusing duplicates. It then rebuilds the archive
and relinks using the original `-Wl,--whole-archive` mode needed for static
decoder registration. There is NO replacement constructor/destructor, no
change to matching algorithms, and no `Z7_RARVM_VM_ENABLE` override.

## Applying Fix 9 to the existing project

1. Extract `SDIO_Native_RAR3_VM_Fix9_Hotfix.zip` DIRECTLY into the existing
   `SDIO_Modern_Origin_Integration_Candidate` directory, retaining paths.
2. Allow replacement of matching files in `tools/integration`, etc.
3. Do NOT delete `vendor`, `.source-cache`, `build-origin-matching`.
4. Run `01_BUILD_ENGINE_MATCHING.cmd`, not the installation test.
5. A warning such as `BOOST_ALL_NO_LIB redefined` does not stop linking.
   Send the first new error at the end of `build-origin-matching/build-native.log`
   if the build still fails.

**This is a source fix, not a Windows-verified EXE.** Tests in this
delivery are Linux source-wiring and native policy tests, not successful
native Windows linking, real device matching, installation, XP support, or
Qt static single-file packaging.

Reference:
https://github.com/IDisposable/sdio-go/blob/25468bdb37d46a72ab4e8bf037b4e1a28ffec70e/SDIO-baseline/external/SevenZ/build/CPP/7zip/Compress/Rar3Vm.cpp
