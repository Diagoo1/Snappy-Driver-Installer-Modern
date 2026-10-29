# Architecture and source provenance

| Directory | Responsibility |
| --- | --- |
| `src` | Qt interface, state/workers, matching presentation, downloads and automation |
| `native` | In-process original-engine bridge, exact selection validation and unavailable-engine stub |
| `assets`, `icons.qrc` | Embedded artwork, icons, fonts and their provenance/licenses |
| `i18n` | Forty embedded translation catalogs and locale index |
| `themes` | Modern JSON theme example and schema guidance |
| `tests` | Structural checks, Qt interactions, native plan and transfer fixtures |
| `tools/integration` | Pinned source preparation, compatibility patches and dependency builds |
| `tools/checks`, `tools/release` | Diagnostic utilities and maintainer release publishing |
| `docs` | Current guides, verification scope, notices and screenshot gallery |

The engine baseline is `IDisposable/sdio-go`, subdirectory `SDIO-baseline`, commit `25468bdb37d46a72ab4e8bf037b4e1a28ffec70e`. This is a pinned community mirror, not a claim to be the newest official SDIO checkout. The Go rewrite in that repository is excluded.

`tools/integration/source-lock.json` records critical original Git blobs and dependency pins. `prepare_sources.ps1` verifies the original sources before applying explicit, occurrence-checked changes from `patches.json` to a separate copy. The original matching algorithm is preserved. Adaptations expose candidate identities, isolate legacy GUI callbacks, validate paths/devices and make index I/O fail cleanly. Hashes establish integrity, not publisher signing.

Dependencies include Qt 5.6.3, Boost 1.79, the pinned original 7-Zip sources, WebP and libtorrent 1.2.20. Their build manifests and notices remain with the source/build tools. The static Qt patch fixes fractional-DPI caches in widget opacity effects and tree expansion; it does not disable interface animation. See [fractional-DPI notes](FRACTIONAL_DPI_AR.md).

Production engine operations are serialized through their owning worker. The UI copies original candidate identity/order and validates a selected plan before extraction/installation. Target-system and snapshot previews cannot install. UI/demo fixtures never imply installation success.

Build caches are not source: `.source-cache`, `.review`, `vendor`, `build-*` and `dist-*` are ignored. No driver packs, personal device reports or runtime INI/DLL files are committed. Mutable preferences use the user's Registry; translation and theme data required at runtime are embedded.

Original and dependency notices are in [licenses](licenses). Font-specific licenses and fingerprints are also under `assets/fonts`. Keep those notices when redistributing the application.
