# PS5 Homebrew UI SDK — archive-only consumer

This is the real independent SDK linking test.

- **Title:** Homebrew UI SDK Consumer
- **Title ID:** `PPSA99863`
- **Version:** `v0.4.0` (v0.3.0 remains the verified earlier hardware baseline)
- **Library:** `dist/sdk/lib/libps5hui.a` (produced by `make sdk`)
- **Public headers:** `dist/sdk/include`
- **Compiled application sources:** `examples/sdk-consumer/main.cpp`,
  `src/runtime/app_heap.c` and `src/runtime/runtime_shims.c` only.
  The two runtime C files are title boot/heap glue, **not UI implementations**.
- **OpenGL:** links the verified ps5-opengl public static SDK normally.
- **Installer:** `dist/PPSA99863.zip`

```sh
make sdk-consumer
```

This first runs the unchanged original app build, creates the reusable SDK
archive and finally compiles the three explicit consumer/glue sources. It
then links `libps5hui.a`, OpenGL and the PS5 system stubs. If the archive
omits an implementation, the final ELF link **fails**. It does not silently
fall back to `src/gfx/*.cpp`, `src/core/*.cpp` or any other UI implementations.

The CI workflow verifies the source list in `build/app.ninja` and uploads
a separate `-sdk-consumer` installer artifact after building the original
PPSA99050 and the standalone-but-source-linked PPSA99862 SDK Lab.

## Test on console

1. Install **PPSA99863** without uninstalling the older test titles.
2. Open and confirm the *ARCHIVE CONSUMER* page renders.
3. Press **X** to load all eight PNG covers. Check every entry reaches READY.
4. Press **Triangle** to clear the GPU and reupload the cached textures.
5. Press **X** for frame-time, worker, RAM and VRAM diagnostics.
6. **O** returns; **Options** closes the app cleanly.

The consumer produces lines under the `[HUI ARCHIVE CONSUMER]` prefix
in the app's runtime log.

## Boundaries

The library is currently linked statically against the source-version-matched
package. This validates archive packaging and symbol resolution; it is **not**
a promise of a stable binary ABI across future SDK versions. Real console
testing and lifecycle/stress measurements are separate milestones.

## Verified build receipt (2026-10-24)

The full PS5 cross-build and archive-only link passed on
[GitHub Actions run 40096468528](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40096468528)
at source commit `14d02c4c1e9ab852025a22256375f87fa54badd4`.

- CI compiled only `examples/sdk-consumer/main.cpp` and the two
  `src/runtime/` C bootstrap sources for the independent app.
- All non-runtime UI symbols resolved from `dist/sdk/lib/libps5hui.a`.
- CI validated the `PPSA99863.zip` native app folder, signed
  `eboot.bin`, runtime, and required ZIP permissions.
- [Download archive-only consumer installer artifact](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40096468528/artifacts/12560763851).

This CI result is **not** a PS5 console run. Hardware verification remains
required, including GPU cache-clear/reupload and fresh file read behavior.

## PS5 console validation — 2026-10-24

**Hardware smoke test: PASSED (user tested on a modified PS5).**

The user supplied three actual console screenshots showing the independent
archive-linked consumer on its Home, PNG Gallery, and Frame Telemetry screens,
and reported that all tested functions work.

| Captured measurement | Result |
| --- | --- |
| Eight PNG gallery tiles | All 8/8 `READY` with image previews |
| Background worker | `RUNNING` |
| CPU image cache | 8192 KiB / 8 images |
| GPU texture cache | 8192 KiB / 8 textures |
| Captured frame number | 1198 |
| Average frame time | 16.68 ms |
| P99 frame time | 17.01 ms |
| Max frame time | 17.03 ms |

This verifies a **real PS5 successfully boots and runs UI rendering,
navigation and PNG/texture caching linked from `libps5hui.a`**, which is
independently verified as the app's only UI implementation by the GitHub
Actions source whitelist and linker step. Frame statistics are a limited
rolling capture (~60 Hz), not long-duration performance certification.

Console logs were not provided; specific cache invalidation sequencing,
fresh file reads, clean exit, error recovery and suspend/resume require
separate evidence. The library has no stable binary ABI guarantee yet.

