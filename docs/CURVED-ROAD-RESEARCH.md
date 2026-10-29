# Local curved-road contact research

Not a runtime feature and not ready to enable. Original behavior is the default.
`Track_ResearchExactEdges` has no game/UI/CLI caller; only an offline probe enables
it against private generated collision data.

A source-matched adaptive curve experiment found a fixed-point shared-edge hole:
the original separate shifted products can produce -1 on both sides of an edge.
The local experiment uses exact 64-bit determinants, rejects unsafe coordinate
deltas and degenerate/reversed projected triangles, and adds no tolerance.

The actual game collision query code was linked into an offline probe. With the
original edge test, 52,396 whole-stage sample queries produced one NEW off-mesh
sample after curvature (19 original vs 20 modified). With exact research edges,
the same queries produced 12 original and 12 modified off-mesh samples, none new.
The curved collision geometry changed 30 sampled surface IDs and had a maximum
height change of about 12.7 cm. This is a handling change, not zero-impact polish.

Sampling is not exhaustive. No native driving, frame-time, FIN barrier alignment,
vertical-layer/junction acceptance, original profile integration, binary CLI,
cache/source identity or opt-in contract is supplied by this experiment. These
must be resolved before a runtime patch can be proposed. No proprietary stage
files are included here. The synthetic seam regression does not prove any stage
is safe to change.

## Matched-road opt-in/cache integration preparation (local, unpublished)

`tools/scenery-patch/road_contract.py` is an unused private research validator,
not a runtime feature or alternate launcher. Original four-member hashes are
pinned in source, not accepted merely because a supplied manifest asserts them.
The cache key includes validator recipe bytes, exact source and output inventory.
Original HPC, FIN, SRF and render C3D identities must all match before staging.
Cache outputs are rehashed on reuse, symlink/path escapes and source/cache overlap
are refused, and an actual bounded HPC parse checks counts and indices against
its manifest. Atomic private staging never emits `OPENCMR2_PACK` or a runtime
`manifest.tsv`; it cannot be mistaken for the current scenery helper's pack.

Actual retained Ballon bytes and the private adaptive HPC passed this validator
and reuse check. This validates artifact identity/structure, not matched visual
runtime behavior. `require_runtime` always refuses. FIN remains original with
unverified alignment. No FIN warp or made-up collider categorization is allowed.

Current binary contract:

- `src/platform/enhancements.h`: only tree/spectator/name/car/effect selections.
  There is no road CLI choice, saved road key, or silently active road path.
- `PrepareEnhancementPack` in `src/platform/sys.cpp` launches the selected
  executable-adjacent/source helper, uses the valued profile's enhancements
  cache, and activates a returned scenery pack only through its existing marker
  and `manifest.tsv`. It does not load HPC/C3D pairs. Do not put a road HPC into
  this texture-sidecar namespace or spoof that marker.
- `tools/scenery-patch/prepare.py` re-verifies originals, keys recipe bytes,
  stages atomically, and rechecks output hashes. The road research validator is
  separate until all four original member identities and joint render/contact
  outputs have a production contract.
- `game/Race.cpp` loads the original stage `.hpc` into its allocated stage
  block before `StageTiming_IndexSerializedStageTables` records pointers into
  it. Replacing only global vertices/triangles is unsafe: counts, quadtree node
  tables/list references and lifecycle all belong to that same block.
- `TrackCollision.cpp` loads `%sfin.dat` separately. This is not covered by HPC
  topology or point-query success. Barrier alignment must be grounded separately.

Next production design must jointly select render/contact data, validate all
source identities before activation, reset the exact-edge research switch on
stage release/failure, and fall back to the original *pair*, never half a pair.
The binary should eventually list only genuinely available choices and prepare
through `--prepare-only` without touching saves. Until native/FIN/render-loader
and lifetime work is complete, adding an apparently working `--roads curved`
option would misrepresent availability. It is deliberately absent.

Open acceptance gates: exact source-to-render loader mapping and joint output,
FIN classification/alignment, dense seam/overpass tests, native driving/height
changes, memory/frame time, explicit original opt-out and failed-pack fallback.
Grading and reactive rendering are separate pending lanes, not covered by this
road-only preparation.

### Actual render loader and remaining paired artifact gap

`Race_LoadStageModel` (`game/Race.cpp`) fetches `TEMP.C3D` from the same stage
archive and calls `Sector_BuildC3DModelScene`. The latter validates `PP_F` and
calls `Sector_RelocateStageMeshFile` (`game/Sector.cpp`). That loader does more
than upload positions: native scene/mesh/object/sector/collision records are
allocated and attached to the original file buffer, while pointer-free triangle,
vertex, light-level and vertex-flag arrays remain owned by that buffer. All ten
triangle texture slots are relocated by the current texture count, textures are
loaded from the original archive, and parts are rebuilt after vertex upload.
Repeated relocation of one mutable C3D block is therefore unsafe.

