# Finland1 strip hedge, first pass

The owner selected hedge extrusion for Finland1's flat forest-strip cards.
With verified uprez textures, `--strip-hedge 1` adds a shaded back card,
stepped top runs following a smoothed alpha contour, vertical risers between
steps, and end walls. Depth is 18% of source height, clamped to 0.8..4 world
units. The dark, flat end walls are intentional, as in the selected preview.
This is an artistic hedge body, not recovered tree geometry.

The original front geometry, collision, road edge and game files are
unchanged. Geometry is appended at draw time, cached per material run,
and submitted in the same draw as the existing strip relief. Both windings
are supplied for the new body because source meshes have different cull
modes. The caller's render state is not changed. Live lighting/fog colours
use all four source corners. The added body stays still when front-card wind
is enabled.

## Safety and incomplete coverage

The texture and position/UV source hashes must match the existing allowlists.
The world matrix must be identity. The existing road-side safety helper must
find a route within 60 units, a clear road-facing normal, and no approach to
another road section at its conservative 4.1-unit probe depth. Otherwise the
old relief remains. The maximum hedge depth is 4 units.

This is not an all-strips-complete claim. An offline run using owned Finland1
high-detail stage geometry, its 500 route nodes and a source-alpha grid found
1,381 eligible quads out of 2,432 full-UV strip quads in 786 material runs.
The native renderer has not confirmed that coverage. The offline mesh
appended 179,700 triangles across the entire stage, including reverse faces
and step risers. Existing relief accounts for 43,776 triangles across those
2,432 quads. These are whole-stage counts, not a per-frame cost or an FPS
measurement. Runtime uprez-alpha sampling and safety declines can change the
appended count. No additional draw call is added per material run.

The older `strip_sources.py` uses an incorrect interleaved stage-object mesh
stride for this archive. Its earlier 163-quad estimate must not be used for
cost reporting. The figures above use the actual `Sector_RelocateStageMeshFile`
layout: all 0x120-byte mesh records, then the 0xa0-byte object records, and the
texture-slot table rather than direct texture-name indices. Repairing that
older exporter is separate follow-up work.

## Validation

- Failing-first `strip_hedge` behavioural test, then green: back-card depth and
  UVs, contour height change, shaded caps, immutable source, away-only bounds,
  four-corner colour weights, output append contract and invalid inputs.
- CLI parser and rebuild dry-run tests cover the 0/1 switch and reject bad
  values. The all-mods preset includes `--strip-hedge 1`; a later explicit 0
  wins.
- Software renders were made from the actual C++ helper output with two owned
  strip textures, including oblique/elevated views. The owner receives these
  privately. No game art or extracted geometry is committed.
- Native Metal appearance: not verified. Native before/after FPS: not measured.
  This environment has no GPU device or complete game install/replay.

## Native A/B run

On the PR branch, run the same Finland1 replay in each launch. Keep resolution,
renderer, camera, weather, all other settings, and replay time range identical.
Use the normal profile so the same replay is available. Do not use video
export or attract capture while measuring, because those change timing.

Replace the data path below with the owned installation. Keep the same saved
VSync setting for both runs and record it. A VSync-capped result is only a cap,
not evidence that the added geometry is free. The F10 FPS/draw overlay can be
used to record the same warm-up and timed replay segment. Compare at least
three runs of each setting in alternating order and report median FPS and
frame-time spikes along with the GPU and display settings.

Build and run the old relief path:

```sh
./rebuild.sh --all-mods --data "/path/to/owned/CMR2" --cockpit-extend-rear 80 --cockpit-door-extend 40 --windshield-crack-overlay 1 --fog-amount 60 --strip-hedge 0 --run
```

Run the same binary and settings with the hedge body:

```sh
./rebuild.sh --assets-only --all-mods --data "/path/to/owned/CMR2" --cockpit-extend-rear 80 --cockpit-door-extend 40 --windshield-crack-overlay 1 --fog-amount 60 --strip-hedge 1 --run
```

Check for a `tresh strip hedge active` log line in the on run. Its absence
means activation is unconfirmed, so a zero FPS delta cannot be attributed to
this conversion. Repeat the first command with `--assets-only` for the next
off run; do not change the branch or rebuild between A/B samples.

Rendering optimization is deferred until the remaining 2D scenery conversion
is complete. Measurements document cost without blocking that conversion
program. Safety and correctness issues still need fixing as they appear.
