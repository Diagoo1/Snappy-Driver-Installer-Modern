# Validation scope

Current application version: **0.23.2 Beta**. Hardware/Windows compatibility testing is ongoing. This document separates verified behavior from implementation that still needs hardware/VM testing.

| Area | Evidence |
| --- | --- |
| Interface, menus, RTL and instances | 312 regression checks at 100% and 150%; English/Arabic and light/dark visual review |
| Structural/source integration | 82 checks; critical original source fingerprints and reviewed patches |
| Current native matching | Read-only scan: 216 devices and 567 candidates on the development host |
| Original engine workflows | Repeated scan, snapshot save/load, target-system preview and extraction of six INF files |
| Index maintenance | Original rebuild, text export, deliberate damaged-cache recovery and reload |
| Legacy automation | Read-only script/device-list output, parse-only validation of effectful scripts and installed-ID queries |
| Transfers | 38 offline checks; 95,000 verified loopback bytes including a second downloader served through sharing |
| Self-update | Public release metadata/download verified against asset size, SHA-256 and PE before any replacement |
| Binary | Static x86 build; targeted audit found no known post-XP static imports |

Window state transitions use the OS window manager. The removed screenshot/opacity animation is no longer a performance claim. OS transition appearance depends on Windows settings. No Basic Display hardware benchmark is claimed.

**Not verified on disposable systems:** real driver installation, restore points, DPInst, DISM backup, reboot, execution on XP/other older Windows and actual 32-bit Windows hosts. A matching preview for XP is not execution on XP, and a successful build/import audit is not an OS compatibility certificate.

Forty locale catalogs are embedded. English and Arabic cover all collected keys. Some newly collected interface strings currently fall back to English in the other 38 locales after the translation endpoint returned HTTP 429; see LOCALIZATION_COVERAGE.json. Machine-generated translations still require native review. The gallery contains sample-data UI captures; installation/completion screenshots are simulations, not evidence of real installs.

Bug reporting opens a GitHub issue form only after a review dialog and an explicit click. The generated URL includes version, Windows version, architecture, language and theme; it does not read device inventory, paths, logs or the local support export. The local support export remains a separate feature and can contain identifying information; review it before attaching it to a public issue. Beta releases are marked as GitHub prereleases and do not replace the stable automatic-update channel.

The repository provides a source-review workflow. Cloud CI is not counted as passing: GitHub Actions was disabled for the account during release validation. Historical test logs and development snapshots are retained outside the active local project and in Git history, rather than published as repeated reports.

0.21.6: non-admin installation is intercepted before the queue begins. The elevation dialog can cancel, and the elevated app waits for the prior process to exit and starts without an installation plan. UI tests do not invoke UAC. A fresh read-only scan of the supplied SDI R2600 library, using copied indexes, enumerated 222 devices. Real elevation/installation still needs user or disposable-system validation.

0.21.6: 219 UI checks at 100%/150%; a simulated restore failure exposes System Protection and leaves installation progress at zero. Actual restore creation/installation were not invoked. Process COM callback setup returned S_OK (0x00000000) during read-only native scans. Both SDI and SDIO directory layouts, using copies of the same supplied SDI R2600 index set and the same pack library, returned 222 devices/572 candidates; source index hashes were unchanged. Unsupported SDW 0x206 input was left intact while the native engine regenerated a usable private index from a 1.5 MB local test pack and exported its contents.

