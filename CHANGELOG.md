# Changelog

All notable changes to OpenCMR2 are documented here.

## [Unreleased]

### Atmosphere

- **Fog responds to car lights** (`video.fog_lights`, 0-200%, default 100):
  head and rear lights visibly cut the fog - radial in-scatter around each
  projected car light in the fog pass, tinted per light and coupled to the
  local fog density, with a depth gate off the car's own body (SDL_GPU; the
  OpenGL renderer keeps the glow unoccluded). Atmosphere menu row, launch
  flag `--fog-lights`, and the `atmosphere` test preset enable it.

- **Snow glints** (`video.snow_glint`, 0-200%, default 100): snow sparkles in
  the light. Snowbanks throw short-lived glint particles where the half-vector
  between sun and view aligns with a jittered micro-facet of the true ground
  normal (deterministic per ground position, so glints stick to the ground
  and flicker as the view moves), and falling flakes twinkle with their own
  sway phase, strongest looking toward the sun (forward scatter). Car head
  and rear lights boost both from the same light field the fog response uses,
  so flakes and banks brighten inside the beams. Render-only; nothing feeds
  physics or the rand stream. Atmosphere menu row, launch flag
  `--snow-glint`, and the `atmosphere` test preset enable it.

### Rendering enhancements ("juice")

All changes below are optional and gated behind `[video]` settings in
`opencmr2.ini`, exposed as rows in the Advanced graphics menu, so the original
look stays available.

- **Draw distance** — new `video.draw_distance` setting (25–400% of the stage's
  far plane) scales the far plane in `View_SetPlayerProjection`, easing between
  the route's view distances instead of snapping.
- **Debug HUD** — per-frame draw calls / vertices / indices / triangles counters
  plus FPS on the gameplay overlay, controlled by `video.debug_draw` (also the
  debug flag bit 2).
- **Texture filtering** — anisotropic filtering now defaults to 8x (was off as in
  the original) via `video.anisotropy` (Off/2x/4x/8x/16x), implemented in both
  the SDL GPU (Vulkan/D3D12/Metal) and OpenGL renderers with a runtime
  `Gfx_SetAnisotropy()` setter.
- **Breakable trees** — a hard hit sets `STAGE_OBJECT_BROKEN`; the tree stops
  colliding and is no longer drawn. Controlled by `video.tree_break` (default
  on).
- **Camera effects** — dirt on lens (accumulates on shake, decays), additive lens
  flare at screen centre driven by sun visibility, and handheld camera-operator
  jitter in `ChaseCamera_BuildMatrix`. Gated by `video.camera_effects`.
- **Animated bitmaps** — idle scenery (crowds/trees) cycles through its bitmap
  frames behind the video when `video.animated_scenery` is on (default off).
  `STAGE_OBJECT_ANIMATED` flag bit, per-object frame/timer state cleared on
  stage unload, applied to both immediate and deferred draw paths.
- **Motion blur** — blends each frame into the previous one for motion
  ghosting, controlled by `video.motion_blur` (Off/On; blend strength fixed at
  a modest level). Implemented as a full-screen blend pass in the SDL_GPU
  renderer.
- **Bloom** — bright pixels bleed into their neighbours with a soft glow,
  controlled by `video.bloom` (Off/On). Bright-pass extract, separable Gaussian
  blur and additive composite, all in the SDL_GPU renderer.
- **Particles** — rain or snow falling through the scene, controlled by
  `video.particles` (Off/Rain/Snow). Simulated and rendered entirely in the
  port layer, no game-code changes required.
- **Directional shadows** — a shadow map from a directional sun light darkens
  occluded surfaces, controlled by `video.shadows` (Off/On) with a depth bias.
  Rendered into a 1024x1024 depth texture before the normal pass and sampled
  with PCF in the fixed-function fragment shader (SDL_GPU only).
- **Advanced graphics menu** — new rows for Draw Distance, Debug HUD, Texture
  Filtering, Breakable Trees, Camera Effects, Animated Scenery, Motion Blur,
  Bloom, Particles and Shadows, all saved to `opencmr2.ini`.

### Related visual work in this batch

- Fog the sky and backdrop of foggy stages (`video.fog`-adjacent far-plane work).
- Dissolve scenery into the sky/backdrop toward the far plane.
- Car reflections drawn only from the sectors they reach; sorted and blended by
  their own distance; background layer drawn without depth after reflections.
- Vectorised arcade minimaps, dotted circuit-selection maps, menu flags, and
  co-driver/menu icons drawn from resolution-independent geometry.
- Animated menu flags and selection arrows preserved.

### Build & platform

- Baseline build fixed on macOS (zlib `Byte` typedef under `TARGET_OS_MAC`).
- Build is clean on the macOS preset
  (`-DOPENCMR2_SHADERS_MSL=OFF -DOPENCMR2_RENDERER_GL=ON`).

### Testing

- 30 tests pass (22 original + `graphics_options`, `render_stats`, `tree_break`,
  `camera_effects`, `animated_scenery`, `motion_blur`, `bloom`, `shadows`).
