# Validation scope

Current application version: **0.21.5 Beta**. Hardware/Windows compatibility testing is ongoing. This document separates verified behavior from implementation that still needs hardware/VM testing.

| Area | Evidence |
| --- | --- |
| Interface, menus, RTL and instances | 212 regression checks at 100% and 150%; English/Arabic and light/dark visual review |
| Structural/source integration | 83 checks; critical original source fingerprints and reviewed patches |
| Current native matching | Read-only scan: 222 devices and 572 candidates on the development host |
| Original engine workflows | Repeated scan, snapshot save/load, target-system preview and extraction of six INF files |
| Index maintenance | Original rebuild, text export, deliberate damaged-cache recovery and reload |
| Legacy automation | Read-only script/device-list output, parse-only validation of effectful scripts and installed-ID queries |
| Transfers | 31 offline checks; 95,000 verified loopback bytes including a second downloader served through sharing |
| Self-update | Public release metadata/download verified against asset size, SHA-256 and PE before any replacement |
| Binary | Static x86 build; targeted audit found no known post-XP static imports |

Window state transitions use the OS window manager. The removed screenshot/opacity animation is no longer a performance claim. OS transition appearance depends on Windows settings. No Basic Display hardware benchmark is claimed.

**Not verified on disposable systems:** real driver installation, restore points, DPInst, DISM backup, reboot, execution on XP/other older Windows and actual 32-bit Windows hosts. A matching preview for XP is not execution on XP, and a successful build/import audit is not an OS compatibility certificate.

Forty locale catalogs are embedded. English and Arabic cover all collected keys. Some newly collected interface strings currently fall back to English in the other 38 locales after the translation endpoint returned HTTP 429; see LOCALIZATION_COVERAGE.json. Machine-generated translations still require native review. The gallery contains sample-data UI captures; installation/completion screenshots are simulations, not evidence of real installs.

Bug reporting opens a GitHub issue form only after a review dialog and an explicit click. The generated URL includes version, Windows version, architecture, language and theme; it does not read device inventory, paths, logs or the local support export. The local support export remains a separate feature and can contain identifying information; review it before attaching it to a public issue. Beta releases are marked as GitHub prereleases and do not replace the stable automatic-update channel.

The repository provides a source-review workflow. Cloud CI is not counted as passing: GitHub Actions was disabled for the account during release validation. Historical test logs and development snapshots are retained outside the active local project and in Git history, rather than published as repeated reports.

0.21.5: non-admin installation is intercepted before the queue begins. The elevation dialog can cancel, and the elevated app waits for the prior process to exit and starts without an installation plan. UI tests do not invoke UAC. A fresh read-only scan of the supplied SDI R2600 library, using copied indexes, enumerated 222 devices. Real elevation/installation still needs user or disposable-system validation.
