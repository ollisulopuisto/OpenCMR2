# Post-effect polish

This changes the existing optional DoF, god-ray and color-grading implementation.
It does not introduce MetalFX, force effects on, or replace any game assets.

## What was repaired

- DoF previously moved colors toward gray without sampling adjacent pixels. It
  now uses 16 golden-angle disk samples, a smooth focus transition and foreground
  rejection. Flat colors keep their brightness; focused pixels keep their detail.
- Rays previously brightened the whole frame, ran twice, and started from the raw
  scene, discarding earlier effects. One normalized 32-step pass now samples
  bright, unoccluded sky, decays along the ray and uses a restrained screen blend
  to avoid clipped white haze. It consumes the previous effect's output.
- There was no grading LUT. The HSL conversion changed colors even at neutral
  settings. Analytic RGB balance and luminance-based saturation now preserve
  neutral settings. The existing settings remain, including saturation/contrast
  as percentages (100 = neutral). Initial values and bypass checks now use those
  same units. Temperature/tint preserve black rather than adding colored fog.
- Depth effects need a sampleable depth texture. D32_FLOAT is selected only when
  the device reports support for both depth-target and sampler use. If unavailable,
  those effects are skipped while ordinary rendering and grading remain.
- New effect targets are released on resize before recreation. Two redundant
  ping-pong buffers are removed and a distinct grading target prevents feedback.
  Lens grime also avoids sampling its output and supplies its full uniform block.

## Taste and limits

The intent is less white fog, actual soft defocus and a subtle grade, rather than
turning the picture into a heavily processed screenshot. Suggested starting
values, not forced defaults: temperature 15, tint 0, saturation 105, contrast 103;
keep DoF modest during driving and rays low. Retain the user's existing settings.

Focus still uses the game's nonlinear normalized depth (0..1), not meters or a
physical lens model. The sun is an artistic fixed screen position (0.85, 0.15),
not a world-space sun projected per camera, and rays are a screen-space effect,
not volumetric scattering. Occlusion uses the depth buffer's far/sky region.
Actual sky depth and focus behavior must be checked with the game's scenes on
Mac. DoF includes HUD text; no HUD separation is claimed. Skybox depth conventions,
transparent foliage and weather may need further scene-specific work.

## Evidence and acceptance

The optional `post_effects_pixels` test compiles and executes the actual fragment
shaders through SPIR-V/GLSL on Mesa llvmpipe. It checks neutral grading, in-focus
preservation, real out-of-focus blur, flat-color preservation, fully occluded and
zero-intensity ray identity, and alpha. It fails on the old neutral grading and
passes on this implementation. Optional dependencies and invocation match
`tests/spatial_aa_shader.py`; run:

```sh
python tests/post_effects_shader.py --output /tmp/effects.png
```

The comparison is synthetic test scenery, not game footage. It was visually
inspected for smoother defocus, unchanged focused foreground, restrained rays and
subtle color. Complete Linux build and Metal MSL translation are not proof of
native Metal runtime correctness. Before merge, check actual Mac gameplay and
menus, both isolated and stacked effects, resize/fullscreen, sky depth, haloing,
text readability, and warmed CPU/GPU frame time. No FPS improvement is claimed.

Sources are the existing fork's shaders and renderer. Changes preserve its
options, not a quoted suggestion's assumption that a LUT or a true DoF blur
kernel already existed.
