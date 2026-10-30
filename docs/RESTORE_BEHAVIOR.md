# System Restore behavior

The user-provided `DriverPack_UI_Reference.zip` was inspected without executing it. In `drp.js` (around lines 47980–48090), the restore module checks `DisableSR`, returns `system_restore_disabled` when it is set, writes `SystemRestorePointCreationFrequency=0` in Registry, and calls WMI `SystemRestore.CreateRestorePoint` through `wmic.exe`. Changing the creation frequency does not enable protection on a disabled Windows installation.

This project uses the documented `SRSetRestorePointW` API and initializes callback COM security before Qt/engine startup. It preserves the actual Windows result and does not report a disabled or failed restore point as created. A required restore point stops installation on failure. Users can explicitly review and turn off that requirement before rescanning/confirming a new installation plan; no Windows protection setting is silently changed.

Microsoft references: [Using System Restore](https://learn.microsoft.com/en-us/windows/win32/sr/using-system-restore), [Enable method](https://learn.microsoft.com/en-us/windows/win32/sr/enable-systemrestore). Enabling monitoring is an operating-system change. Restore creation and protection changes were not executed during this work.

Reference archive SHA-256: `60b98861dd086ece8d3cecebbdca604933a21cf4cdf06c9822f4de064c379f60`. The reference code is not redistributed here. Windows may reuse a recent restore point according to its configured creation interval; the application preserves that policy.