## v0.4.0 validation focus and limitations

- The test app now uses the reusable `sdk::WorkerThread` rather than a
  locally duplicated pthread class, and reports **processed background jobs**
  in frame telemetry.
- `sdk::LifecycleMonitor` tracks input interception/focus-return counts.
  These are not real system suspend/resume events.
- `sdk::Breadcrumbs` keeps a bounded 32-event log, emitted only if the
  EGL swap operation fails (controlled failure path; no native crash dump).
- `JobQueue::cancel_queued()` marks cancelled-but-unstarted work `finished`
  when shutting down. Always cancel long-running jobs before worker.stop().
- Hardware launch/gallery/telemetry now validated as described below.
  Cache invalidation event order, clean exit and full system suspend/resume
  are still **not** verified by screenshots.

## v0.4.0 GitHub CI result (2026-10-24)

GitHub Actions run [40098174696](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40098174696)
**PASS** at commit `c066c3107f513f7ab48c1987394b0c2de48218e8`:
format/lint, host unit + integration tests, original PS5 app, SDK archive,
SDK Lab demo, archive-only consumer PS5 cross-link and ZIP structure/permissions.

[Download v0.4.0 PPSA99863 installer (artifact 12561971440)](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40098174696/artifacts/12561971440).

### Real PS5 smoke test: PASS (2026-10-24)

The user supplied three console screenshots showing v0.4.0 on a real PS5:

| Observed on screenshot | Result |
| --- | --- |
| Screen navigation | Home, Gallery and Telemetry displayed |
| PNG covers | 8 of 8 visible, all READY |
| Worker | RUNNING |
| Processed background jobs | 8 |
| CPU image cache | 8192 KiB, 8 images |
| GPU texture cache | 8192 KiB, 8 textures |
| Telemetry sample | 927 total frames |
| Average / P99 / maximum | 16.68 / 17.01 / 17.11 ms |
| Focus losses / returns | 0 / 0 |

This confirms that the archive-linked library, new worker wrapper and
telemetry screen operate on actual PS5 hardware without an observed regression
in the screenshot sample (~60 FPS). The focus counter was **not** exercised
by a focus-loss event. These screenshots do **not** yet verify Triangle GPU
cache invalidation, Square fresh decoding, Options clean exit, long-run memory
stability or OS suspend/resume. Those require event logs or dedicated tests.

## v0.5.0 cache stress testing (pending PS5 validation)

This SDK consumer version adds:

- **R1** in the gallery: run a **20-round cache stress test** (press R1
  again to stop). Each round discards the previous image request generation,
  releases GPU textures on the render thread, clears decoded CPU images,
  reads/decodes eight PNG covers via the worker, and uploads at most one
  texture per rendered frame.
- The gallery shows `STRESS X/20 RUNNING` then `PASS` or `FAIL`.
- The frame telemetry adds cumulative successful texture uploads and
  separate PNG-decode and GPU-upload error counters.
- **Square** restarts all eight covers from disk. **Triangle** clears GPU
  textures and reuploads decoded covers already in RAM. Both actions are
  designed to work even when other loads are still pending.
- Every round is time-limited to 15 seconds. Failure and final success
  produce a single `[HUI ARCHIVE CONSUMER] stress` log record.
- Read `SDK_MILESTONES.md` before claiming v0.5.0 ready: the previous
  hardware test proved v0.4.0 only. It also verified repeated manual
  Square and Triangle tests by user report (without logs).

## v0.5.0 CI validation (2026-10-24)

All checks passed on [Actions run 40100047639](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40100047639)
at commit `c1865f0a5192861de686fcf3131ceca6bc46a6d3`:
clang-format/lint, host GoogleTest suite including stale-job reset and
bounded stress state machine, host integration tests, PS5 cross-compiler/linker,
archive-only source whitelist, consumer installer ZIP permissions.

**Install PPSA99863 v0.5.0:**
[SDK Consumer ZIP artifact 12562437870](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40100047639/artifacts/12562437870).

### Real PS5 v0.5.0 stress test: PASS (2026-10-24)

User-provided console screenshots show the Home, Gallery and Frame Telemetry
screens running from the archive-linked PPSA99863 v0.5.0 application.

