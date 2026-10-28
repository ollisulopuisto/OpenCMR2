# Dry dirt/gravel dust

`[video] dust_plume=1` enables extra dry dirt/gravel clouds and `dust_volume=1`
selects the GPU volume path where supported. Both default on at the user's
request; saved opt-outs are preserved. `dust_plume=0` stops new emission;
`dust_volume=0` keeps the legacy camera-facing cloud-sheet fallback. Neither
setting changes original game files. No artwork pack is needed for dust.

## Terrain-anchored rebuild

The GPU body no longer borrows the first visible particles or overlays the
same camera-facing cloud sheets. A dedicated fixed-capacity pool captures
world XYZ once at wheel contact. It has no car/camera owner or inherited car
velocity. Simulation advances its age, never its horizontal anchor. Repeated
camera draws only sample it. Nearby fresh emissions merge without moving the
stored position. Both particle-clear and particle-kill-all clear the trail.

Each cloud lives up to12 simulation seconds, expands to roughly9-15 world
units in radius, and fades. World units are treated as metres for synthetic
scale comparisons, not a new measured native gameplay guarantee. The pool
holds128 anchors, overwriting oldest slots when full; at most32 are sampled
across the active pool per draw. Fixed position-derived variation changes
billow size/height/density without consuming the game's random stream.
World-fixed noise erodes the broad, ground-hugging bodies. This is visual
volume, not fluid simulation, lighting/scattering or terrain collision.
Ground height is captured at emission, not continuously sampled across slopes.

A preflighted GPU volume command replaces only added DUST_PLUME cloud sheets.
Original chips/spray remain. Missing sampled depth, shader/pipeline/target
failure, OpenGL, offscreen targets, split viewports, orthographic and singular
camera matrices reject the command and retain sheet fallback. A rare GPU pass
failure after command acceptance cannot restore already skipped sheets in that
frame. First successful encoding logs `world dust volume encoded` once; startup
requested settings alone do not prove rendering.

The half-width/half-height density pass uses32 ray steps and up to32 lobes,
then a depth-weighted full-resolution composite. A cheap radius check avoids
noise lookups outside each lobe. The doubled lobe bound still increases worst
case work. Native Metal/M2Air frame time is not measured.

## Fallback and material scope

The legacy sheet path still uses the existing smoke texture and400-particle
pool, capped at96 added clouds with64 slots reserved for original effects.
It has independent spawn randomness and no owner callback, but its sheets do
face the camera. Dry dirt/gravel only: snow, ice, water and hard surfaces are
excluded from added clouds; original effects remain. Snow needs separate
calibration, not a universal low-volume rule.

## Evidence and pending acceptance

CPU tests cover fixed horizontal anchors, expansion, expiry, bounded sampling,
read-only camera sampling, sheet replacement/fallback and stage clear. A real
renderer-recorder test covers32-lobe snapshot ownership and unsupported,
split, orthographic and singular-camera rejection. Software-GPU tests compile
and render the production fragment shaders, checking orbit/translation,
determinism, opaque depth rejection, neutral composite and alpha preservation.

Reproduce the optional moving-camera synthetic road proof with:

```
./build/tests/world_dust_test --fixture /tmp/dust-pool.json
python tests/dust_volume_shader.py --pool-fixture /tmp/dust-pool.json --output /tmp/dust-orbit.gif
```

The scene uses actual C++ pool output, a curved road, a checker ground plane
and a4.2 x1.8 x1.5 world-unit white box as a car-size cue. It is not gameplay.
Dependencies: moderngl, numpy, Pillow, glslangValidator, spirv-cross and EGL.

Correctness evidence is separate from appearance acceptance. Uneven WRC-like
billowing/opacity and native Mac ground/projection alignment, camera cuts,
replays, HUD/depth ordering and frame time still need real game captures.
The rebuild remains under visual review, not a completed WRC-quality claim.
