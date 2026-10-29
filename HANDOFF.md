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
| ELF wrap isolation gates | Claimed on fix/elf-wrap-test-platform-gate, main3a094f7 after merged PR50/51. Windows Clang is not MSVC, but its COFF lld-link cannot implement GNU ELF --wrap/Itanium symbol interception. Replace compiler-brand guesses with explicit Linux GNU/Clang gate across all eight affected blocks. Keep native Windows portable tests; unsupported isolation harness coverage remains a documented gap. No game change. |
| Tree test standard header | Claimed on fix/tree-test-string-header, main1e2a625 after merged PR49. Windows now links scenery test and game, then tree collision test fails std::to_string because it omitted string header. Add direct standard include, no behavior change. Linux/Android pass, Mac remains queued. |
| Windows scenery test linkage | Claimed on fix/windows-scenery-test-linkage, based merged PR48/main4dd2c90. Windows advanced to scenery_pack_runtime link failure: private-to-public macro changes MSVC access-qualified static symbol names. Replace macro with narrow friend test accessor, preserving real private linkage. No runtime behavior change. |
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

## France1 first asset test

Claim: feat/france1-asset-test. User selected France1 first. Exact hi/lo quartets
recovered and verified; hi matches existing Ballon hash, lo differs and currently
fails old unique-contact-triplet converter guard. Keep lo road original, do not
relax it. Opaque t_1a/b2x periodic-filter/mips only, exact four high/low source
hashes, CLI --textures original|france1, stage-name+originalC3D gate, originals
preserved. No restored detail claim. People/tree motion is now mergedPR54.
Hedge/bank depth proposal accepted; regional strips are not
yet bound to screenshot's actual mesh. Native appearance/performance unverified.

## Current request: full supported scenery motion

Claim: feat/stronger-supported-scenery, based main7efec63. Keep accepted source-fit
3D tree geometry and authored arm frames. All verified spectator placements now
animate (remove Finland65%placement gate), plus fixed-feet geometry sway moves
all people on each supported card rather than one arm on a multi-person atlas.
Tree crown amplitude increases1.5%to4.5%height, applies to fitted replacement
geometry too, with fixed base and immutable source/cached vertices. Same stage
simulation clock and deterministic phase; no render-clock or physics RNG use.
This does not claim every game's asset/track is source-covered or add unrelated
object motion. Native culling/reflections/appearance/performance remain open.

## Deliberate custom-camera operator

Claim: feat/deliberate-camera-operator on main15308db. Existing camera_effects
is shake, not anticipation. New opt-in Human operator row in actual custom
camera menu saves separately on accept, Cancel does not save. Only custom
mode4 gets bounded, acceleration/slew-limited orientation lead from heading
trend and actual travel direction. No position movement, random jitter or
extra spring lag. Existing height/distance/pitch/lag and terrain-clearance
paths retained; operator suppresses old chase jitter only while active.
State per camera index, reset start/restart/disabled/discontinuous heading.
Source and actual update/apply harness tests pass; full83/83 Linux tests on
merged main ddeec08. Native feel/menu/seek/split-screen acceptance unverified.

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

## CI execution efficiency handoff

Claim: ci/deduplicate-native-checks-final. Preserve six native build targets,
existing tag release dependencies and artifacts. Remove feature-push/PR duplicate
execution, ignore docs-only automatic changes, cancel obsolete PR checks only,
and bound jobs/individual CTests. Manual full checks remain available. No new
cache/storage/upload permissions. Six policy tests and actionlint pass; revised
workflow execution remains unverified. Existing credential cannot write workflow
files, so this change is a user-applied patch on a separate branch/PR, not a
published workflow. Windows portability repairs remain separate PRs. Revert
this one commit through a PR if the execution policy needs to be rolled back.


## Current requested steering

Claim: feat/wiper-w-toggle. User requests stronger people/tree motion, all-track
rounding, W wiper toggle and actual in-app remapping. Wiper change is isolated:
physical W default, saved explicit key retained, rising-edge continuous toggle
independent of dirt strength. Does not clear any driving bindings. W may conflict
with user WASD presets; current source-only review cannot inspect their saved
controller file. In-app original driving reconfiguration exists through Controls
and device configuration, but wiper is outside its nine actions. Controls now adds a separate wiper-key capture row, with release-before-capture,
Escape cancel and refusal of driving-key conflicts. Implicit W is disabled when
active keyboard driving uses W, rather than stealing acceleration; explicit saved
choices are retained. Native menu pixels/navigation remain unverified.
All-track rounding remains source-gated Ballon only until paired source/layout
and FIN/native validation can extend safely. No arbitrary object animation.