| Test or counter | Observed on console |
| --- | --- |
| Automated 20-cycle R1 cache stress | **STRESS 20/20 PASS** |
| PNG gallery | 8 of 8 visible, all READY |
| Successful image uploads | 160 |
| Decode failures | 0 |
| GPU upload failures | 0 |
| Completed background jobs | 168 |
| Background worker | RUNNING |
| Decoded-image CPU cache | 8192 KiB / 8 images |
| GPU image cache | 8192 KiB / 8 textures |
| Frame count at screenshot | 691 |
| Average frame time | 16.68 ms |
| P99 frame time | 17.18 ms |
| Maximum frame time | 17.30 ms |

This **confirms the bounded 20-wave re-read/decode/upload loop works on an
actual PS5**, with no reported decoding or upload errors. The observed
rolling frame times are approximately 60 FPS and do not show a major stall
in the displayed window.

**Not yet demonstrated:** sustained multi-hour operation, heap/VRAM leak
measurements across multiple sessions, focus-loss and OS suspend/resume
recovery, native crash dumps, explicit EGL context reconstruction, and
controlled Options shutdown with logs. Those remain SDK v1.0 tasks.


## v0.6.0 — long-run test (hardware validation pending)

- Title remains **PPSA99863**, version **v0.6.0**.
- **R1** in gallery: 20 cache/PNG decode cycles (unchanged).
- **R2** in gallery: **200** cycles, eight images per wave, with the same
  per-wave 15-second timeout and maximum one GPU upload per frame.
- Gallery shows `STRESS 200/200 PASS` on success. Press R2 again during a
  running stress test to cancel it.
- Telemetry adds observed **peak decoded-image CPU-cache KiB**, **peak
  GPU-cache KiB**, number of frames exceeding 33.334 ms, and worst frame time.
  These cache counters **do not measure the process heap or total VRAM**.
- Shutdown logs now record pending work before worker.stop() and job state
  after the worker is joined. These logs are emitted once, not per frame.
- Progress logs are throttled to one event per **20 cycles** to reduce
  the risk of console I/O stalls.
- A successful GitHub Actions build is **not** a real PS5 200-wave stress
  result. The v0.5.0 PS5 screenshots validate the previous version only.

### Suggested v0.6.0 hardware validation

Open the gallery, press R2 and let 200/200 finish. Use X to open
telemetry, confirm upload count **1600**, both error counts zero, and note
slow-frame count, worst frame time, and CPU/GPU cache peaks. Screenshots
plus runtime logs are useful for tracking longer-term reliability.

## v0.6.0 CI receipt — 2026-10-24

[GitHub Actions run 40105545011](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40105545011)
passed lint, GoogleTest including `sdk/soak_monitor.hpp`, host integration
tests, native PS5 cross-build, static SDK archive creation, and the
archive-only consumer compile/link/installer ZIP verification.

- **Consumer installer:** [PPSA99863.zip — artifact 12564004961](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40105545011/artifacts/12564004961)
- Build commit: `a7a86470fc4becd41690d672539fb30e8b653cb2`
- **Hardware state:** R2/200-wave PS5 stress validation PASSED on
  2026-10-24, user-provided telemetry screenshot documented below.
- **Observed:** `STRESS 200/200 IDLE / PASS`, 1600 uploads, decode
  errors 0, GPU errors 0; 8 CPU images and 8 GPU textures in the caches.

### Real PS5 v0.6.0 R2 long-run result (2026-10-24)

| Console measurement | Observed |
| --- | --- |
| R2 automated cache stress | **200/200 PASS** |
| Successful image uploads | **1600** |
| Decode / GPU errors | **0 / 0** |
| Worker / completed background jobs | RUNNING / 1768 |
| Current decoded-image / GPU caches | 8192 / 8192 KiB (8 entries each) |
| Soak frames over 33.334 ms | **0** |
| Soak worst frame | 17.65 ms |
| Captured total frame count | 3199 |
| Rolling average / P99 / maximum | 16.68 / 17.18 / 17.22 ms |
| Soak reported cache peaks | 7168 / 7168 KiB |
| Controller focus losses / returns | 0 / 0 |

