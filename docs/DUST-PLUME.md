# Dry dirt/gravel plume, first visual implementation

Enable `[video] dust_plume = 1`; default0 preserves current rendering.
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
The plume is opt-in because its actual appearance/overdraw cost is unmeasured.

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
opt-in until a real game capture confirms density/placement and acceptable cost.
