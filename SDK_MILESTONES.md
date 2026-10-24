# UI SDK Milestones

Last updated: 2026-10-24. Development branch: `feature/ui-sdk-foundation-20261024`.

## Baseline
- 21 designs, 30 themes, 99 UI components in the OpenGL showcase.
- Reusable gfx/ui/audio/core/platform directories with manual adoption instructions.
- Unit and screenshot tours exist; the October 2 console tour validated the showcase,
  not the new SDK integration.

## 2026-10-24: SDK groundwork
- [x] Add public `src/sdk/` umbrella and portable services (navigation, jobs,
      decoded RGBA CPU cache, localization dictionary, path resolver and frame monitoring).
- [x] Add SDK unit tests for stack limits/back, cancellation, threading, cache eviction,
      UTF-8 fallback, path traversal and frame statistics.
- [x] Add SDK-specific usage and limitations documentation.
- [x] Add an archive packaging target for the existing library subsystems.
- [x] Add CPU BMP decoder and explicit bounded render-thread GPU texture cache.
- [x] Vendor upstream stb_image 2.30 and decode PNG/JPEG/BMP/GIF/TGA/PNM under
      encoded-size/pixel-count caps, plus a tiny PNG regression fixture.
- [x] Track renderer-created textures and allow explicit image texture deletion.
- [x] Add build-artifact packaging and image-cache regression tests.
- [x] Add AsyncImages pipeline (bounded read callback, worker decode, CPU cache,
      capped GPU uploads per UI frame, cancel/status, LRU eviction recovery).
- [x] Add a working file-read adapter via the existing bounded core file API.
- [x] Ensure cached-cover reuploads obey pump()'s upload-attempt limit.
- [x] SDK smoke, full lint, GoogleTest and host tooling integration tests pass
      on GitHub Actions. Earlier native cross-build was blocked by HTTP 404
      downloading the original ps5-opengl SDK; the SHA-256-verified mirror
      unblocked the successful build.
- [x] Add independently mirrored, SHA-256-pinned SDK 1.0.2 fallback while
      retaining both original archive and extracted manifest verification.
- [x] PS5 cross-build and SDK archive both passed on GitHub Actions runs
      40092359437 and 40092364065; standalone SDK smoke test also passed.
- [x] User reports installing/testing the built app on a real PS5 and confirms
      that the app works (2026-10-24). This is a user-reported hardware smoke
      test, **not** comprehensive SDK feature or performance validation.
- [x] Add a separate SDK Demo native application (PPSA99862), demonstrating
      navigation, CPU/GPU caches, real PNG decoding on a PS5 pthread worker,
      bounded render-thread uploads, controller input and frame timings.
- [x] Add unit tests for SDK Demo route transitions and demo-specific packaging.
- [x] Standalone SDK Demo CI build and installer validation passed on GitHub
      Actions run 40094858417 (source commit 9e076870a52bc48e6703b048aa8e935352f503b0).
      Independent installable artifact: PPSA99862.zip, GitHub Actions artifact
      12560840719. The original PPSA99050 ZIP and SDK archive also passed.
- [x] User tested SDK Demo PPSA99862 v0.2.0 on an actual PS5 (2026-10-24),
      reported that all tested controls and screens work, and provided three
      console screenshots: Home, Gallery and Frame Diagnostics.
- [x] SDK Demo navigation, asynchronous PNG loading and CPU/GPU cache integration
      exercised by the user on PS5: all eight images shown READY, worker RUNNING,
      CPU cache 8192 KiB / 8 images and GPU cache 8192 KiB / 8 textures.
      Reported frame sample: 1686 total frames, 16.69 ms rolling average,
      p99 17.17 ms and maximum 17.24 ms (roughly 60 Hz).
- [ ] Standalone archive-only SDK consumer (current demo is a separately installed
      title, but compiles UI-kit sources in the same repository).
- [ ] Capture console logs to confirm cache invalidation and reload sequencing,
      repeat-load stress, long-running memory stability and worker shutdown; no
      console logs were attached to this validation.
- [ ] Verify console audio, suspend/resume, crash recovery, and broader real-game use.

## Production blockers, in priority order
- [ ] WebP decode, priority queue and concrete PS5 worker ownership (bounded async image pipeline implemented).
- [ ] OS thread management and graceful shutdown, completion/error result API.
- [ ] Full Unicode shaping, bidirectional text, locale formatting and accessibility.
- [ ] Native crash dump/symbolication and EGL/lifecycle recovery.
- [ ] SDK-only starter app, stable ABI, examples, CMake/PkgConfig integration.
- [ ] End-to-end TESTBUILD deploy; screenshots and manual controller/audio validation.

## Next agent instructions
Read `docs/SDK.md`, `docs/ADOPTING.md`, `docs/PERFORMANCE.md`,
`src/sdk/*.hpp`, and `.github/workflows/tooling.yml` first.
Do not replace already-working components. Keep `main` untouched until lint,
host unit tests and PS5 cross-build pass. Never claim a hardware test succeeded
without console logs. Preserve GPL-3.0-or-later and the three-line source header.
Update this file after every change.
