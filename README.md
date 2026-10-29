<div align="center">
  <img src="assets/tarek-sadek-logo.png" width="92" alt="Snappy Driver Installer Modern">
  <h1>Snappy Driver Installer Modern</h1>
  <p><strong>Original matching. A modern interface. A clearer driver workflow.</strong></p>
  <p>A community project by <strong>Tarek Sadek · Diagoo1</strong></p>
  <p>
    <img src="https://img.shields.io/badge/release-0.21.7_Beta-E9A23B" alt="Beta">
    <img src="https://img.shields.io/badge/license-GPL--3.0-00857D" alt="GPL 3">
    <img src="https://img.shields.io/badge/built_with-C%2B%2B_%2F_Qt-075266" alt="C++ and Qt">
  </p>
  <p><a href="README_AR.md">العربية</a> · <a href="https://github.com/Diagoo1/Snappy-Driver-Installer-Modern/releases/tag/v0.21.7">Download Beta</a> · <a href="docs/GALLERY.md">Screenshots</a> · <a href="https://github.com/Diagoo1/Snappy-Driver-Installer-Modern/issues/new?template=bug_report.yml">Report a bug</a></p>
</div>

## Meet the project

A native Windows interface built around an embedded, pinned **Snappy Driver Installer Origin** engine. Browse installed drivers and matching alternatives, compare versions and follow installation from one dedicated page. The application ships as **one EXE**; driver packs and indexes remain separate.

**This is a Beta.** Compatibility testing across hardware and Windows versions is ongoing. Actual installation, restore points and reboot require further disposable-system testing; this project is not an official SDI/SDIO release.

## A look inside

| English · Light | English · Dark |
| --- | --- |
| [![English light](docs/screenshots/en-light/01-drivers.png)](docs/screenshots/en-light/01-drivers.png) | [![English dark](docs/screenshots/en-dark/01-drivers.png)](docs/screenshots/en-dark/01-drivers.png) |
| العربية · فاتح | العربية · داكن |
| [![Arabic light](docs/screenshots/ar-light/01-drivers.png)](docs/screenshots/ar-light/01-drivers.png) | [![Arabic dark](docs/screenshots/ar-dark/01-drivers.png)](docs/screenshots/ar-dark/01-drivers.png) |

[Open the full gallery](docs/GALLERY.md). Captures use sample data. Installation screens are explicit simulations, not evidence of a real install.

## Tools for the whole workflow

| Area | What you can do |
| --- | --- |
| Driver matching | Scan devices, inspect installed and alternative versions, compare candidates and select a specific version |
| Installation | Review the selection and restore/restart options; reopen as administrator before installation when needed; follow per-driver results |
| Offline libraries | Choose driver/index folders, download/resume packages, extract files and maintain indexes |
| Device tools | View diagnostics, save/load snapshots and open Windows utilities |
| Your interface | Arabic RTL, English, embedded Cairo/Satoshi fonts, light/dark, zoom and importable JSON themes |
| Automation | Legacy commands/scripts and validated JSON installation plans |
| Feedback | Review minimal environment details before opening a public GitHub issue; no automatic report or log upload |

Stars indicate **relative matching priority**, not safety. Forty locales are embedded; English and Arabic cover current collected keys, while some new strings use English in other locales. Native translation review is welcome.

## Get started

1. Download the EXE or portable archive from [0.21.7 Beta](https://github.com/Diagoo1/Snappy-Driver-Installer-Modern/releases/tag/v0.21.7).
2. Place `drivers` and `indexes` beside it, or choose their locations in Settings. Packs are not included.
3. Scan, compare versions and review your selection. If administrator access is needed, the app offers to reopen; scan and confirm again afterward. No selection is installed automatically after elevation.

Settings use your Windows Registry. No adjacent DLLs or INI are required. Automatic application updates follow the stable release channel; Beta downloads use their explicit release links.

## SDI, SDIO and Windows support

The app automatically detects **SDI and SDIO library layouts**, including flat, `indexes/SDI` and `indexes/SDIO` folders. When both exist, it uses the detected distribution or the index names matching your local packs. Native scanning and maintenance use a private application cache: compatible SDW 0x204/0x205 indexes are copied, while unsupported formats are regenerated from available local archives. Native processing does not overwrite the source library's indexes.

Both layouts were tested with the same supplied SDI R2600 pack library and produced equal device/candidate counts. An unsupported-index regeneration case was also verified. The matching engine remains embedded SDIO Origin; there is no dependency on launching external SDI/SDIO executables, and every fork's index format is not guaranteed.

The x86 build targets XP–11 and includes the helper used for x64 driver workflows. Older-Windows execution, actual 32-bit hosts and broad hardware compatibility remain unverified. This is not a native ARM64 build. Read the [validation scope](docs/VALIDATION.md) and [engine provenance](docs/ARCHITECTURE.md).

## Build and contribute

Run **`BUILD.cmd`** for the interactive menu, or use:

```bat
BUILD.cmd -PrepareStaticQt
BUILD.cmd -SkipSourceDownload
BUILD.cmd -MatchingOnly
BUILD.cmd -RunTests
BUILD.cmd -Help
```

[Build guide](docs/BUILDING.md) · [Engine comparison](docs/SOURCE_PARITY_AUDIT_AR.md) · [Legacy compatibility](docs/LEGACY_COMPATIBILITY_AR.md) · [Changelog](CHANGELOG.md)

| Folder | Purpose |
| --- | --- |
| `src`, `native` | Interface, worker and engine integration |
| `assets`, `i18n`, `themes` | Resources, translations and sample theme |
| `tools` | Build, verification and release tools |
| `tests` | Maintained regressions; not application runtime files |
| `docs` | Guides, validation, gallery and legal notices |

Build outputs, caches, private reports and vendor checkouts are excluded from Git. Standalone Satoshi fonts are fetched and fingerprint-checked during bootstrap rather than redistributed in source.

## License and credits

GPL-3.0, preserving original SDIO and third-party rights. See [LICENSE](LICENSE-GPL-3.0.txt) and [dependency notices](docs/licenses). Driver matching and installation are based on SDIO contributors' work; the modern interface is developed by Tarek Sadek.

<div align="center">Made with care by <strong>Tarek Sadek · Diagoo1</strong><br><a href="https://github.com/Diagoo1">More projects</a> · <a href="https://github.com/Diagoo1/Dreams-Software-Collection">Dreams Software Collection</a></div>
