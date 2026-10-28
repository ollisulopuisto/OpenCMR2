# Dry dirt/gravel plume, first visual implementation

Dry dirt/gravel clouds now default on, as explicitly requested. Set
`[video] dust_plume = 0` to disable the added clouds. Existing explicit values
are respected; this changes fallback defaults, not saved settings.
Uses the existing smoke texture and billboard/depth-tested/no-depth-write path.
No downloaded artwork, scene collision changes or volumetric simulation.

Each dry dirt/gravel wheel contact can emit a cloud in world space. It has no
car attachment or owner-position callback, so old bends remain dusty after the
car moves away. Motion decays toward existing wind, rises, expands over150fixed
particle-update steps and fades in/out. Real seconds depend on game stepping.
Original near-wheel debris/water/snow remain separate. Snow, ice, water and dry
hard surfaces are explicitly excluded from this new cloud, not recoloured.
Snow requires separate later calibration, including substantial powder clouds;
this patch establishes no universal low-volume snow rule.

Bounded to96clouds inside the existing400particle pool, leaving64free slots
before cloud emission can occur. No pool growth or evicting original particles
for clouds. Lower quality modes naturally have a smaller budget. A private RNG
for added-cloud spawn spread avoids advancing the game's global random stream.
Its actual appearance/overdraw cost is still unmeasured on native Mac.

## Evidence and remaining acceptance

CPU tests run actual growth/advection/fade code, dry-surface exclusions and pool
spawn code including96cap/64reserve/world-space birth. Synthetic motion views
use actual C++ simulation output with a generated soft mask, NOT the uninspected
game smoke texture or real gameplay. They verify persistence behind a moving
emitter and eventual fade, not WRC-level visual quality.

Still needs native gameplay: texture softness/colour, cloud density against
terrain, depth intersections, billboard transparency ordering, near-camera
overdraw, camera cuts/replays, snow separation and M2Air frame time. There is no
soft-depth intersection, light scattering, volumetric shadow, GPU simulation or
FPS claim. Existing billboard sorting/material limits remain. Keep this draft
under visual review until a real game capture confirms density/placement and acceptable cost.

## Experimental world-density GPU body

`[video] dust_plume=1` plus `dust_volume=1` adds a world-space ray-integrated
body behind the original sheet/debris fallback. Both now default1 by explicit user request. Existing explicit0 settings are
not overwritten. Set dust_volume=0 to retain ordinary sheets without the GPU
body, or dust_plume=0 to stop new dry-cloud emission. This is an early prototype, not a finished correction of all
camera-facing dust: the ordinary billboards still coexist. Sixteen active
lobes maximum,32steps, depth-clipped, fixed world-space procedural noise.
The new path snapshots the camera at particle draw submission, executes before
later HUD commands, uses a separate colour target and shared depthless pass
helper, and samples scene depth without attaching it for writing. OpenGL,
missing sampled depth, resource failures and non-fullscreen viewports skip it.
No external texture pack is installed. Density is procedural, not the actual
asset alpha used in the earlier offline comparison. The shader's light/tint is
simple, with no real scattering or volumetric shadows.

This first wiring prototype runs at half width/height with linear upscale.
It stores scatter/transmittance separately and uses a depth-weighted
four-tap full-resolution composite, preserving untouched scene pixels.
Foreground halos and undersampling still need native checks.32×16
worst-case samples can still be expensive. M2Air acceptance remains needed,
along with lobe
selection by camera relevance instead of first16pool entries, replay/reset,
viewport orientation and native projection/terrain/HUD integration still need
acceptance. Native performance remains unverified despite the requested default change. Tests are software-GPU pixels and CPU matrix
contracts, not native Metal gameplay or a frame-time result.
