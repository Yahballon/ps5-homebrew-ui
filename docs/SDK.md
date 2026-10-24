# Reusable PS5 Homebrew UI SDK (experimental)

This repository contains the native C++ OpenGL renderer and 99 reusable UI components. The
new `src/sdk/` headers add **portable application services**; they are not a replacement
renderer and are not yet a stable ABI.

## Make the library

```sh
make sdk
# dist/sdk/lib/libps5hui.a
# dist/ps5-homebrew-ui-sdk.tar.gz
```

This requires the same public PS5 toolchain and OpenGL runtime as `make app`.
The pinned `ps5-opengl` SDK 1.0.2 download tries the original upstream
release first, then a third-party archive mirror when upstream is unavailable.
Both paths are checked against the **same pinned archive SHA-256**, and the
extracted manifest has its own pinned digest. Never bypass these checks. The target
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
| `sdk/bmp.hpp` | Lightweight strict uncompressed 24/32-bit BMP decoder |
| `sdk/image_decode.hpp` | Bounded PNG/JPEG/BMP/GIF/TGA/PNM decoder via vendored stb_image |
| `sdk/async_images.hpp` | Nonblocking file-reader/decode/CPU-cache/GPU-upload cover pipeline |
| `sdk/read_image_file.hpp` | Bounded file-reader adapter using existing `save::read_file` |
| `sdk/texture_cache.hpp` | GPU budget/LRU cache via explicit render-thread callbacks |
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

hui::sdk::TextureCache texture_cache(
    64u * 1024u * 1024u,
    [&](const hui::sdk::Image &image) {
        return renderer.batch().create_texture(image.width, image.height, image.rgba.data());
    },
    [&](std::uint32_t texture) { renderer.batch().destroy_texture(texture); });
// texture_cache.get_or_upload("cover", image) on the render thread.
// texture_cache.clear() before renderer.release().
// On context loss, texture_cache.context_lost() must be called WITHOUT GL calls.
```

The queue intentionally does **not** create OS threads: apps supply platform threads
with deliberate stack sizes and scheduling. A token's `finished()` means work returned,
not that a download succeeded. App callbacks publish results to the UI thread.
Cancelling a running job is cooperative. Join worker threads before destroying the queue.

### Putting the decoder, worker queue, and GPU upload together

```cpp
hui::sdk::AsyncImages covers(
    jobs, cpu_cache, texture_cache, hui::sdk::read_image_file);

covers.request("DOOM64", "/data/N64XY/covers/DOOM64.png");
// One of the application's OS worker threads invokes jobs.process_one().
// Every render frame, on the UI thread:
covers.pump(2); // upload at most two decoded covers
const std::uint32_t texture = covers.texture("DOOM64");
if (texture != 0) {
    // draw the cover via gfx::DrawList::image(...);
}
```

The pipeline does not create worker threads. The default
`read_image_file` adapter uses `save::read_file` with a size limit; apps can
supply an alternate callback for different storage/HTTP sources. It never calls OpenGL
in the worker. It uses the queue's *unreported completion* mode, so its many
jobs do not fill the shared completion-deque; other applications can still use
`JobQueue::take_completed()` for their own jobs. Use `cancel_all()` before
shutting down the workers, then join them before destructing the queue. The
GPU cache may evict textures once its budget is reached. Re-requests of a cover
cached in RAM are queued for a capped UI-thread re-upload, not uploaded during
`request()`. Draw only texture handles retrieved **this frame**, not stale
stored texture IDs.

### Runtime storage

`StoragePaths` defaults to `/app0`, `/download0`, `/data`, and `/mnt/usb0`.
These are *names*, not a promise that a given sandbox can access them. Elevated titles
often need `set_root(install, "/data/homebrew/PPSA...")`. `resolve` rejects
relative traversal but is not a filesystem sandbox: symlinks and race conditions must
also be considered by the file-accessing application.

## Remaining v1.0 blockers — do not misrepresent as implemented

- **WebP** decoding, priority scheduling and fully integrated platform worker threads. The
  new `decode_image` handles PNG/JPEG/BMP/GIF/TGA/PNM through stb_image with
  configurable input-size and decoded-pixel limits (GIF: first frame only).
  The lightweight `decode_bmp` handles **uncompressed 24/32-bit BMP**.
  `ImageCache` owns CPU RGBA images, while `TextureCache` uploads/evicts
  through explicit GL callbacks. `GlBatch::destroy_texture` now releases image textures; callers
  must not draw texture handles after eviction. The cache must be cleared
  *before* destroying the GL context.
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

The portable service APIs have unit tests in `tests/unit/sdk_test.cpp`,
`tests/unit/sdk_images_test.cpp`, `tests/unit/sdk_decode_test.cpp`, and `tests/unit/sdk_async_images_test.cpp`. Hardware
performance of the integrated SDK is **not verified** until CI and a console run succeed.

## Licensing

GPL-3.0-or-later; see `LICENSE` and `THIRD_PARTY_NOTICES.md`. Applications linked with
the SDK must satisfy the license terms.
