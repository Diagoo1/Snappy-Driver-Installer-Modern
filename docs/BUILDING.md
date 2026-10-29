# Build and test

The native Windows target uses C++11, **Qt 5.6.3 static**, **MinGW 4.9.2 x86**, Python 3 and PowerShell. Use a short checkout path, such as `C:\SDIO`. It produces one x86 executable; driver packs and indexes are external.

## Windows application

1. Install the Qt 5.6.3 MinGW SDK and Python 3. If necessary, set `QT_QMAKE`, `QT_MINGW_BIN` and `SDIO_PYTHON` to their executable/compiler paths.
2. Run `BUILD_STATIC_QT.cmd` once to build the patched static Qt SDK. This downloads pinned Qt sources and takes substantial time. If a static SDK already exists in `vendor/qt-static`, skip this step.
3. Run `BUILD.cmd` to prepare pinned dependencies and build the integrated application. Font verification/bootstrap runs first. The build never launches the executable or copies files into a driver library.

The installation-enabled executable is written to `dist-origin-install-test-static/SnappyDriverInstaller-TS.exe`. Installation still requires an explicit runtime action and administrator privileges; actual installation needs validation on a disposable system.

Options:

| Command | Purpose |
| --- | --- |
| `BUILD.cmd -MatchingOnly` | Static native matching build with installation disabled |
| `BUILD.cmd -SkipSourceDownload` | Reuse already prepared native dependency sources |
| `BUILD.cmd -ValidateOnly` | Inspect the selected build plan without downloading or compiling |

Dependencies, source fingerprints and bridge patches are in `tools/integration`. `vendor/`, `.source-cache/`, `build-*/` and `dist-*/` are local generated dependencies/outputs and are ignored by Git. They are not part of the application distribution.

## Modern-host interface review

This target uses a current Qt5 Widgets/Svg development environment and **does not link the native driver engine**:

```
python tools/integration/bootstrap_fonts.py
cmake -S . -B build-review-ui -DCMAKE_BUILD_TYPE=Release
cmake --build build-review-ui
```

The six font weights are embedded. Cairo assets are included under OFL. Standalone Satoshi files are deliberately excluded from Git; bootstrap downloads official files and verifies `assets/fonts/PROVENANCE.json`.

## Checks

For structural review on a clean checkout:

```
python tools/integration/prepare_review_sources.py
python tools/integration/bootstrap_fonts.py
python -m unittest discover -s tests -p "test_*.py"
```

The review preparer writes only to `.review/` and `.source-cache/`, keeping its partial source fixtures separate from full native dependencies in `vendor/`.

The Windows Qt interaction suite is `tests/ui/interaction_test.pro`. Build it with the static SDK in a two-level directory such as `build-tests/ui`, passing `SDIO_TORRENT_PRI=<checkout>/build-torrent-static/torrent-link.pri` after building torrent dependencies. Run at `QT_SCALE_FACTOR=1` and `1.5`. Its settings use a separate regression-test namespace and system-changing effects are mocked.

Run `ui-interaction-test.exe --publication-gallery <directory>` to generate the sanitized screenshot gallery. Installation images are explicit simulations.

Native, offline transfer and peer fixtures live in `tests/native` and `tests/online`. PE/import checks and local diagnostic utilities are in `tools/checks`. Example:

```
python tools/checks/audit_windows_pe.py path/to/SnappyDriverInstaller-TS.exe --single-exe --require-x86 --check-xp-imports
```

## إرشادات مختصرة بالعربية

البناء الأساسي: `BUILD_STATIC_QT.cmd` مرة واحدة لتجهيز Qt الثابتة، ثم `BUILD.cmd`. يمكن استخدام `-MatchingOnly` لبناء الفحص فقط. لا يشغّل البناء البرنامج ولا يثبّت تعريفات. المكتبات والكاش ومخرجات البناء المحلية غير مرفوعة إلى GitHub. راجع [التحقق وحدوده](VALIDATION.md) قبل اعتبار دعم الأنظمة القديمة أو التثبيت الفعلي مُختبرًا.
