# Validation for 0.21.1

This release changes combo presentation/disabled choices, the About developer card and driver comparison presentation. Engine matching, driver installation and transfer logic are unchanged from 0.21.0.

Validation covers the existing UI/instance regression suite plus checks for the zoom control's shared styling, dark popup inheritance, disabled-option preservation, unnecessary search omission, stable LTR version values and the selected candidate's five-star priority. Screenshot review covers English/Arabic, light/dark and 100%/150% scaling. The resulting suite contains 193 checks per scale.

All 83 structural checks pass. A fresh read-only scan and PE audit use the rebuilt static x86 executable. The collected interface catalog has 865 messages across 40 locales without missing-key English fallbacks. Machine translations still need native review.

Original snapshots, index maintenance, damaged-cache recovery, extraction and loopback transfer evidence are documented under 0.21.0; those engine/transport paths were not changed for this visual release. Actual driver installation, restore points, reboot and XP/older-Windows execution remain untested on disposable systems. Cloud CI has not been counted as passing; GitHub Actions was disabled for the account during the previous release.

Personal hardware reports and machine screenshots are excluded from public assets.