The current adaptive GPU NPZ is *not* a replacement C3D: it retains positions,
UVs and one material slot for the comparison, not full original triangle flags,
ten texture slots, per-corner colour bytes, light levels, vertex flags, normals,
portal records, bounds or native attachment lifetime. No runtime pair exists
yet. A C3D writer must preserve those original records and interpolate only the
correct attributes, then validate the real loader and scene in isolation before
CLI activation. GPU comparison success cannot substitute for this work.

`Sector_RebuildStageGrid` links original FIN sector indices directly into the
loaded sectors. `Sector_FreeMeshPartLists` clears scenery state and generated
parts on release. A joint road session must be prepared before *either* loader,
retain original archive identity for texture routing, own independent fresh
mutable buffers, and release them after native attachments/parts are finished.
Do not put a reset in the middle of two loaders or leave exact-edge state alive
on failed/unsupported stage loads. These lifetime semantics are a design gate,
not an implemented transaction.

### Private serialized pair experiment, 29 October

A local restricted Ballon writer now expands the actual source C3D, not just
comparison NPZ: 13,181 -> 15,379 render triangles, 23,780 -> 25,976 mesh-local
vertices, largest mesh 438 vertices (actual uploader limit is below 2,000).
Original per-mesh vertex/light prefixes are retained byte-for-byte. It copies
all triangle flags/ten texture slots, unchanged texture records/names and sector
records, interpolates both UV sets, packed colour channels and light levels,
and normalizes new edge-average normals. This is a private inactive artifact,
not an installed loader or a general C3D converter. Interpolation is a proposed
policy, not native visual acceptance.

An encoding discrepancy was caught: new HPC16.16 and C3D float32 coordinates
differed by up to 0.086mm. New shared points were canonicalized to coordinates
exactly representable in both encodings (maximum shift from initial target
0.087mm). Re-read serialized C3D has all 15,297 HPC triangles exactly matching
at fixed-point positions, plus 82 original visual-only triangles. No original
vertex was changed. This comparison is against the *canonical* collision pair,
not against the earlier float64 offline target, which remains different.

Actual game contact query probe on 52,396 original stage barycentric samples:
12 offmesh original and 12 canonical, zero newly lost samples, 30 surface changes,
maximum height change 0.127274m. This supports only those samples with the
explicit research exact-edge switch. FIN remains unchanged/unverified. Native
loader upload, scene attachments, culling and driving remain open; standalone
GPU rendering of re-read serialized positions/UV/material does not test them.
`Sector_ComputeBounds` derives terrain sector extents from loaded mesh vertices;
portal and sector identity are retained, but culling still needs native testing.

The local validator staged a joint two-output research inventory with both
canonical HPC and C3D hashes. Runtime is still unconditionally refused. The
validator bounds HPC structure, not full C3D semantic correctness; paired
serialization audit and the restricted writer are additional local evidence,
not permission to load an unchecked supplied C3D.

### Actual native-record loader isolation

`tests/road_loader_probe.cpp` is a Linux manual diagnostic, not a launch mode.
On independently hash-verified original and canonical private C3D artifacts it
ran three fresh buffer load/unload cycles each through the actual game library:
`Sector_RelocateStageMeshFile`, `Disk_Unpack`/buffer attachments,
`Mesh_BuildParts`, `Sector_ComputeBounds`, `Sector_FreeMeshPartLists`, and
`CFileBuffer::FreeGenericFileBuffer`. It checks 174 meshes, 800 sectors,
56 texture records, vertex and part index bounds, 50-part array capacity,
and nonzero texture-index relocation. Canonical totals: 25,976 vertices,
15,379 triangles, 46,137 part indices. The full 78-test suite also passes.

Only archive texture loading and GPU vertex upload are wrapped/stubbed. This
proves native-record relocation/parts and repeat load/free execution under that
boundary. It is not an actual GPU upload, texture-routing acceptance, Mac game
run, sanitizer memory proof, frame-time measurement, or FIN/barrier driving test.
The diagnostic must never receive an unverified arbitrary C3D: the original
loader has unchecked offset assumptions. No runtime hook invokes this probe.

Further runtime activation is gated on native graphics/handling evidence and
FIN correspondence. The private writer and canonical audit remain local review
scripts with retained source bytes; they are not advertised as a completed
binary option. No default data, profile, tree or crowd selection changed.

### Paired output validation and all-or-original transaction research

