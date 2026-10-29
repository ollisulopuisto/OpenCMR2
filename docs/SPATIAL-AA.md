# Opt-in spatial antialiasing

This patch adds `spatial_aa = 1` in `[video]` to the SDL_GPU renderer, including
its Metal backend. The default is `0`, preserving the existing render path.
Restart the game after changing it. The OpenGL 3.3 executor now uses the same
translated fragment shader before bloom and other effects. This is a port, not
a Metal or OpenGL hardware restriction. Both backends keep the default off.

This is single-frame edge smoothing, not MetalFX, temporal AA, a resolution
upscaler, or an FPS optimization. It can soften small text and thin features.
It runs on the complete back buffer, including HUD/menu/movie content, before
bloom, motion blur and the other post effects. No game assets are changed.

The new pass uses a separate color target, linear sampling and clamped borders.
It never samples its own render target. Shader, pipeline and target are created
only when enabled. Target storage is released on resize and shutdown. Failure
falls back to the original scene and disables further attempts for that session.
It adds one full-resolution BGRA8 target (4 bytes/pixel) plus driver overhead.
The dimensions follow the game's back buffer, not the window's drawable size.

## Why not MetalFX yet?

The pinned SDL 3.4.18 GPU API provides opaque textures and command buffers. Its
public API does not expose native MTLTexture/MTLCommandBuffer handles for the
MetalFX encoder. Reaching through SDL's private backend structs would couple
the game to private layout and lifetime assumptions. This patch does not do it.
Temporal reconstruction would also need motion vectors, jitter, history reset,
and a HUD separation plan. Spatial AA is a bounded first improvement, not a
claim that MetalFX has been implemented.

API references:
- https://wiki.libsdl.org/SDL3/SDL_GetGPUDeviceProperties
- https://github.com/libsdl-org/SDL/blob/release-3.4.18/include/SDL3/SDL_gpu.h
- https://developer.apple.com/documentation/metalfx/mtlfxspatialscaler

## Tests and current limits

On Linux, the full game builds with Clang 14, including generation of Metal MSL
from SPIR-V. The optional real-pixel test runs the translated GLSL shader on
Mesa llvmpipe. It checks flat colors, alpha, low-contrast gradients, diagonal
edges and clamped borders. Synthetic pictures are not game-quality acceptance.

Run the optional pixel test with glslangValidator and spirv-cross on PATH:

```sh
python3 -m venv /tmp/opencmr2-pixels
/tmp/opencmr2-pixels/bin/pip install moderngl numpy Pillow
/tmp/opencmr2-pixels/bin/python tests/spatial_aa_shader.py --output /tmp/aa.png
```

Or configure `-DOPENCMR2_SHADER_PIXEL_TESTS=ON` with that Python interpreter and
run CTest on a machine with EGL. The default build needs no Python GL packages.

The existing `tree_break` test fails with `gentle tap does not break` on both
unchanged 5f36d94 and this patch. The other 30 default tests and the new optional pixel test pass locally (31/32 total with pixel tests enabled).

## Mac acceptance still required

No Mac runtime or game data was available during this implementation. MSL
translation is not Metal compiler/runtime validation. No FPS or native image
quality claim has been made. Before treating this patch as ready to merge:

1. Build the normal macOS preset with MSL enabled, then run CTest. Do not replace
   your working game binary or data directory until the new binary is checked.
2. Use the same replay, camera, resolution, settings and display scale for both
   `spatial_aa = 0` and `1`. Back up the INI and change only this one setting.
3. Compare road edges, fences, foliage, thin lines, dashboard and menu text while
   stopped and moving. Also check window resize, fullscreen, menus and movies.
4. Repeat with bloom, motion blur and lens effects both off and on. Confirm no
   black frames, feedback artifacts, border bleed or memory growth on resize.
5. Record median/p95 CPU and GPU frame time using Xcode Metal capture/Instruments,
   with the same vsync setting and replay duration. Warm both paths first. Do not
   infer GPU cost from FPS while the display caps FPS. Keep AA off if softening
   or frame cost is unacceptable. Restore the backed-up INI to roll back.
