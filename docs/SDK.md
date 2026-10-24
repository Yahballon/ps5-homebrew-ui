# Reusable PS5 Homebrew UI SDK (experimental)

This repository contains the native C++ OpenGL renderer and 99 reusable UI components. The
new `src/sdk/` headers add **portable application services**; they are not a replacement
renderer and are not yet a stable ABI.

## Make the library

```sh
make sdk
# dist/sdk/libps5hui.a
# dist/ps5-homebrew-ui-sdk.tar.gz
```

This requires the same public PS5 toolchain and OpenGL runtime as `make app`. The target
builds the PS5 app once, then archives the existing platform-neutral and PS5 backend object
files *without* the demo app's `main.cpp`, `app/`, `concepts/` or `demo/`. The package
includes headers, original fonts/sounds, notices, and the runtime shim sources. Link the
archive with the matching PS5 SDK / ps5-opengl dependencies. It is **not** a drop-in
replacement for the loader, and does not contain a second copy of the entry point.

## Public portable services

| Header | Facility |
| --- | --- |
| `sdk/navigation.hpp` | Bounded, explicit screen/back-stack state and revisions |
| `sdk/jobs.hpp` | Multi-producer worker queue, cancellation tokens and progress |
| `sdk/assets.hpp` | Validated decoded RGBA images and memory-bounded CPU LRU cache |
| `sdk/i18n.hpp` | Translatable keys, fallback catalog and named substitutions |
| `sdk/paths.hpp` | Install/save/data/USB roots and validation of relative paths |
| `sdk/diagnostics.hpp` | Allocation-free rolling frame times, p99 and budget misses |
| `sdk/sdk.hpp` | Include-all facade for the portable facilities |

Existing `ui/components.hpp`, `gfx/renderer.hpp`, `core/input.hpp`, `core/save_file.hpp`
and `platform/ps5/*` remain the real UI and platform APIs. Use their existing documentation.

### Main-thread / worker contract

```cpp
hui::sdk::JobQueue jobs;
auto token = jobs.submit([](hui::sdk::JobQueue::Context &ctx) {
    // Do file I/O and decode into CPU memory; check ctx.cancelled() often.
    ctx.set_progress(0.5f);
});
// A platform worker thread calls jobs.process_one() until shutdown.
// The UI frame polls jobs.take_completed() and uploads finished images with
// renderer.batch().create_texture(...); do not call OpenGL on the worker.
```

The queue intentionally does **not** create OS threads: apps supply platform threads
with deliberate stack sizes and scheduling. A token's `finished()` means work returned,
not that a download succeeded. App callbacks publish results to the UI thread.
Cancelling a running job is cooperative. Join worker threads before destroying the queue.

### Runtime storage

`StoragePaths` defaults to `/app0`, `/download0`, `/data`, and `/mnt/usb0`.
These are *names*, not a promise that a given sandbox can access them. Elevated titles
often need `set_root(install, "/data/homebrew/PPSA...")`. `resolve` rejects
relative traversal but is not a filesystem sandbox: symlinks and race conditions must
also be considered by the file-accessing application.

## Remaining v1.0 blockers — do not misrepresent as implemented

- PNG/JPEG/WebP decoders, on-demand GPU cache, upload queue, and GPU texture destruction.
  The new `ImageCache` deliberately owns CPU RGBA images only.
- A production OS thread pool with shutdown/join ownership, cancellation on lifecycle
  transitions, and priority queues.
- Complete multilingual shaping / bidirectional text (dictionary substitution is **not**
  font shaping or pluralization), locale number/date formats, and OS accessibility hooks.
- Signal-safe native crash dumps and symbolication, robust suspend/resume/system overlays,
  multi-user pad handling, and recovery when EGL contexts are recreated.
- A stable standalone sample app that links only `libps5hui.a` with automated console
  validation; stable ABI/version policy and semver testing.
- Full PNG screenshots during live gameplay, image/asset hot reload, JSON theme editing,
  and optional renderer ports.

The portable service APIs have unit tests in `tests/unit/sdk_test.cpp`. Hardware
performance of the integrated SDK is **not verified** until CI and a console run succeed.

## Licensing

GPL-3.0-or-later; see `LICENSE` and `THIRD_PARTY_NOTICES.md`. Applications linked with
the SDK must satisfy the license terms.