Restore callback permissions follow the [Windows System Restore guidance](https://learn.microsoft.com/en-us/windows/win32/sr/using-system-restore). System Protection is never enabled automatically, restore failure never silently disables the user's requirement, and the global restore-point frequency value is left untouched. Static archive dependencies were verified in the generated app Makefile.

The final executable's `--gui-security-probe` returned `guiCom: 0` and `restoreCallbacks: 0`, with `installationRequested: false`. GUI startup initializes callback security before QApplication/Qt can establish COM defaults. This probe does not create a restore point or install a driver.

0.21.7: 221 UI checks at 100%/150%, including equal About action widths/icons and selecting an alternative from its status cell. Gallery review covers the unified table card, expanded card shadows, bottom accents and rounded scrollbars. Windows 1058 means System Restore is disabled; the program preserves the requirement and offers System Protection or review of the explicit restore option. No Windows protection settings, restore creation or installation were changed during these checks.

0.21.8: explicit reinstallation resolves the installed metadata row to an eligible, locally available same/current Origin candidate. The plan retains the native token and ordinary validation. Bulk selection skips reinstallation. Regression coverage checks the consent warning and simulated running/completed reinstall statuses; no real driver installation or restore creation was performed. Detail tabs fill the width and have vector icons, device values use regular body type, quiet actions have resting surfaces, and the update activity strip is directly painted with rounded caps.

0.21.9: 234 UI checks at 100% and 150%, including shared command dimensions/type/icon sizing and header corner pixels blending into the containing card. The five workspace commands share the same painter and state handling; header rendering bypasses native section backgrounds. No native driver/restore/reboot behavior changed or was executed.

0.21.10: 243 UI checks at 100%/150%, including global Recommended selection under a hiding search, clear-all after page navigation, row toggling, details after a selection rebuild, finite chevron animation and reduced motion. Select-all now follows all eligible local engine recommendations, including a Recommended reinstall; operation-specific installation consent remains required. Native matching and installation code did not change and no actual installation was performed.
The old static assertion requiring filter-scoped bulk selection was removed because that behavior was explicitly replaced; actual UI regression checks now cover global selection under hidden search results and page navigation.

0.21.11: 258 UI checks at 100%/150% cover category persistence, complete and opt-in filtered alternatives, archive names, reinstall version handling, specific icons, Apply/Restore default/help, gradient rendering and cursors. Fresh read-only SDI/SDIO-layout scans both returned 216 devices/567 candidates; original index hashes remained unchanged. The current library exposes 12 Thunderbolt candidates and 14 Generic Participant candidates. Automatic post-install rescan is implemented, but was not exercised after real hardware installation; install/restore/reboot and old-Windows execution remain unverified. See RESTORE_BEHAVIOR.md for the inspected DriverPack restore code.

0.21.12: the UI fixture suite additionally verifies the default recommended view, exposing all matches, custom/legacy filter switching, keeping manual choices visible, a padded reinstall notice, actual restore preference changes, and the reinstall plan carrying its restore prerequisite. Visual captures were reviewed for the column/category contrast and notice geometry. These are UI/plan checks, not evidence of a restore point or driver installation on hardware.

0.22.0 consolidates the previous project releases. The 271-check UI suite passed at 100% and 150%, including real checkbox input far down a 48-device fixture and twelve repeated selection changes without losing scroll position. List geometry is completed before restoring the scrollbar; rebuilds suppress transient expansion animation. Category and device branches use a 36px indentation, with category surfaces spanning the branch gutter. The refreshed English/Arabic gallery and static x86 import audit were reviewed. No real install, restore creation or reboot was performed. Only the consolidated Beta is published; stable automatic updates remain unavailable until a stable release is published.

The final sidebar marker is paint-only: its short inset rounded pill and rounded pressed state were rebuilt and reviewed visually in the refreshed four-mode gallery. The 271-check runs above validate the scroll/branch fixes before that final cosmetic adjustment.

0.22.1: 277 UI checks passed at both 100% and 150%, including intermediate shared hover/selection positions, rapid retargeting, immediate reduced-motion behavior and highlight alignment after compact footer/layout changes. Shared selection/marker and hover surfaces use finite property animations; item text/icons remain directly rendered. Resize/show synchronization and hide-time cancellation are implemented. The final four-mode gallery and static x86 import audit were reviewed. No new real hardware operations were performed.

0.23.0: 288 UI regression checks at 100% and 150% cover single-surface sidebar motion, repeated active-page clicks, update notices, download columns/metrics/filtering and classic Index directory resolution. The 38 offline transfer checks include cross-file verified progress and local-version availability. The loopback peer test transferred 95,000 verified bytes to an independent downloader. HTTPS metadata-only probes parsed 68 packs + 68 indexes from SDI and 109 packs + 109 indexes from SDIO at validation time. These changing catalog counts are observations, not hard-coded application values. Full public driver-pack downloads and real installation/restore/reboot were not performed. The 84-image four-mode gallery uses labeled sample data.

0.23.1: 305 UI checks at 100%/150% cover standalone catalog routing, progress-column proportions, staged source/schedule settings, startup vs timer behavior, elapsed hours, launch thresholds, retry cooldown and dated checks completing once. Opening cached metadata does not advance the successful network-check timestamp. Automatic checks run only while the app is open; no background service or OS scheduled task was installed. English/Arabic have no collected-key fallbacks. Actual multi-hour/day waits and sleep/wake behavior were not tested; date/time rules were checked with deterministic timestamps. No real driver installation/restore/reboot was performed.

0.23.2: 312 UI checks passed at 100%/150%, including selected-sidebar background persistence under an independent quiet hover, a three-row popup fitting its items, the real three-second reinstall notice timeout, refreshes not re-showing that notice, and Manual/Never notice policy. The 84-image gallery was refreshed and metric-card paint/shadows reviewed in light/dark. Settings Check now is wired to a forced catalog refresh in the Download center; live public network transfer and hardware installation remain outside these fixture checks. No new restore points, driver installations or reboots were performed.
