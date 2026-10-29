# Handoff - OpenCMR2 rendering enhancements


## Current graphics coordination (29 October 2026)

Owner: current optional graphics work. This section is the coordination record
for these lanes; the completed batch below is historical, not a claim that all
current experiments have shipped. Read this file and `docs/DEVELOPMENT.md`
before changing these areas. No AGENTS.md, CODEX.md, board, roadmap, TODO or
bugs coordination file was present at this revision. Preserve other owners' work.
All patches are PRs. Merges remain human-led. Never commit original game bytes.

| Lane | Current state and next handoff |
| --- | --- |
| Windows dust test follow-up | Claimed on fix/windows-dust-recording-env, based human-merged PR47/main00b4ca8. New first Windows error in dust_volume_recording.cpp: POSIX unsetenv. Use Windows _putenv_s for that test, retaining Unix path. No other unguarded POSIX environment calls found in current tests. Linux native/ARM/x86 and Android CI pass after PR47; Mac pending; Windows requires follow-up CI. No rendering/runtime/default changes. |
| CI portability repair | Claimed on fix/portable-ci-regressions, current main b74c8065. Failed run40840862203: missing optional Pillow in enhancement preparation test, macOS temporary-root symlink fixture, Windows POSIX setenv, x86 stale Particle size assertion after runtime-only dust tag. Repair tests/fixtures only; no native acceptance or workflow/release changes. Validation: isolated full79/79CTests; prepare8pass/1skip without optional Pillow and9pass with it; road contract10pass with symlink-ancestor refusal; actual32-bit Particle header compile-only probe passes. Mac/Windows/fullx86 CI still required; no native gameplay claim. |
| Curved road CLI | PR #45 merged by the owner. `--roads original` and the default retain original geometry. Experimental `curved` is exact-source Ballon only, jointly selects render/contact buffers, and preserves original FIN. Isolated full suite: 79/79 tests; actual binary prepare-only and loader/session probes passed. Native Mac graphics, driving, culling, barrier alignment and frame-time acceptance remain unverified. Do not treat Linux probes or merge as native acceptance. |
| Reactive crowd/person layers | Claimed, local inactive research only. Existing accepted forearm animation stays unchanged. Source-authored transparent person seam and lossless rest partition checked privately. Head tilt/hop needs extra canvas and matched UV/geometry; neck joins, source-cell layout, single-update integration, car-relative orientation, occlusion, reflection and performance remain open. Single-view art cannot supply true head yaw. No runtime activation or ready-to-merge claim. |
| Scene-only grading | Claimed, local inactive research only. Pure snapshot/transaction and compiled shader checks cover viewport bounds, alpha, failure refusal and later synthetic HUD composition. No game caller. Native SDL_GPU/Metal execution/copy failures, stage/weather eligibility and performance remain open. Preserve original full-frame effects and default settings. |
| Bitmap upscale | Retained private candidates only, no runtime installation. Road seam/mip periodic filtering and exact source identity integration remain open. Foliage, people and fonts stay quarantined from generic atlas filtering. Do not duplicate asset-owner processing or publish owned art. |
| Trees/font source gaps | Preserve accepted tree baseline and human-merged PR #44. Missing exact source exports remain separate; no guessed replacement bytes or blanket acceptance. |

Update the relevant lane with tests, blockers and next handoff as it changes.
Do not overlap these claimed lanes silently or combine inactive experiments into
a road follow-up. Changes to eligibility/defaults need their own reviewed scope.

## Historical completed batch

## Summary
User-adjustable draw distance, in-GUI debug stats (FPS + draw calls + vertices), expanded Advanced graphics options ("juice"), breakable trees, camera effects (dirt on lens, lens flare, human-operator shake), animated idle bitmaps (crowds/trees), motion blur, bloom, rain/snow particles, and directional shadows are all implemented. Everything is gated behind `[video]` settings in `opencmr2.ini` and marked `// OpenCMR2:` per the README.

## Status
- Baseline build fixed (macOS zlib `Byte` typedef was undefined under `TARGET_OS_MAC`; `game/zlib/zconf.h`).
- All 27 tests pass (22 original + `graphics_options`, `render_stats`, `tree_break`, `camera_effects`, `animated_scenery`).
- Build is clean on macOS preset (`-DOPENCMR2_SHADERS_MSL=OFF -DOPENCMR2_RENDERER_GL=ON`).
- Handoff complete: nothing in this batch remains outstanding.

