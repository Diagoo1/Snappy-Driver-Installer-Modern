# Validation for 0.21.0

Local Windows validation completed on the built static x86 executable:

- 186 interface/instance checks at both 100% and 150% display scale; independent zoom rendering probes at 75% and 125%; all 40 locale switches.
- 83 structural checks in the working project and the clean public-source staging directory. The staged source fetched and verified its own official Satoshi fonts.
- 31 offline transport checks and 95,000 independently verified loopback bytes, including persistent sharing to a second downloader and stopping the session.
- Read-only current-device scan, repeat scan, original snapshots, target-system preview and extraction.
- Original index rebuilding and text export, deliberate truncated-cache recovery and subsequent cache reload.
- Actual legacy read-only script, device list/snapshot output, parse-only validation of system-changing commands, installed-ID query and concurrent-launch protection.
- PE audit: standalone x86, no known post-XP static imports. This is a targeted loader audit, not an operating-system compatibility certificate.

No real driver installation, restore point, DISM backup or reboot was performed. Actual XP/older-Windows and 32-bit-system execution requires disposable test systems. Full translations include machine-generated text and need native review.

The GitHub source-review workflow is provided. Its first run could not be started because GitHub returned `Actions has been disabled for this user`; cloud CI is not reported as passing. Windows compilation and tests above were performed locally.

Personal hardware reports and machine screenshots are excluded from public source and release assets.
