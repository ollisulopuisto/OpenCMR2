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

The standalone probe deliberately does not change CMake, workflows, minimum game
OS, signing, runner settings or the default game render path.
