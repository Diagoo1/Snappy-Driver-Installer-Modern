# Changelog

## 0.22.1 Beta

- Slide a shared hover surface between sidebar items and animate the selected background/rounded marker together, with finite easing and immediate retargeting. Text and icons remain directly rendered.
- Align shared highlights after compact/expanded layouts and window resizing; snap when motion is disabled and stop animation when navigation is hidden. Preserve RTL and theme support.
- Replace the previous public Beta with this cumulative release; keep one current download while preserving source history and local asset backups.

## 0.22.0 Beta — consolidated release

- Consolidate all accumulated engine integration, offline libraries, downloads, automation, localization and interface improvements into one current public Beta. Earlier release entries below remain the source history.
- Restore the driver list scrollbar only after completing the rebuilt row geometry; repeated selections no longer reset a scrolled list to the top. Disable expansion animations during internal rebuilds while keeping normal user animations.
- Inset category/device chevrons with a wider branch gutter inside the shared rounded category/hover surfaces. Retain the enlarged header, labels and icons.
- Use an inset rounded pill for the active sidebar marker and keep pressed backgrounds within the rounded navigation surface.
- Replace English/Arabic project pages with a structured feature tour, paired gallery, download choices, setup guide, compatibility scope, contributor and support links.
- Explain the absence of a stable update channel while the consolidated release remains a Beta.

## 0.21.12 Beta

- Distinguish the rounded column header with a deep gradient and bright text, separate from quiet category bands.
- Default to eligible local engine recommendations and preserve manual selections in the view. Turn Recommended view off to browse all matches; advanced and explicit legacy filters switch to custom mode. Reset filters restores the recommended preset.
- Replace the thin heading-label warning with a separate padded, rounded reinstall notice. It reflects the actual required/off restore-point preference and disappears when the selection clears.
- Add in-app reinstall instructions: use the installed-version row with a matching local package, review the plan and request the same restore-point prerequisite used for other installations. No protection settings are silently changed.

## 0.21.11 Beta

- Stage compatibility choices until Apply; add project-styled help, compact paired selectors and Restore default with an automatic scan of the current PC.
- Refresh actual Windows matching after installation or a restore-blocked session, waiting for the engine to become idle. Explain pending reboot state.
- Group devices under collapsible category headings, preserve expanded states and use more specific device/category vector icons.
- Show all native alternatives by default; applying the original visibility filters to alternatives is a separate opt-in setting.
- Match reinstalls by normalized installed version rather than requiring identical rank flags; show a selection warning and the 7z package in comparison details. Missing archives still cannot be installed from metadata alone.
- Use quiet Details tags, hand cursors, gradient progress fills and startup interface credits.
- Document the inspected DriverPack restore implementation and Windows' protection requirements without changing OS settings during validation.

## 0.21.10 Beta

- Select all follows every eligible local engine Recommended candidate independently of search/filters; clear-all removes every mark, including hidden/manual choices. A Recommended current-version package is included and retains the reinstall warning.
- Clicking a version row again toggles its radio/selection off. Choose and Selected are quiet status pills rather than prominent buttons.
- Handle double-click details against the current row after selection rebuilds, preserving the selection from before the double-click.
- Animate chevrons over a bounded 160 ms interval alongside the standard tree expansion; stop the timer when idle and respect reduced motion.
- Use the shared card surface, lightweight shadow and faded bottom accent inside driver details, replacing the old solid stripe.

## 0.21.9 Beta

- Use one shared renderer, height, type scale, border and hover treatment for the five driver-workspace commands; every command has a consistent vector icon.
- Replace the ambiguous tools gear with a wrench and use sliders for match filters.
- Paint full-row hover/keyboard selection as one rounded surface, including the expansion arrow and three-dot action area.
- Paint the table header as one continuous rounded band with subtle separators, removing native rectangular backgrounds around the corners.

## 0.21.8 Beta

- Support explicit reinstallation from a matching local same/current Origin candidate, with operation-specific consent and queue labels. Never add reinstalls through bulk selection or install from metadata alone.
- Give driver detail tabs equal full-width space and vector icons, align card typography and use regular-weight device identifiers.
- Make quiet actions and table detail buttons visible before hover, with a clearer icon-led table header.
- Add a direct GitHub releases action, explain the technician compatibility preview, and paint a rounded update activity indicator.
- Balance the developer signature text with its two-beat heart.

## 0.21.7 Beta

- Equalize the five About actions and show an icon with every label.
- Extend directly painted shadows and subtle bottom accents to settings, diagnostic, metric and comparison cards.
- Group the driver toolbar and tree in one card, clarify the header and select alternatives by clicking any row cell; choosing the installed row removes the queued upgrade.
- Paint rounded scrollbar thumbs throughout the Qt interface while retaining native scrolling behavior.
- Explain Windows restore error 1058 concisely and open editable installation preferences when returning from a failed restore; never change the restore requirement automatically.
- Keep the chosen restore/restart preferences across administrator restarts and subsequent sessions.

## 0.21.6 Beta

- Detect SDI/SDIO library layouts, select the relevant index subtree and use an isolated application cache for native scans and maintenance. Compatible SDW 0x204/0x205 indexes are copied; unsupported caches are rebuilt from available local archives.
- Initialize process COM callback security before the original engine's WMI queries, preserve restore API error codes and stop before installation when a required restore point fails. Do not override Windows' restore-point frequency setting.
- Show blocked restore operations with zero installation progress and a System Protection shortcut; remove the live-installation banner.
- Add compact About links, Ko-fi support and an icon-only bug report action. Paint subtle card shadows directly without blur effects or animation.
- Make static engine libraries explicit linker dependencies so incremental builds cannot silently retain an older engine.

## 0.21.5 Beta

- Count selected drivers independently of the restore-point task.
- Intercept non-admin installation before opening the native queue; offer an explicit elevated restart, wait for the original process to exit and require a fresh scan/confirmation.
- Redesign installation review and progress with themed cards, stage indicators and status badges; keep restore/restart summaries accurate.
- Theme multiline inputs globally and clip decorative hero artwork to rounded corners.
- Consolidate build modes and checks in one `BUILD.cmd` entry point, refresh English/Arabic project documentation and provide copy-only forum material.

## 0.21.4 Beta

- Add Report a bug to About and Tools, with a cancelable preview of minimal environment information and no automatic submission or log attachment.
- Provide a bilingual GitHub issue form for reproduction steps and expected/actual behavior.
- Mark the public test release as a GitHub prerelease; keep automatic updates on the stable channel.

## Unreleased

- Consolidate the local checkout and Git repository, remove obsolete development snapshots from the active source tree, and organize maintenance tools.
- Replace historical launchers with `BUILD.cmd` and `BUILD_STATIC_QT.cmd`; consolidate current build/architecture/validation documentation.
- Separate partial review fixtures from native dependency sources. Application behavior and the 0.21.3 executable are unchanged.

## 0.21.3

- Delegate window-state changes to Windows and avoid whole-application stylesheet resets.
- Use rounded themed text menus; correct Arabic installation-preview text and badge layout.
- Publish 68 sanitized English/Arabic, light/dark screenshots, including explicit installation simulations.

## 0.21.0–0.21.2

- Legacy scripts/CLI, index maintenance, drag/drop, independent zoom and project update channel.
- Embedded fonts/locales, RTL, developer card, comparison panels and paired heartbeat.
- Damaged-cache recovery and safer index writes. The temporary snapshot window animation introduced during development was removed in 0.21.3.

Earlier commits and release source archives preserve development history. See [GitHub Releases](https://github.com/Diagoo1/Snappy-Driver-Installer-Modern/releases).
