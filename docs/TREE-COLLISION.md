# Tree collision safety

The optional `video.tree_break` setting no longer forces sector objects to break
on broad bounding-sphere overlap. Current runtime records have no verified
breakable-tree classification. A tree-like texture name, a replacement visual,
or sufficient car speed is not that classification.

Static and moving objects use the original detailed contact tests and response,
regardless of this setting. The reserved broken flag no longer bypasses collision
or activates the old falling transform. The old transform mixed float drawing
matrices with fixed-point math. Existing settings and original assets stay intact.

This is a safety fix, not completed destructible trees. A future implementation
needs an explicit source-backed contract, a contact-only check before committing
state, and a correct per-instance visual fall transform. Native visible-trunk to
collision-shape alignment still needs verification independently of this fix.

`tree_collision_runtime` executes the real sector-object collision loop with
synthetic sphere and car-box geometry: a broad-phase miss, a broad overlap that
misses detailed contact, and actual contact. It covers both setting values, low,
threshold and high speed, and clear/reserved broken flags. It checks identical
contact results and impulses, unchanged speed, no new broken flags/fall states,
and restored detailed contact even with the reserved flag present. It does not
verify native assets, gameplay feel or all moving/oriented collision cases.
