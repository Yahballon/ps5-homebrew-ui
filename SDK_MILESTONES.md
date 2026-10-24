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
- [x] Real PS5 hardware smoke test for PPSA99863 v0.4.0 (user screenshots,
      2026-10-24): home, gallery, and frame telemetry render correctly.
      All 8 image entries READY, CPU and GPU cache 8192 KiB / 8 each,
      worker RUNNING, 8 background jobs completed.
      Sample at frame 927: average 16.68 ms, P99 17.01 ms,
      max 17.11 ms (~60 FPS); focus losses/returns 0/0.
- [ ] Dedicated PS5 tests for Triangle GPU invalidation/reload, Square fresh
      PNG reads, focus-loss/return, Options clean exit, repeated long-running
      cache stress and OS suspend/resume; screenshots do not establish these.
- [ ] Add true OS suspend/resume hooks, re-create lost EGL resources,
      and native crash stack traces; these are not yet implemented.

## 2026-10-24: PS5 Consumer v0.5.0 cache stress and diagnostics

- [x] User reports repeated Triangle GPU clears and Square PNG reloads
      work correctly on PS5 with the previous v0.4.0 Consumer.
      No console logs or long-duration data were supplied.
- [x] Add `AsyncImages::reset()` to safely cancel/discard stale in-flight
      image results before a fresh generation of requests; leave shared
      CPU/GPU resource ownership with their caches.
- [x] Add cumulative per-frame-safe image diagnostic counters for upload
      success, decode errors, GPU upload errors and cancellation.
- [x] Add reusable, bounded `sdk::ImageStress` state machine and tests
      for finite rounds, failure, timeout and cancellation.
- [x] Add R1-triggered 20-round independent PS5 PNG + GPU cache stress
      test to archive-only PPSA99863, with progress, counters and a
      single completion/failure log. Update title to v0.5.0.
- [x] GitHub Actions run 40100047639 at commit c1865f0a5192861de686fcf3131ceca6bc46a6d3:
      lint, new stress regression tests, full host tests, ordinary PS5 title,
      SDK static archive, original SDK Lab, archive-only PPSA99863 v0.5.0
      compiler/linker and ZIP source-whitelist validation all PASS.
      SDK Consumer installer artifact: 12562437870.
- [x] Real PS5 v0.5.0 stress test (user-provided console screenshots,
      2026-10-24): STRESS 20/20 PASS, all eight covers READY, 160 GPU
      uploads, decode errors 0, GPU errors 0 and worker RUNNING with 168
      completed background jobs. CPU cache 8192 KiB / 8 images; GPU cache
      8192 KiB / 8 textures. Captured frame 691: average 16.68 ms,
      P99 17.18 ms, maximum 17.30 ms (~60 FPS).
      This confirms a complete automated 20-wave stress run on real PS5
      hardware, not extended soak testing or overall memory stability.
- [ ] Long-duration soak test for memory growth, structured crash logs,
      suspend/resume, EGL loss/recovery, and clean worker shutdown.
- [ ] Add native OS suspend/resume callbacks, real EGL recovery and
      safe crash report capture (none of these are proven here).

## 2026-10-24: SDK Consumer v0.6.0 long-run validation

- [x] User confirmed v0.5.0 20/20 PASS, 160 uploads, 0 decode/GPU errors
      and ~60 FPS on real PS5. Original hardware screenshots documented above.
- [x] Add allocation-free `sdk/soak_monitor.hpp` with tests for observed
      cache peak bytes, pending-job peaks, elapsed time, worst frame and frames
      exceeding 33.334 ms.
- [x] Add **R2: 200 cache cycles** to PPSA99863 v0.6.0; preserve R1:
      20 cycles and all previous controls. Run uses the archive-linked SDK.
- [x] Add soak telemetry (CPU/GPU *cache* peaks and slow-frame count),
      throttle long-run progress logging to once per 20 waves and add
      explicit shutdown-before/join-after log lines.
- [x] GitHub Actions run 40105545011 on commit a7a86470fc4becd41690d672539fb30e8b653cb2:
      lint, GoogleTest, host integration, normal PS5 app, static SDK archive,
      previous SDK Lab, archive-only PPSA99863 v0.6.0 PS5 cross-build/link,
      source whitelist and ZIP verification all PASS.
      v0.6.0 Consumer install artifact: 12564004961.
- [x] Real PS5 PPSA99863 v0.6.0 user-reported 200-round R2 stress PASS
      (screenshot, 2026-10-24): 200/200 PASS, 1600 uploads, 0 decode errors,
      0 GPU errors, 0 frames >33.334 ms during tracked test, 1768 background
      jobs, worker RUNNING. Screen captured at frame 3199 with average 16.68 ms,
      P99 17.18 ms, rolling max 17.22 ms; soak monitor worst 17.65 ms.
      Current CPU/GPU cache 8192 KiB / 8 entries each.
- [x] Fix 200-wave final-frame telemetry sampling (v0.6.1): defer soak
      finish/report until after the final frame's cache/frame-time sample.
      Add regression test asserting all eight 1024-KiB covers produce an
      8192-KiB peak instead of the previously reported 7168 KiB.
      Real console confirmation of v0.6.1 is still required.
- [ ] Further memory/lifecycle proof still needed: a 200-wave cache test
      is not a process heap or system VRAM leak test.
- [ ] OS-level suspend/resume and lost-EGL restoration, process memory
      instrumentation, long-duration soak and native crash dumps remain open.

## 2026-10-24: v0.6.1 peak telemetry and shutdown verification

- [x] Correct v0.6.0 final-frame omission for cache-peak monitoring and
      worst frame metrics without adding GPU calls, blocking IO or
      recording duplicate frames.
- [x] Make WorkerThread::stop() report the number of unstarted cancelled
      jobs, whether pthread_join succeeded, and processed job count after
      join; include controlled shutdown and EGL swap failure logs.
- [x] Add host regression coverage for final-sample cache peaks and
      shutdown reports; bump PPSA99863 content version to v0.6.1.
- [x] CI run 40106797524 on commit 7dd9c5e88e25310ab67be4902447ebf6e9d63e07:
      clang-format lint, GoogleTest, host integration tests, native PS5
      showcase, reusable SDK archive, previous SDK Lab and archive-only
      PPSA99863 v0.6.1 link/ZIP checks all PASS.
      Consumer install artifact: 12564858169.
- [ ] Hardware retest: R2 200/200 PASS, 1600 uploads, errors 0,
      **peak CPU/GPU 8192 KiB** and safe Options shutdown.
- [ ] Real OS suspend/resume, lost EGL context restore and native crash
      dumps remain separate v1.0 tasks.

## Production blockers, in priority order
- [ ] WebP decoding, priority queue and production multi-worker scheduling
      (owned pthread worker, bounded image pipeline and stress harness exist).
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