## Completed changes
| Area | Files | What |
|------|-------|------|
| Draw distance | `game/FixedPoint.cpp`, `game/FixedPoint.h` (removed), `game/GraphicsOptions.h` | `video.draw_distance` 25–400% clamp, scales far plane in `View_SetPlayerProjection` |
| Debug stats | `src/render/gfx_core.cpp`, `src/port/gfx.h`, `game/Graphics.cpp/.h`, `game/Game.cpp`, `game/RallyData.cpp` | per-frame draw calls/vertices/indices/triangles; `Graphics_DebugDrawWanted()` (debug flag bit 2 OR `video.debug_draw`); FPS overlay now honors setting; draw calls + vertices on gameplay overlay |
| Texture filtering | `src/render/gfx_gpu.cpp`, `src/render/gfx_gl.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h` | anisotropic default 1→8, always requests feature, `Gfx_SetAnisotropy()` runtime setter |
| Breakable trees | `game/StageObjects.cpp`, `game/Game.cpp` | hard hit (>0x20000 speed) sets `STAGE_OBJECT_BROKEN` flag bit; tree stops colliding and is no longer drawn; default on via `video.tree_break` |
| Camera effects | `game/FixedPoint.cpp`, `game/RallyData.cpp`, `game/Race.cpp`, `game/GraphicsOptions.h` | dirt on lens (accumulates on shake, decays), additive lens flare at screen centre (sun visibility), handheld camera jitter in `ChaseCamera_BuildMatrix`; gated by `video.camera_effects` |
| Advanced graphics menu | `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp` | rows: Draw Distance, Debug HUD, Texture Filtering (Off/2x/4x/8x/16x), Breakable Trees, Camera Effects, Animated Scenery, Motion Blur, Bloom, Particles, Shadows; saved to `opencmr2.ini` |
| Pure helpers | `game/GraphicsOptions.h` | clamp/round-trip helpers for draw distance, anisotropy, lens dirt, shake amplitude, flare brightness, motion-blur mix, bloom intensity, shadow bias |
| Tests | `tests/graphics_options.cpp`, `tests/render_stats.cpp`, `tests/tree_break.cpp`, `tests/camera_effects.cpp`, `tests/animated_scenery.cpp`, `tests/motion_blur.cpp`, `tests/bloom.cpp`, `tests/shadows.cpp` | TDD for clamp mappings, renderer counters, broken flag, dirt/shake/flare helpers, anim frame advance + texture indexing, blur/bloom/bias mappings |
| Animated bitmaps | `game/GraphicsOptions.h`, `game/Game.cpp`, `game/Sector.cpp`, `game/Graphics.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp` | `video.animated_scenery` Off/On row in Advanced graphics (default off). `STAGE_OBJECT_ANIMATED` flag bit; per-object frame/timer state in a global table (cleared on stage unload); `AnimObject_Texture()` swaps the current cycle frame into the draw path (immediate and deferred); menu row saved to `opencmr2.ini`. |
| Motion blur | `src/render/gfx_gpu.cpp`, `src/render/gfx_core.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h`, `src/render/shaders/motionblur.*`, `game/GraphicsOptions.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp`, `tests/motion_blur.cpp` | `video.motion_blur` Off/On row; full-screen blend of current frame into previous frame (mix factor from strength); SDL_GPU implementation, OpenGL stub |
| Bloom | `src/render/gfx_gpu.cpp`, `src/render/gfx_core.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h`, `src/render/shaders/brightpass.frag blur.frag composite.frag`, `game/GraphicsOptions.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp`, `tests/bloom.cpp` | `video.bloom` Off/On row; bright-pass extract, separable Gaussian blur, additive composite into the scene; SDL_GPU implementation, OpenGL stub |
| Particles | `src/render/gfx_gpu.cpp`, `src/render/gfx_core.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h`, `src/render/shaders/particle.vert particle.frag`, `game/GraphicsOptions.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp` | `video.particles` Off/Rain/Snow row; simulated and rendered entirely in the port layer (screen-space billboards), no game-code changes; SDL_GPU implementation, OpenGL stub |
| Directional shadows | `src/render/gfx_gpu.cpp`, `src/render/gfx_core.cpp`, `src/render/gfx_internal.h`, `src/port/gfx.h`, `src/render/shaders/common.glsl fixed.frag fixed3d.vert fixedtl.vert shadow.vert shadow.frag`, `game/GraphicsOptions.h`, `game/FrontendMenus.cpp`, `game/FrontendScreens.cpp`, `tests/shadows.cpp` | `video.shadows` Off/On row + depth bias; 1024x1024 shadow map from a directional sun light rendered before the normal pass, sampled with PCF in the fragment shader; SDL_GPU implementation, OpenGL stub |

## Key references
- `StageObject` struct: `game/Sector.h:36` (disk record 0xa0 bytes; `pMesh` at 0xc, `flags` at 0x10)
- `SectorObjectEntry`: `game/SectorObjectEntry.h:20` (runtime; `pObject`, `pBounds`)
- Collision/draw share `StageObject` instances (RallyData.cpp:7422 draw list build, 7458 collision entry)
- Draw loop: `Game_DrawStaticStageObjects` Game.cpp:2321; object draw via `Graphics_DrawMeshLOD` or queued `StageObjectDraw`
- `g_sunVisibility` (StageObjects.cpp:6156, set at 6814) for flare brightness
- Settings: `Sys_GetOption`/`Sys_SetOption` (src/platform/sys.cpp:1204)

## Build/test commands
```
cmake --preset macos -DOPENCMR2_SHADERS_MSL=OFF -DOPENCMR2_RENDERER_GL=ON
cmake --build build/macos -j 8
ctest --test-dir build/macos
```