## Cockpit W toggle failure follow-up

Claim: fix/cockpit-wiper-toggle. Native owner report: 3D blades always on in
rain and W has no effect. Confirmed source mismatch: PR53 toggles the added
screen-dirt overlay; CarInterior_UpdateWiperSweep independently re-enables the
actual cockpit blade modes from rain and ignores that option. Fix the shared
control path and test actual rain/press/held/release/off/park/on transitions,
not only a pure bool helper. Preserve rain-dependent speed, CIN blade limits,
remapping and driving conflicts. Native behavior remains failed acceptance
until a new build is tried. Dust cost regression is the other current priority;
hedge/bush/grass additions remain saved and paused, not lost or complete.
## Driving remapping menu reachability

Claim: fix/driving-bindings-menu. Native report says only Wiper rebind is
available. Source reproduction: Controls exposes two opaque controller rows;
the device configuration's sole link to ten existing binding actions is labeled
Back (localized 0x67). Selecting apparent Back actually enters bindings, so code
existence was not a usable UI. Make that action explicitly Rebind driving
controls, label the bindings save action and player rows. Test constructed menu
links and actual action traversal/capture/save. Native pixels/navigation still
need owner's check; do not tell them an unverified hidden path is usable.
## Heavy dust cost follow-up

Claim: perf/dust-shared-detail-noise. Native screenshots show lower FPS during
heavy dust; varying camera/time and recording mean this is not a controlled
bottleneck measurement. Inspect actual kernel and reduce repeated work without
changing density/shape first. Shared world detail noise is independent of lobe
and now computed once per ray sample before the lobe loop. The lazy branch
variant was byte-identical but slightly slower on llvmpipe, so was rejected. Preserve depth/HUD,
fallbacks, sample counts/defaults and compare actual shader pixels, not source
syntax alone. Mac same-camera/weather frame-time acceptance remains required.
Wiper PR57 and Controls PR58 stay separate; scenery additions paused.

Local synthetic240x160 draw-query evidence, not Mac gameplay FPS: interleaved
40timed samples each, dense32lobes original median6.410ms vs shared5.157ms;
sparse3smalllobes original.0943ms vs shared.0856ms (noisy short draws).
These measure llvmpipe/software rasterization only.120 before/after actual
compiled-kernel outputs byte-identical across dense/empty/occluded/orbit cases.
No sample-count, shape, pool, compositor, default or fallback change. Native
same-camera/weather heavy-dust comparison remains open.

## Helicopter camera operator refinement

Claim: fix/helicopter-camera-framing. Owner screenshot/steering asks stable
helicopter following, occasional late reframing after a turn and slight
overshoot, not continuous anticipation or shake. Test bypass of original spring
only while customized operator mode is on; never overwrite saved lag. Native
label loses uppercase H/O; inspect font glyph path, use menu-consistent lowercase
and verify actual pixels once exact owned font pair is available. Native feel,
projection and menu acceptance stay open. Dust59 and cockpit57 remain separate.

Refinement validation:86/86Linux tests. Actual ChaseCamera_Update confirms custom
operator spring bypass and immediate original-lag restoration while keeping saved
lag unchanged; temporary isolated INI only. State fixture confirms mirrored turns,
cut/NaN/dt resets, per-view independence, bounds, return to stable framing and a
small single overshoot. Synthetic .3rad/s turn after calm produces -8.4deg lag,
+.32deg overshoot, then zero; not a native gameplay claim. Label regression calls
actual Font_DrawText with a deliberately lowercase-only glyph map: old uppercase
H/O vanish, new lowercase label emits both. Original font pair unavailable, so
this is a source-path regression, NOT native-font/pixel acceptance. Loader slot1
is Fonts/general/hel_15pt in frontend, hel_12pt in-race (.pcf+.tga). Native
menu and helicopter feel must still be checked in owner build.

## PR59 CI repair follow-up
All failed Linux job logs show recipe tests missing PIL. Exact desktop workflow
requirements-install patch held separately because credential lacks workflow
scope. Linux CI dependency gap remains open, not fixed by skipping tests.
Windows enhancements_test link failure came from redefining private as public,
changing static member mangling. Use existing public SetGameHDPath, same save
assertions. Explicit string headers repair wiper and camera Windows tests.
Both human-merged camera/wiper and dust notes retained above. Native CI pending;
no merges, dispatches or reruns performed.

