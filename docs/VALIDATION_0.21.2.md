# Validation for 0.21.2

- 202 UI/instance checks per display scale (100% and 150%). New checks cover the signature's two peaks and rest period, phrase/heart ordering, actual maximize/restore/minimize/restore state changes, repeated actions, bounded raster size/frame count, snapshot cleanup and reduced-motion behavior.
- Visual review covers English/Arabic, light/dark, the Device tab's three cards and the developer signature. Technical values remain selectable and are not rewritten.
- 83 structural checks; fresh rebuilt static x86 read-only scan and PE import audit.
- 40 embedded languages, 867 collected interface messages, no missing-key fallbacks. Translations still need native review.

Window animation renders one software snapshot bounded to 960×640 pixels, advances at 33 ms intervals for up to five geometry steps, and targets a 165 ms transition. It does not repeatedly resize/re-layout the live interface. A machine without its vendor display driver was not available, so no Basic Display performance guarantee is claimed.

Matching, installation and transport logic are unchanged. Actual driver installation, restore points, reboot and older Windows execution still require disposable test systems. Prior original-engine/index/transfer tests remain documented under 0.21.0. Cloud CI is not reported as passing; GitHub Actions was disabled for the account at the previous validation.
