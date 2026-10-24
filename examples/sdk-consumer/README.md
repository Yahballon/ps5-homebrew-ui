# PS5 Homebrew UI SDK — archive-only consumer

This is the real independent SDK linking test.

- **Title:** Homebrew UI SDK Consumer
- **Title ID:** `PPSA99863`
- **Version:** `v0.3.0`
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
