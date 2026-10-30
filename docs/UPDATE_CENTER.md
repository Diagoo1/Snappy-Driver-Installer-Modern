# Driver-pack and index updates

The Drivers page shows a compact update notice. Open it to review the Download center. The optional startup check reads catalog metadata only; disable it on the Driver packages page. It does not start a package download or install a driver.

## Library and source

The detected SDI library uses the official `sdi-tool.org/releases/SDI_Update.torrent` catalog; SDIO uses `www.glenn.delahoy.com/downloads/sdio/SDIO_Update.torrent`. Classic `Drivers` and `Index/SDI` paths and SDIO `drivers` / `indexes/SDIO` paths are recognized. A manually imported catalog is not cached as an official feed. Errors are displayed without pretending that the library is current.

Scanning uses a private index cache and preserves source indexes. Explicitly selected downloads intentionally save the chosen packages/indexes in the configured library. Older archives are not deleted automatically.

## Choosing files

The list shows size, verified progress, offered version, highest local family version, relevance to the last native scan and availability. Version/filename/size comparison identifies updates; it is not a cryptographic audit of every existing local archive. Missing and outdated files are distinct. Use Updates only, Hide missing, search and the selection presets to control the list. Existing hidden selections remain selected and are counted in the footer.

Packages for this scan matches package families from actual native candidates. It does not guess hardware from a package name. After updating indexes, scan again to discover newly matching packages. A family match alone does not guarantee that a new pack contains a suitable driver.

## Downloading

The center reports received bytes, verified bytes, speed, estimated remaining time, peers and upload speed. Per-file progress counts verified intersections with torrent pieces, including pieces crossing file boundaries. Rates and ETA are estimates and expire when transfer samples stop. Pause preserves verified cached blocks; Resume reuses them. Space checks use the nearest existing destination parent.

Files are checked before publication. Downloads do not install drivers. Closing after saving files refreshes the native scan. Sharing remains governed by the transfer settings and only runs while the application is open. Installation still requires a separate reviewed plan and consent.

## Validation limits

Both real official metadata catalogs were parsed, offline integrity/resume cases passed, and a second loopback downloader received verified peer data. Public multi-gigabyte pack downloads, network behavior on old Windows, restore points and hardware installation still need beta testing. See [validation scope](VALIDATION.md).
