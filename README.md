# Snappy Driver Installer Modern

A native Windows interface by **Tarek Sadek (Diagoo1)**, built with C++11/Qt and an embedded, pinned SDIO engine. One executable; driver packs and indexes remain external.

[العربية](README_AR.md) · [Download](https://github.com/Diagoo1/Snappy-Driver-Installer-Modern/releases/latest) · [Build guide](docs/BUILDING.md) · [Validation](docs/VALIDATION.md)

## Features

- Original device matching and candidate order, alternative versions, relative match-priority stars and a dedicated installation workflow.
- Offline libraries, snapshots, extraction, index maintenance, downloads/resume and sharing while the app is open.
- Legacy scripts/CLI and JSON automation, importable JSON themes, independent zoom, diagnostics and settings.
- Forty embedded locales, Arabic RTL and embedded Cairo/Satoshi fonts; light and dark themes.
- GitHub updates with asset size, SHA-256 and PE checks. Executables are unsigned.

Download the EXE from Releases, place `drivers`/`indexes` beside it or choose their paths in Settings. Preferences use the current user's Registry; no adjacent DLLs or INI are required. Stars indicate relative matching priority, not safety.

The x86 build targets XP through 11, but actual older-Windows execution and real installation/restore/reboot still require disposable-system validation. See the [verified scope and remaining tests](docs/VALIDATION.md). Translation coverage is complete for collected keys; native language review remains necessary.

## Screenshots

[Explore all 68 screenshots](docs/GALLERY.md): English, Arabic, light and dark, including dialogs and **explicit installation simulations**. Screenshots use sample data; they do not demonstrate a real driver installation.

| English · Light | English · Dark |
| --- | --- |
| [![English light](docs/screenshots/en-light/01-drivers.png)](docs/screenshots/en-light/01-drivers.png) | [![English dark](docs/screenshots/en-dark/01-drivers.png)](docs/screenshots/en-dark/01-drivers.png) |
| العربية · فاتح | العربية · داكن |
| [![Arabic light](docs/screenshots/ar-light/01-drivers.png)](docs/screenshots/ar-light/01-drivers.png) | [![Arabic dark](docs/screenshots/ar-dark/01-drivers.png)](docs/screenshots/ar-dark/01-drivers.png) |

## Build

On Windows with Qt 5.6.3, MinGW 4.9.2 x86 and Python 3:

```
BUILD_STATIC_QT.cmd
BUILD.cmd
```

The first command prepares the patched static Qt SDK once. The second verifies fonts, prepares pinned dependencies and builds without launching. `BUILD.cmd -MatchingOnly` disables installation. Full instructions and review/test targets are in [docs/BUILDING.md](docs/BUILDING.md).

## Source layout

| Path | Contents |
| --- | --- |
| `src`, `native` | Interface, workers, original-engine bridge and validation |
| `assets`, `i18n`, `themes` | Embedded resources, translations and sample theme |
| `tools` | Build preparation, checks and release tools |
| `tests` | Structural, interface, native and transfer checks |
| `docs` | Current guides, source provenance, notices and gallery |

Build caches, vendor checkouts, private reports and executables are excluded from Git. Standalone Satoshi fonts are downloaded and fingerprint-checked by the build bootstrap; they are not committed.

[Architecture/provenance](docs/ARCHITECTURE.md) · [Engine comparison](docs/SOURCE_PARITY_AUDIT_AR.md) · [Legacy compatibility](docs/LEGACY_COMPATIBILITY_AR.md) · [Changelog](CHANGELOG.md)

## License

GPL-3.0, preserving original SDIO and third-party rights. See [LICENSE](LICENSE-GPL-3.0.txt), [dependency notices](docs/licenses) and font/asset provenance. This is an independent community interface, not an official SDI/SDIO release. Driver packs are not included.
