# Atmosphere: fog and air motes

Optional, default off. Requested as "more atmosphere - motes, fog, light cutting
through it", deliberately overdone first and then brought down. Everything here
is visual only: no physics, gameplay, saves, original assets or default settings
change. Both effects work on the SDL_GPU (Metal) and GL paths.

## What it is

- **Fog** (`video.fog`): one extra full-screen post pass (scene + depth in) that
  runs before DoF and god rays so those stack on the fogged scene. Exponential
  distance fog, a ground-mist height gradient (0.55x at the top of the frame,
  1.45x at the bottom), a sky cap (the sky hazes but never becomes a flat wall,
  max 85 percent blend) and a sun-ward glow that brightens and warms the fog
  around the projected sun position. This is the "light cutting through" dial.
  It is screen-space glow in the fog pass, **not** occluded volumetric light
  shafts - geometry does not carve beams out of it. Existing god rays remain the
  occluded-shaft effect and stack on top.
- **Air motes** (`video.air_motes`): slow-drifting floating dust particles, a
  new particle type in the existing plume pool. Base cap 48 billboards at 100
  percent, 144 at 300. They drift with a slight sink at any speed and are
  deliberately distinct from the rejected speed-mote look (fast streaming
  specks). Spawn/step math is pure and unit-tested; they render through the
  existing particle draw path.

## Controls

All settings are [video] options, off unless enabled:

| Setting | Range | Default | Meaning |
| --- | --- | --- | --- |
| video.fog | 0/1 | 0 | fog pass |
| video.fog_amount | 0..200 | 100 | density; 100 is the deliberately heavy starting look (23 units ~50 percent fog, 60 ~84, 150 ~99) |
| video.fog_glow | 0..100 | 60 | sun-ward glow strength |
| video.air_motes | 0/1 | 0 | drifting motes |
| video.air_motes_amount | 0..300 | 100 | count/rate scale |

CLI: `--effect fog --fog-amount 100 --fog-glow 60 --effect air_motes
--air-motes-amount 100`. The `--preset overdo` bundle now includes fog at
100/100 and motes at 300, matching the overdo-first intent. The default
`--all-mods` preset does **not** enable them.

Menus: Advanced graphics > Effect amounts > Atmosphere... has all five rows
(its own submenu because Menu::items is layout-locked at 22 entries). The F10
tuning panel has the same five rows and applies live. The OSD effect line and
startup log show the active fog/mote state.

## Performance shape (unmeasured)

Fog is one full-screen pass over scene + depth - the same cost class as the
existing grade/god-ray passes, no volumetric marching, no extra scene render.
Motes are at most 48 billboards at 100 percent through the existing particle
path. Actual frame-time impact on the M2 is **not measured**: verify with the
F10 FPS readout and bring fog_amount down from 100 if it costs too much.

## Renderer notes

The SDL_GPU path recovers view distance from depth with the real near/far (same
formula as dof.frag). The GL executor does not have near/far at the post stage,
so it uses a documented raw-depth ramp stand-in (fog_math.h
Fog_FactorRawDepth); the fog band there covers roughly 9 world units to the far
plane. Fog needs a sampleable depth texture, like DoF. Pure maths reference for
both paths: src/render/fog_math.h, mirrored by src/render/shaders/fog.frag and
covered by tests/fog_math.cpp.

## Evidence and acceptance

- tests/fog_math.cpp: density anchors, viewZ round trip, height gradient, sky
  cap, sun glow falloff, raw-depth fallback.
- tests/air_mote_spawn.cpp: ring bounds, uniform disc density, spawn rate
  scaling, drift/sink/fade step, pool caps.
- tests/gl_effects_runtime.cpp (manual, `python3 tests/gl_effects_runtime.py
  --build build/linux --output out`): production fog.frag through the real GL
  executor on llvmpipe. Off = exact identity, near/cockpit-depth pixels
  unfogged, distant pixels fogged, ground-mist gradient, sky cap keeps 15
  percent of the sky, sun-ward glow localized, amount scaling, alpha preserved,
  plus stacked off/100/200 review frames (out-fog-road.ppm). llvmpipe pixels
  prove appearance and math, not native Metal FPS.
- Menu wiring covered by tests/advanced_menu.cpp; F10 rows by
  tests/tuning_panel.cpp and tests/tuning_runtime.cpp.

Not proven here: native Mac pixels and frame time, mote appearance in a real
stage, fog interaction with real skybox/weather depths, Metal MSL output beyond
compilation. Those need on-device driving, then dial the amounts down to taste.