`road_pair.py` is unused by the binary. It validates bounded, supported C3D
layout/counts/offsets/mesh indices and checks every contact triangle against
serialized render positions with cyclic winding preserved (reverse winding is
refused), allowing exactly the known 82 visual-only triangles. The research
cache contract invokes this check whenever `curved.c3d` is inventoried; the key
includes both validator recipes. Actual canonical artifacts pass 15,297/15,379.

A pure immutable selection test transaction returns either both validated
candidate buffers or both original buffers, never half a pair. Nine tests cover
partial/missing/corrupt pairs, unverified source identity, mismatched positions,
reversed winding, NaN/index/offset limits, unsupported layouts, failed selection
after success, and immutable snapshots. This models fallback only; it does not
install a runtime hook or validate asynchronous game-loader rollback. Runtime
is still refused by the source contract. Full test suite: 79/79 pass.

### FIN correspondence boundary

Exact original source bytes contain 2,462 FIN records. A whole-stage exact
endpoint-pair comparison found zero FIN segments identical to a ground mesh
edge, both in full fixed-point XYZ and in fixed-point XZ alone. This does not
mean the edges do not correspond visually: it means direct identity cannot
supply a safe warp. No nearest-neighbor association was promoted to ownership.

Within 40m in XZ of the comparison center, 24 records occur (including duplicated
segments in different sectors): ten type12, twelve type14, two type15. This is
an XZ-only window, different from the earlier 18-record 3D window; neither is a
whole-stage count. Nearest midpoint is about 5.10m in XZ. Local records use
-1000/+1000 Y values, consistent with `StageObject_IsCarOutsideCollisionHeightInterval`
reading these as a vertical interval, not ground endpoint heights.
`Collision_TestCarAgainstSectorEdges` sends types12/14/15 through its default
physical-contact branch and `Collision_ResolveSectorEdgeContact`, after a height
interval test. They cannot be ignored as decorative/nonphysical records.

Blocked at this boundary: no exact segment-to-ground-edge mapping or justified
semantic class permits relocating these segments. A terrain deformation does
not authorize moving barriers. Keep FIN original and runtime off; source-backed
geometry correspondence plus native visible/collision acceptance is needed.

Morning validator hardening: the standalone pair-selection path now validates
the complete HPC reference tail, quadtree child/leaf spans and grid dimensions,
not only triangle geometry (the cache contract already checked those tables).
Candidate byte snapshots are frozen before validation, so selected buffers are
exactly those checked. Ten transaction cases, nine cache-contract cases, actual
canonical pair check and full 79/79 CTests pass. This closes a research-only
fallback-path validation gap; no runtime or publication clearance is implied.

## Requested CLI option (supersedes earlier inactive-road status)

The user's later explicit request adds `--roads original|curved` to the binary.
No flag/default and explicit `original` retain original geometry. `curved`
prepares the reviewed Ballon pair from exact owned `fra01hi.bfl` members using
`road_prepare.py`; requires existing Python3.10+ with numpy, downloads nothing.
It is independent of scenery sidecars and the save profile is not changed.
`--prepare-only` prepares/verifies the same pair and exits without game launch.

The helper reconstructs topology/attributes from source bytes, not retained
research NPZs. Both generated hashes must equal the reviewed canonical outputs.
Cache recipe includes all three source-only modules; reuses only verified bytes.
Missing source/dependencies/preparation error stops that launch with an error,
not a misleading curved success. No existing original data/cache is overwritten.
Other stages during a prepared curved launch stay original, with a diagnostic.

Before either stage loader, `CurvedRoad_Begin` verifies all four actual live
archive member hashes and exact paired output hashes. It allocates both fresh
buffers and acquires a release callback before publishing either pointer.
Callback registration failure, source mismatch, partial/tampered pair or failed
allocation keeps both originals. Release callback is registered before mesh and
sector callbacks so LIFO cleanup frees generated parts/native attachments before
its source buffers. Research exact-edge query state resets on release/next stage.
A stale active session is not reused by a different stage. Nothing changes FIN.

Actual binary help and `--roads curved --prepare-only` passed against a synthetic
archive containing the independently verified owned original members. This is
provider-independent recipe/dispatch proof, not proof of the user's native install
path. Session probe passes wrong/unsupported source, callback exhaustion, paired
acceptance, missed-release guard, release and missing-pair original fallback.
Actual loader/parts/free probe passes regenerated serialized render three times.
Full suite 84/84 passed before final callback test extension, then rerun recorded
with the implementation report. No Mac native driving/graphics/performance claim.

Example from a repo checkout with an existing numpy Python environment:
`<OpenCMR2 executable> --roads curved`
Use the actual executable path produced by the local build. `--data` remains
available for an owned installation; no different `--user-dir` is required.
The feature is experimental: source-exact Ballon only, barriers original,
height/grip boundary changes possible, native performance/driving unverified.
