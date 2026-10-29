# Snappy Driver Installer Modern

A modern native Windows interface by **Tarek Sadek (Diagoo1)** for a pinned Snappy Driver Installer Origin engine. C++11 / Qt Widgets, with embedded matching, archive support, fonts, translations and torrent transport in one executable. Driver packs and indexes remain external.

[العربية](README_AR.md) · [Downloads](https://github.com/Diagoo1/Snappy-Driver-Installer-Modern/releases) · [Engine comparison](docs/SOURCE_PARITY_AUDIT_AR.md)

## Screenshots

[Explore all 68 screenshots](docs/GALLERY.md): English, Arabic, light and dark, including dialogs and **explicit installation simulations**. Screenshots use sample data; they do not demonstrate a real driver installation.

| English · Light | English · Dark |
| --- | --- |
| [![English light](docs/screenshots/en-light/01-drivers.png)](docs/screenshots/en-light/01-drivers.png) | [![English dark](docs/screenshots/en-dark/01-drivers.png)](docs/screenshots/en-dark/01-drivers.png) |
| العربية · فاتح | العربية · داكن |
| [![Arabic light](docs/screenshots/ar-light/01-drivers.png)](docs/screenshots/ar-light/01-drivers.png) | [![Arabic dark](docs/screenshots/ar-dark/01-drivers.png)](docs/screenshots/ar-dark/01-drivers.png) |

## Features

- Original device matching and candidate ordering; alternative versions, relative five-star match priority, selection and installation workflow.
- Offline packs, original snapshots, target-system previews, extraction, index rebuilding and human-readable index export.
- Download/resume and sharing while the application is open; bandwidth and connection limits.
- Modern JSON automation plus legacy scripts and operational command-line options; [compatibility notes](docs/LEGACY_COMPATIBILITY_AR.md).
- Light/dark and importable JSON themes, independent zoom with restart, animated navigation, rounded windows, settings and diagnostics.
- Forty embedded languages, including Arabic RTL. The collected interface catalog has complete coverage; machine translations still need native review.
- GitHub updates with asset size, SHA-256 and PE verification before replacement. Releases are unsigned.

## Running

Download `SnappyDriverInstaller-TS.exe` from Releases. Put `drivers` and `indexes` beside it, or choose existing folders in Settings. Settings use the current user's Registry; no adjacent DLLs or INI file are needed. Installation requires administrator privileges and explicit confirmation.

The x86 executable targets Windows XP through 11, including 64-bit hosts. **Execution on XP and other older Windows versions, actual installation, restore points and reboot require further testing on disposable systems.** Compilation and import auditing do not establish full compatibility. Hardware validation covers read-only scanning, matching, snapshots and extraction on the development host. Stars describe relative match priority, not safety.

## Building

Production: Windows, Qt **5.6.3**, MinGW **4.9.2 32-bit**, Python 3, PowerShell and a short path such as `C:\SDIO`. Dependencies are pinned in `tools/integration/source-lock.json`. Critical original Git blobs are checked; patches are recorded and matching remains unchanged.

1. Install the Qt 5.6.3 MinGW SDK and Python. Set `QT_QMAKE` and `QT_MINGW_BIN` if discovery cannot find them.
2. Run `python tools/integration/bootstrap_fonts.py`. Satoshi is downloaded directly from Fontshare and verified; standalone font files are excluded from source distribution. Cairo and font licenses are included.
3. Run `00_BUILD_STATIC_QT.cmd` to build the patched static SDK (a substantial one-time build).
4. Run `03_BUILD_SINGLE_FILE.cmd`, which prepares pinned sources and torrent dependencies. See [static build details](docs/integration/STATIC_BUILD_AR.md).

For a modern-host **interface review build without the native engine**, install Qt5 Widgets/Svg, bootstrap fonts, then run `cmake -S . -B build` and `cmake --build build`. This target cannot validate native installation.

Structural checks: `python tools/integration/prepare_review_sources.py`, then `python -m unittest discover -s tests -p 'test_*.py'`. Windows UI and transport projects are under `tests/ui` and `tests/online`. System-changing effects are mocked in UI tests.

## License

GPL-3.0, retaining original SDIO and dependency notices. See `LICENSE-GPL-3.0.txt`, notices under `docs`, and asset/font provenance. This is an independent community interface, not an official SDI/SDIO release. Driver packs are not included.