## Accepted graphics CLI options follow-through
Claim: fix/graphics-cli-options. User command used --textures=france1, silently
passed through rather than selecting texture preparation. Accept equals syntax
for recognized valued enhancement flags, preserving separate-token form. AA
CLI overrides were accepted/logged but GPU constructor read saved-only settings;
use the same override-aware startup selector in constructor and runtime tests.
No saved defaults/profile change. Test full user's combined flags with synthetic
helpers, not proprietary artwork or a claimed native renderer acceptance.
Validation87/87Linuxpass: full user's combined argument form through actual binary
with isolated synthetic helper outputs dispatches textures/roads/scenery/cars.
Invalid/empty equals values fail. Actual Platform_Init per-launch AA request feeds
same GPUStartupOptions::Read used by GPU constructor, saved-only reader stays0,
INI remains byte-identical and subsequent no-flag launch restores saved0.
No native frame or active AA/MetalFX success claim. MetalFX still needs compatible
upscale/extension and reports encoded/fallback; input render above output is not
an upscale recommendation. Font174/174 vectorized log covers existing glyph cells,
not proof of uppercase code coverage; native lowercase label check remains open.

## Native camera rejection: world-space inertia
Claim: fix/world-inertial-helicopter-camera. User rejected rear-attached aiming;
requires independent camera inertia, car can turn before camera follows, distant
helicopter/long lens, wider turns/tighter fast straights. Previous spring bypass
and orientation-only offset were wrong. Replace with simulation-owned world
position/velocity, trailing heading, damped aim and actual projection zoom. Off
restores untouched original settings. Verify real built records, not graph only;
synthetic visuals cannot establish native feel or terrain clearance acceptance.
Validation87/87Linuxpass, finalfocused4/4 after saved-tilt regression. Real
ChaseCamera_Update/BuildMatrix now shows car yaw30deg without immediate camera
world orbit/forward rotation, viewpoint farther back and projection zoom wider
with steering. Repeated draws don't step world physics. Original spring path
restored, saved lag untouched/off returns original custom path; per-view state,
teleport/cut/bad-input resets covered. Saved custom tilt offset relative to
original default carried into independent look-at basis. Synthetic geometry/path
pixels inspected (world trajectory and lens1.46turn vs1.85straight fixture), NOT
native game/terrain/collision/feel proof. Steering is an input cue before body
rotation, not route knowledge or guaranteed anticipation of an unseen corner.
Native rejection remains open until owner tests this distinct architecture.
## Wiper binding beside driving actions
Claim: feat/wiper-driving-control-row. Move shared wiper-key row from Controls
landing page into the actual driving bindings list beside camera/look-back.
Retain extension INI key, never enlarge nine-key packed save layout. Both player
binding pages expose same global wiper binding; not independent new per-player
wiper actions. Preserve release-before-capture, Escape/conflict behavior and
actual merged cockpit update. Include current unsaved driving edits in conflict
checks. Tighten list spacing32 vs36 to retain Save row, verify actual draw layout.
Verified actual Menu_Update selection into W capture from both player pages,
release-before-capture, unsaved edited driving-key rejection, Escape, preserved
nine-key tables. 88/88 Linux tests pass. Actual draw-call layout fixture confirms
keyboard11rows+joystick12rows fit480p at32px spacing (last baseline442 for12).
Synthetic preview inspected; native atlas/Mac pixels still acceptance gate.

## Tracking-drone framing and terrain follow-up

The optional human-operator camera still has its independent world position,
velocity, orbit heading and steering-sensitive lens. It now follows closer
(9-14 nominal units, height 7-10, 1.3x transient tether), aims ahead, and keeps
normal-projection player framing near the lower third. Saved custom pitch/FOV
can intentionally change that framing. Other camera modes and saved settings
are not changed.

Simulation update checks collision-ground height along two 24-sample corridors
(player to camera and forward aim to camera), plus the predicted camera point.
A bank can pull the camera closer along its inertial direction before requiring
more altitude. Rise clearance is immediate, descent limited to 2 units/second.
Grounded corridor endpoints prevent a forward aim buried in a crest from
creating runaway altitude. Previous lift is removed before advancing the free
world spring, then reapplied as a constraint. Draws never advance it.

This is heightfield terrain avoidance, not a general scene raycast. Off-mesh
samples are unknown, not a clear-view certificate. Very thin geometry, vertical
faces and tree/hedge billboards outside collision terrain are not guaranteed.
Native Mac validation on the user's bank/crest/fast-curve case remains required.
The synthetic framing probe is a diagnostic, not game footage.
