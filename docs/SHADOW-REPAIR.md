# Optional directional-shadow repair

This fixes the existing optional shadow path, not a new default-on effect.

The previous pass declared no vertex input for a position-reading vertex
shader, never bound geometry/index buffers, and tried to include HUD and
render-to-cube-map commands. The main shader declared eight samplers but the
GPU shader was created with seven, while draw submission bound nine, including
the actively written depth attachment. Those resource declarations now agree.

The repaired path binds 48-byte 3D positions and indices, filters HUD/offscreen,
non-depth-writing, blended and alpha-tested draws, fixes row-vector matrix
composition and the D3D depth/top-left texture conventions, and stores shadow
depth in R32F rather than eight-bit color. Coverage-aware tree/transparent
shadows are intentionally not claimed. Existing game shadows remain intact.

Shadow map sampling clamps at edges, retains base/ambient detail rather than
turning occlusion black, and fails back to no added shadow when format/resource
or pipeline creation is unavailable. Resize can recreate the map lazily.

## Acceptance and limits

Linux compilation and shader translation are checked. Optional EGL pixel tests
run the real fixed fragment shader with synthetic inputs and verify signed Y,
D3D depth comparison, retained shadow contrast and unchanged alpha. They are
not real game screenshots or native Metal pipeline execution.

The existing fixed light direction, 300-unit box centered on stage origin and
1024 map size are unchanged. This is not a moving camera-following cascaded
shadow system. Only triangle-list opaque casters are accepted; no new
alpha-coverage handling, animated caster history or performance claim.

Native gameplay still needs inspection with shadows explicitly enabled,
including track coordinates, partial-frame flushes, cars, menus, cube maps and
resize. Keep the optional effect off if that inspection shows artifacts.
