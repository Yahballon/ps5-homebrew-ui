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
