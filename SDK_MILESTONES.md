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
- [x] Add a true archive-only SDK consumer example and dedicated PPSA99863
      build path. Compile only consumer main and minimal PS5 runtime glue; link
      public headers and libps5hui.a, failing on unresolved SDK symbols.
- [x] Verified archive-only SDK consumer cross-build and native ELF linking,
      source whitelist, folder ZIP structure and 0777 permissions on CI run
      40096468528 at commit 14d02c4c1e9ab852025a22256375f87fa54badd4.
      SDK consumer artifact: 12560763851 (PPSA99863.zip).
- [x] User tested PPSA99863 v0.3.0 on actual PS5 (2026-10-24), reported
      that all tested functions work, and supplied screenshots of home, gallery
      and frame-telemetry screens.
- [x] Confirmed visually: 8/8 PNG covers READY, worker RUNNING, both RAM
      and GPU caches 8192 KiB with 8 entries each. Telemetry at 1198 frames:
      average 16.68 ms, P99 17.01 ms, maximum 17.03 ms (about 60 FPS).
      Together with the successful archive-only CI linking test, this verifies
      a genuine external consumer can run the static SDK on PS5 hardware.
- [ ] Independently capture console-log evidence for repeated GPU-clear/reload,
      fresh file reads, clean Options shutdown, and suspend/resume behavior.
- [ ] Capture console logs to confirm cache invalidation and reload sequencing,
      repeat-load stress, long-running memory stability and worker shutdown; no
      console logs were attached to this validation.
- [ ] Verify console audio, suspend/resume, crash recovery, and broader real-game use.

## 2026-10-24: SDK stability foundation, v0.4.0 consumer

- [x] Add `sdk/worker_thread.hpp`: reusable owned pthread worker,
      configurable stack/idle callback, shutdown join, restart and job count.
- [x] Add `JobQueue::cancel_queued()`: acknowledge pending jobs as
      cancelled/finished when a worker shuts down (separate from cancelling
      currently running, cooperative work).
- [x] Add `sdk/lifecycle.hpp`: input-interception/return and EGL swap
      failure status and counters. Input interception is NOT an OS suspend event.
- [x] Add `sdk/breadcrumbs.hpp`: fixed-size, allocation-free diagnostic ring
      and logging of events on a controlled EGL failure.
- [x] Add host regression tests for worker startup/shutdown/restart, queued
      cancellation, lifecycle transitions and breadcrumb ring wraparound.
- [x] Migrate archive-only PPSA99863 consumer to the reusable worker and
      diagnostics; bump its app version to v0.4.0.
- [x] CI run 40098174696 at commit c066c3107f513f7ab48c1987394b0c2de48218e8:
      lint, host unit/integration tests, normal app, SDK archive, previous SDK
      Lab and archive-only PPSA99863 v0.4.0 cross-build/link and ZIP checks PASS.
      Installable SDK Consumer artifact: 12561971440 (PPSA99863.zip).
- [ ] Verify v0.4.0 on PS5 hardware, especially cache reloading,
      focus-interception counters, worker job count and clean exit.
- [ ] Add true OS suspend/resume hooks, re-create lost EGL resources,
      and native crash stack traces; these are not yet implemented.

## Production blockers, in priority order
- [ ] WebP decode, priority queue and concrete PS5 worker ownership (bounded async image pipeline implemented).
- [ ] Full OS lifecycle/suspend/resume and structured job errors (basic owned
      pthread with cooperative stop and pending cancellation now implemented).
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
