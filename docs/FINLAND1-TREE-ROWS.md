# Finland individual-tree rows, first implementation

This builds the approved option-2 orbit direction, separately from the held hedge PR. It reconstructs each supported forest card as five individual trees with fixed bark trunks and separate volumetric crowns. There are no strip-wide walls or hedge caps. It is a visual replacement only. Original files, collision records and gameplay are not edited.

## Coverage and limits

Only `tresh1ba` and `tresh2fa`, from both verified Finland high/low texture archives, are mapped. They use the same high-source conifer/birch recipes as the approved orbit. The five placements are hand-authored representatives, not recovered hidden trees or automatic segmentation. Other strip variants keep their existing rendering. A material run is replaced only if all its quads can be fitted safely.

The crowns retain the orbit's inflated/banded appearance, especially the birches. No native Metal appearance, FPS, process memory, real-stage road-side fit or visual/collision alignment has been accepted. This is ready for code review, not a declaration that all forest conversion is finished.

## Prepare privately

From a source checkout with the scenery Python dependencies installed:

```sh
python3 tools/scenery-patch/tree_rows.py \
  --input-dir "/path/containing/owned/Finland/archives" \
  --output-dir "/path/outside/game-data/finland-tree-rows"
```

Input needs `finhit.bfl` and `finlot.bfl` directly under it. Each selected archive's indexed DDS entries are hash/dimension checked by the existing archive reader. Output must be new and outside input. Generation uses a temporary sibling directory, then publishes the complete pack. Nothing is downloaded. Do not commit or redistribute the output, including OBJ geometry, textures and previews.

## Try and compare

Use the manual scenery-pack route, not automatic `--trees` or `--spectators` preparation. The row pack contains only these rows, so individually placed trees and animated spectators are not replaced by this pack. Uprez must already be available through the normal owned-source texture preparation route.

```sh
./build/macos/src/opencmr2 --data "/path/to/owned/game-data" \
  --scenery-dir "/path/outside/game-data/finland-tree-rows" \
  --textures uprez --tree-rows 1 \
  --cockpit-extend-rear 80 --cockpit-door-extend 40 --fog-amount 20
```

For A/B, repeat the same replay/view/options with only `--tree-rows 0` changed. Default is 0; all-mods does not silently enable this pack. The rebuild script passes the binary switch through, but does not generate the private row pack. Look at front, oblique and side views of supported strips, both directions of the stage, and changing light/fog. Measure FPS/frame time and process memory with the same replay and camera, not different driving runs. Check visual trunks against existing collision separately; unchanged collision does not mean the new visible trunks are aligned with it.

## Placement and fallback

Both the original DDS identity and exact position/UV material-run hash must match. A verified cutout surface, normal scenery/tree/uprez gates, identity world matrix, a full-canvas planar quad and an unambiguous nearby route are required. Each tree keeps uniform proportions at the source card's height (or shrinks uniformly if its own crown cannot fit the width). Authored centres are fitted and clamped within the source span, so narrow source strips naturally produce overlapping crowns rather than squashed or half-height trees. Separate tree volume is placed entirely behind its original plane on the away-from-road side. Each tree base is anchored on the source card's sloping ground at its centre; trunks stay vertical and crowns are not sheared. Every mapped vertex is checked against all route segments, rejecting a row if its displacement would bring it closer to the route than its position on that original plane. A declined row keeps the existing strip path.

The renderer checks all quads before drawing any row in a material run. It preserves the original vertices/indices and reads their live lighting/fog colours for the replacement. Row normals use the fitted basis. No wind animation is applied to these rows in this pass. Texture state is restored after drawing. Stage clear drops row geometry and texture caches. Route pointer/count, asset mapping and geometry hash changes invalidate a cached fit; non-identity world transforms always decline. In-place stage route changes are expected to be accompanied by the existing stage-clear lifecycle, covered by a same-address reload test.

Offline check against the owned Finland1 high-stage metadata and 500 route nodes: 2 of 3 conifer quads and 3 of 10 birch quads pass the C++ fit/route checks, each in a separate material run. This is limited first-pass coverage, not measured runtime activation.

Generated rows cost 16,688 triangles (conifer) and 14,192 (birch), with 20 material groups each. Those are offline per-row counts, not measured runtime cost. The first implementation caps a material run at 1,000,000 expanded vertices and the stage row cache at 2,000,000. Reaching a cap keeps the existing rendering for that run. Cache memory is not a measured process-memory figure; rejected fit results are cached to avoid repeated expensive route checks. Coverage is therefore intentionally bounded. Optimization and broad variant coverage are follow-up work, not reasons to skip correctness checks.

## Checks

- Behavioural tests: full-canvas fit, uniform scale, away depth, route proximity, malformed/nonfinite geometry and fallback.
- Actual `SceneryPack.cpp` draw integration with synthetic-only source geometry metadata: exact identities, default off, source hash, world/route decline, cached/live colours, option changes and same-address stage clear/reload. No game art in tests.
- Converter tests preserve five separate trees, materials, UVs and rotated normals, with deterministic bounded output. Actual owned outputs were parsed with the C++ loader privately.
- Four standing checks: no public database access; tests per feature; bounded/sanitised CLI, file and geometry input; no hardcoded keys.

Unrun: native Metal validation, Mac local build, replay FPS, native memory, real-stage road-side fit and visual collision alignment. No merge until the review gates and owner instructions allow it.
