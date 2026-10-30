# Driver-pack and index updates

The Drivers page shows a compact update notice. Open it to review the Download center. Configure automatic checks on Settings → Driver library updates. Checks read catalog metadata only. It does not start a package download or install a driver.

## Library and source

The detected SDI library uses the official `sdi-tool.org/releases/SDI_Update.torrent` catalog; SDIO uses `www.glenn.delahoy.com/downloads/sdio/SDIO_Update.torrent`. Classic `Drivers` and `Index/SDI` paths and SDIO `drivers` / `indexes/SDIO` paths are recognized. A manually imported catalog is not cached as an official feed. Errors are displayed without pretending that the library is current.

Scanning uses a private index cache and preserves source indexes. Explicitly selected downloads intentionally save the chosen packages/indexes in the configured library. Older archives are not deleted automatically.

## Source and schedule settings

Select Automatic, SDI or SDIO, then Apply schedule. An explicit source works without an existing offline library and overrides detection for catalog checks and the Download center. Automatic detects the configured library and falls back to SDIO when none is detected. This chooses the update catalog; it does not replace the embedded matching engine.

Choose Never, manual checks, every application launch, every 1–720 hours, every 1–100 launches, or one local date/time. Hourly checks use the last successful check per source. The date/time is stored in UTC and displayed locally; a successful check completes that date. Launch counters reset after a successful catalog check. Check now opens the Download center and requests fresh metadata using saved settings; Apply saves staged changes. Manual starts each launch with the home update notice hidden until an explicit check. Never hides the home notice even after a check and disables automatic checks; the Download center remains available for intentional use.

The schedule runs while the application is open. It installs no service or Windows scheduled task. Overdue hourly/date checks run after reopening; startup checks wait if the engine is busy. A failed scheduled interval/date check waits at least 15 minutes before retrying. The driver home page shows availability or a connection error. No package download or driver installation starts automatically.

## Choosing files

The list shows size, verified progress, offered version, highest local family version, relevance to the last native scan and availability. Version/filename/size comparison identifies updates; it is not a cryptographic audit of every existing local archive. Missing and outdated files are distinct. Use Updates only, Hide missing, search and the selection presets to control the list. Existing hidden selections remain selected and are counted in the footer.

Packages for this scan matches package families from actual native candidates. It does not guess hardware from a package name. After updating indexes, scan again to discover newly matching packages. A family match alone does not guarantee that a new pack contains a suitable driver.

## Downloading

The center reports received bytes, verified bytes, speed, estimated remaining time, peers and upload speed. Per-file progress counts verified intersections with torrent pieces, including pieces crossing file boundaries. Rates and ETA are estimates and expire when transfer samples stop. Pause preserves verified cached blocks; Resume reuses them. Space checks use the nearest existing destination parent.

Files are checked before publication. Downloads do not install drivers. Closing after saving files refreshes the native scan. Sharing remains governed by the transfer settings and only runs while the application is open. Installation still requires a separate reviewed plan and consent.

## Validation limits

Both real official metadata catalogs were parsed, offline integrity/resume cases passed, and a second loopback downloader received verified peer data. Public multi-gigabyte pack downloads, network behavior on old Windows, restore points and hardware installation still need beta testing. See [validation scope](VALIDATION.md).
