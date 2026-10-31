# Changelog

All notable changes to OpenCMR2 are documented here.

## [Unreleased]

### Interface

- **Atmosphere menu directly under Graphics options**: the fog/air
  motes/snow glint rows move up from Advanced graphics > Effect amounts, so
  the fog layer is one click from Graphics options. What's New revision 3
  notes the move.

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

- **Weather and motes respond to light** (`video.snow_glint` / `video.air_motes`):
  the screen-space snowfall overlay (both renderers) twinkles toward the sun
  (forward scatter) and brightens inside projected car light beams, tinted per
  light; the floating air motes do the same in world space - sun forward
  scatter computed per step from the view and the stage sun, beam boosts from
  the shared light field. The `atmosphere` test preset is now the durable
  "accepted atmosphere + effects under test" bundle: fog 100 with full sun
  glow, near clear 60, spatial pockets, clearing toward the finish, fog light
  response, snow glints, 500% air motes and god rays 80. Explicit flags still
  override; the experimental `video.fog_cull` stays opt-in.

- **Snow surface material** (`video.snow_material`, 0-200%, default 0 = off;
  prototype): the snow ground itself responds to light. Visible terrain
  triangles are tagged snow only on collision-mesh evidence (the raw surface
  ids the wheel particles use at the triangle centroid, an on-ground height
  tolerance and an upward face normal), a vertex shared with a non-snow
  triangle is never lifted, and tagged vertices get a sun half-vector glint
  plus a cone boost inside car light beams, recomputed from the saved
  original colours every frame. Launch flag `--snow-material`. Terrain beyond
  the collision corridor gets no response; the per-frame vertex-buffer
  refresh cost is unmeasured, hence off by default.

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
- **Drone tuning ranges** — the F10 tuning panel's Drone accel/brake spring row
  now goes down to 5% (was 25%) and Drone speed height drop up to 200% (was
  100%), per native testing feedback. Selected values and defaults (100%) are
  unchanged; the spring's critically damped ratio holds at the new floor, and
  the speed drop keeps its 2.2-unit height floor. `--drone-speed-drop` accepts
  0..200 to match.

- **What's new screen** — a short changelog of shipped changes shows once on
  the main menu after an update (tracked in `ui.whatsnew_seen`) and stays
  reachable from Options > Graphics > Screen. Content is curated in
  `game/WhatsNew.h`; `kWhatsNewRevision` is bumped with new entries.

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
