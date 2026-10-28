# Native MetalFX spatial probe

This is runnable native prototype source, not an integrated game feature. It
creates a MetalFX spatial scaler, encodes a synthetic 320x200 pattern to640x400,
checks completion/nonflat output and optionally saves a PPM for visual inspection.
It does not reach into SDL internals or alter the game. CPU upload/readback is
only for this one-shot synthetic test, never a proposed per-frame game path.

On a Mac with Xcode command-line tools and the macOS13+ SDK:

```sh
xcrun clang++ -std=c++17 -fobjc-arc -mmacosx-version-min=13.0 \
  -framework Foundation -framework Metal -framework MetalFX \
  tools/metalfx/spatial_probe.mm -o /tmp/opencmr2-metalfx-probe
MTL_DEBUG_LAYER=1 /tmp/opencmr2-metalfx-probe /tmp/metalfx-probe.ppm
open /tmp/metalfx-probe.ppm
```

Exit0 means one synthetic encode completed. Exit2 means OS/device unavailable;
exit1 means resource/encode/output failure. Neither success nor failure proves
anything about OpenCMR2's SDL pipeline. The source has not been compiled or run
on a Mac during preparation; Linux syntax review cannot verify Apple headers.
A successful native run and inspection are the next gate, not a claimed result.

After that, the unresolved integration is a narrow maintained SDL Metal backend
bridge exposing current command-buffer and texture lifetimes. Do not cast opaque
SDL structs. Reuse the scaler across frames, rebuild on resolution changes,
encode on the same queue with no active encoder, honor texture usage requirements,
and preserve original blit fallback. Native MetalFX output must be checked against
the game's SDR color space. Do not feed grain/noise before spatial upscaling.
Temporal remains separate: motion vectors, jitter, history reset and HUD separation.

Primary API examples and placement advice:
https://developer.apple.com/videos/play/wwdc2022/10103/

Existing Mac builds now compile this standalone target when the SDK provides
MetalFX, without running it or linking MetalFX into the game. The normal game
minimum OS, workflows, signing, runner settings and default rendering stay
unchanged. A missing SDK produces an explicit warning, not a probe pass.

## SDL integration (experimental)

`OPENCMR2_METALFX=ON` on Mac builds uses pinned vendored SDL3.4.18 and applies the
isolated backend extension before compiling it. It links MetalFX into that SDL
library. Non-Mac builds and `OPENCMR2_METALFX=OFF` remain unchanged. An unmodified
system SDL still runs the game, but cannot provide this extension.

Set `[video] metalfx_spatial = 1`, restart, and select a game resolution smaller
than the drawable/window to exercise scaling. It does not automatically lower
the game's render resolution. It falls back for unsupported OS/device, unavailable
extension, no upscaling, unsupported texture usage/format, movie stretch, or active
film grain/chromatic aberration. Existing game geometry/HUD remain one input;
text and grain separation is not implemented. Enable spatial_aa separately if
needed: MetalFX spatial expects antialiased input, not temporal reconstruction.

The backend encodes on the current SDL command buffer, tracks input/output via
SDL's in-flight references, cycles output textures rather than overwriting a busy
frame, and uses ordinary SDL blit for same-size letterbox placement. Cached scaler
rebuilds on input/output dimension changes. The game sees only a narrow callback
property, never native handles or private SDL structs. This is a single renderer /
render-thread path; no new cross-thread ownership promise is made.

Native compile/GPU/game output have not yet been verified. The first log says
whether a frame encoded or fell back, not that all later frames succeeded or that
FPS improved. Actual visual inspection/API validation and frame-time remain gates.
Temporal is the next phase, not hidden inside spatial scaling: per-object previous
transforms, projected motion, jitter, history invalidation and HUD separation must
be implemented and verified on moving scenes before it can be advertised.