**Known diagnostic sampling issue:** 7168 KiB *peak* appears smaller than
8192 KiB *current* usage. The on-console logic ends the soak on the frame
that completes the last round before its later per-frame monitor sample,
which can omit the last frame's eight-texture peak. This is a measurement
artifact requiring a code/test fix; it does not indicate missing images or
memory leakage.

The 200-round outcome verifies repeated PNG file reads, decoding, texture
uploads, and bounded-cache operation on the tested PS5. This is not a
multi-hour endurance test, a full-process heap measurement, an OS
suspend/resume test, or verified crash recovery. The screenshot does not
include the end-of-test elapsed time.

## v0.6.1 peak-sample and worker-shutdown fix (hardware test pending)

The SDK consumer v0.6.1 fixes the off-by-one-frame peak-cache metric
observed in the user's v0.6.0 PS5 200-wave screenshot. The final stress
outcome is now staged until after a successful display swap and the
final `SoakMonitor::observe()` frame/cache sample, then the test closes
and reports its peaks. Thus the final eighth GPU texture is included in
the peak CPU/GPU cache counts.

`WorkerThread::stop()` now returns `StopReport` with:
- `cancelled_queued`: work that never started and was acknowledged.
- `joined`: whether `pthread_join` completed for a running thread.
- `processed_after_join`: cumulative work completed at join.

The standalone consumer emits this report on Options shutdown and on its
controlled EGL swap-failure path. This does not add crash stack traces or
an OS-level suspend/resume adapter. A clean shutdown report must still
be observed on the PS5 to verify the target-platform behavior.

On console, launch PPSA99863 v0.6.1, press R2 in Gallery and look for
`STRESS 200/200 PASS`, 1600 uploads, zero errors and cache peaks of at
least **8192 KiB**. Capture telemetry. Options tests shutdown; an actual
worker-joined log is the only proof of that final shutdown path.

## v0.6.1 CI receipt — 2026-10-24

The production-style cross-build was successful on
[GitHub Actions run 40106797524](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40106797524),
source commit `7dd9c5e88e25310ab67be4902447ebf6e9d63e07`.

Passed checks: source formatting/lint, host GoogleTest (including final
cache-sampling and worker-stop report regressions), host integration tests,
PS5 cross-build, reusable SDK static archive, original SDK Lab and
independent archive-only consumer linker/source whitelist/ZIP verification.

[Download PPSA99863 v0.6.1 — artifact 12564858169](https://github.com/Yahballon/ps5-homebrew-ui/actions/runs/40106797524/artifacts/12564858169).

**User reports that v0.6.1 works on their PS5**, but no v0.6.1 diagnostic
screenshot or console log has been supplied yet. To verify the corrected
measurements, expect R2
`STRESS 200/200 PASS`, `Uploads 1600`, 0 decode/GPU errors, and
`Peak cache CPU/GPU 8192/8192 KiB` after fixing the terminal-frame
sampling error. The Options shutdown report is visible in runtime logs,
not the GUI; do not claim successful thread join without logs or equivalent
instrumentation.

## Where to find the logs on your PS5

The archive-only consumer links the native `runtime_shims.c` which redirects
stdout and stderr to **`/download0/hui/dev/app.log`**, *not* `/data`.
At the beginning of the next launch, the existing `app.log` is renamed to
`/download0/hui/dev/app.prev.log`. The source also mirrors sys::log
messages to PS5 kernel debug output.

With a PS5 FTP server running, **leave PPSA99863 open**, then browse to:

```text
/mnt/sandbox/PPSA99863_000/download0/hui/dev/app.log
/mnt/sandbox/PPSA99863_000/download0/hui/dev/app.prev.log
```

The sandbox FTP path is exposed only while the title runs in the documented
test setup. Copy the log before ending the title with Options. Look for
`[HUI ARCHIVE CONSUMER] stress PASS`, `soak elapsed_ms`,
`shutdown worker joined=1`, and `clean exit`. Because logs are inside
the running title's sandbox, they may not be visible under `/data` from
ordinary homebrew file explorers. Native debug output may also be captured
using a separate klog receiver.

**Logging performance note:** the current native startup shim explicitly
sets unbuffered stdout/stderr. This produces a crash-resistant log receipt
but can stall on slow filesystems, so don't add per-frame file logging.
