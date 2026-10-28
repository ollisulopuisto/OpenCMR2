# Reversible scenery packs

The external pack loader is visual-only. Original archives, mesh pointers,
collision records, placements and physics stay in place. No private game art
is included in the source repository. Generate the pack as `scenery-pack/`
inside a separate private user directory, outside the original game installation.
Run `opencmr2 --data ORIGINAL-GAME-DIR --user-dir PRIVATE-USER-DIR` to load it.
The default relative `scenery-pack` root is searched under that user directory;
no INI change is needed in a new profile. The private profile has separate saves,
settings and logs, leaving the existing profile untouched.

`[video] scenery_pack` can select a different RELATIVE folder under the user or
data directory. Absolute external paths are not supported by `Sys_OpenFile`:
it resolves game paths under those roots. The internal option name is
`video.scenery_pack`. `[video] scenery_overrides=0` disables the loader.
Missing mappings or invalid assets keep the original rendering. Only
single-source deferred cutout meshes qualify.

## Format

UTF-8 `manifest.tsv` has six columns:
`kind source_archive source_texture asset frames ticks`, separated by tabs.
`kind` is `tree` or `spectator`; archive is the exact lower-case archive leaf
including `.bfl`; source texture is its original leaf (DDS/TGA extension is
ignored to match the native loader's renamed paths). Archive identity comes
from the real archive load, not from the texture's guessed region.

Paths are relative to the pack root, slash-separated, without `..`, drive
letters or absolute roots. Duplicate source mappings reject the manifest.
Trees use triangulated OBJ with positive `v/vt/vn` indices, explicit `usemtl`,
a relative `mtllib`, and separate `newmtl` / `map_Kd` material textures.
Geometry is local X-right, Y-up, Z-depth. Original full cutout height is 1,
base is Y0, width is original image aspect ratio and X is centered. Transparent
source margins are part of that anchor. Runtime scales by original mesh height
and original placement basis, never by the camera's billboard basis.

OBJ UVs are standard bottom-up and may exceed 1. The loader flips V once for
its top-origin texture storage, then uses hardware repeat sampling. TGAs must
be uncompressed type2 32-bit BGRA, descriptor8 (bottom-origin, eight alpha bits),
no colour map or offset. The loader normalizes rows once. Point sampling and
alpha>=128 match the asset recipe; no alpha closing or extra aspect scaling.
Spectators keep original billboard geometry/UVs and whole atlases. `frames` is
an explicit comma-separated sequence of relative TGAs, not inferred adjacent
texture IDs. All frame sizes must agree; 64 frames maximum.

## Time and lifetime

`ticks` is positive authored 25Hz time units per frame, not render frames.
The race player simulation-step gate advances time by 25/player-rate units;
a 50Hz simulation contributes half a unit per step. Render, reflection and
repeated draw passes only sample. Stable placement-derived phase decorrelates
crowds without touching game RNG. At the default25Hz and ticks5, each frame
lasts0.2seconds. Pause inherits the race loop's step suppression. Replay seeking
does not rewind ambient crowd phase; it is not recorded in replay streams.
Stage unload destroys external textures/meshes and resets phase/mappings.
Textures are owned separately from the original 2048-slot texture table.

## Limits and acceptance

The supplied production pack covers64 individual-tree texture occurrences and
48 spectator atlas occurrences from the available France/Finland packs. It
leaves76 forest strips and6 shadow masks unchanged. The available archives are
not the full game's asset set. Tree depth is inferred, not recovered geometry.
Sparse branches, repeats and gaps need visual review. Spectators sway their
upper bodies; this is not authored limb animation or new3D models.

Source-level tests cover parsing, invalid paths/faces, explicit frames,
fixed tree versus billboard transforms, repeated draw sampling, rate conversion,
original mesh preservation, and clear/reload. Production-pack load checks do not
prove native gameplay matching, terrain placement, fog, broken-tree fall,
shadows, replay appearance or M2Air performance. Those remain native gates.

## Driver names

Driver names are an independent, optional sidecar, not part of the scenery
manifest. The original built-in names remain the fallback. See
[the driver-name patch instructions](../tools/driver-name-patch/README.md).
No original executable or archive is patched in place.
