# PS5 Homebrew UI SDK Lab – console test

A separate homebrew test title for the reusable PS5 UI SDK.
It does **not** replace the 21-design Homebrew UI Lab showcase.

- **Title ID:** `PPSA99862` (SDK Lab test only)
- **Version:** `0.2.0`
- **Display:** 1080p virtual 1920×1080, native EGL and OpenGL
- **Background worker:** One pthread with an explicitly bounded 512 KiB stack
- **Image IO:** Reads the installed `sce_sys/icon0.png` (same PNG for eight demo tiles).
  Each tile uses a unique cache key to exercise the asynchronous pipeline.
- **GPU:** 24 MiB LRU budget; maximum **one texture upload attempt per frame**.
- **Logs:** The existing PS5 runtime's system log markers with prefix `[HUI SDK DEMO]`;
  periodic frame P99, RAM usage and GPU usage.
- **Controls:** X to navigate, O to return, D-pad to select, Square to request/
  reload covers, Triangle to clear GPU textures and re-upload from CPU cache,
  Options for clean exit.

The test app is independent at the **installer and entry-point level** but currently
uses the UI kit sources in the same repository and the existing native app linker.
A truly archive-only consumer that links just `libps5hui.a` remains a later milestone.

## Build

```sh
make sdk-demo
# dist/PPSA99862.zip  (the ordinary showcase remains dist/PPSA99050.zip)
```

The CI runs host tests and the normal application build before the SDK Demo build.
On the feature branch, the `sdk-demo` Actions artifact contains the separately
installable `PPSA99862.zip`.

The test does not download or scan user content, does not modify another game's
files and does not open any network port. A successful console boot alone does
not prove the asynchronous image path; press X and confirm tile images move
from DECODING to READY, then use Triangle to validate GPU invalidation/re-upload.

App art uses the repository's existing icon until a dedicated one is created.

## Console validation — October 24, 2026

**Result: PASS for the interactive SDK Lab demo on a real PS5 (user reported).**

Three PS5 screenshots supplied by the user show:

- SDK Integration Test home screen rendering as designed.
- Async Image Gallery: all eight unique cache entries show `READY`, all with
  visible PNG cover art (the same bundled icon image by design).
- Frame Diagnostics: `WORKER RUNNING`; decoded-image CPU cache
  **8192 KiB / 8 images** and GPU image cache **8192 KiB / 8 textures**.
- At the captured frame sample, **1686 frames**, rolling frame-time
  average **16.69 ms**, P99 **17.17 ms**, maximum **17.24 ms**
  (approximately 60 Hz, without evidence of major spikes in that window).
- User reports that all tested demo functions work, including navigation and
  interactive controls.

**Scope:** Visual hardware validation and the user's functional report.
Full-duration stress testing, exact texture reupload sequence logs, cross-firmware
compatibility, complete lifecycle and crash recovery remain unverified. The
`FRAMES >16.7ms` display currently counts frames against a 16.667ms internal
threshold despite its rounded label; this is not itself evidence of a stutter.
