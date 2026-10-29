# Validation for 0.21.3

The custom snapshot window animation has been removed. Window-state changes delegate to the OS without raster capture, transparent live-window hiding or timer-driven geometry. Full application QSS is no longer reapplied on window-state changes. The OS determines whether and how its native transition is animated.

UI regression covers repeated maximize/restore requests, minimize/restore, full opacity and absence of snapshot overlays, and rounded text context menus in light/dark and Arabic/English, including surface color and select-all behavior. The suite has 205 checks at both 100% and 150%. Structural checks: 83. A fresh rebuilt static x86 executable receives a read-only scan and PE import audit.

The publication gallery contains 68 actual UI captures with illustrative data. All installation pictures are simulations; download pictures use a local fixture catalog. Diagnostics are sanitized before capture. No real installation, restore point, download or reboot is performed by the gallery renderer.

Forty locales retain complete collected-catalog coverage; translations still require native review. Driver installation and actual older-Windows/Basic Display performance remain separate validation needs. Engine matching and transport code are unchanged. Cloud CI is not claimed as passing.
