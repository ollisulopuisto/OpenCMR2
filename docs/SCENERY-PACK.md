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

## Keep the same driver profiles while using a separate pack

`--user-dir` selects settings, logs and save writes, not only assets. Player
profiles/progress use `pps/*.pps`; race saves use `gamesave/*.rcs`. Switching
the user directory can make a different profile set appear. Do not copy or
replace saves just to enable scenery.

Use `--scenery-dir /absolute/path/to/scenery-pack` to read ONLY scenery from
that folder while keeping the same existing launch/profile selection. The
folder itself contains `manifest.tsv`. This switch is per-launch, does not
persist configuration, create/move the pack or change save/settings/name roots.
Ordinary game reads/writes never search this asset root. A missing explicit
pack falls back to originals, not a pack from another profile. Existing
scenery-overrides opt-out is still respected. Without the switch, the old
relative `scenery_pack` behavior is unchanged.

Example only, after verifying the selected profile has the desired progress
and this asset folder exists:

```
./build/macos/src/opencmr2 --data /path/to/owned-game --scenery-dir /path/to/private/scenery-pack
```

Leave an existing `--user-dir` unchanged if that is the profile you use. If
you normally omit it, keep omitting it. No save migration is needed. The log
labels the separate asset root read-only; actual spectator-active diagnostics
still require a successful draw, not just a manifest read. Native Mac use
remains unverified.
## Source-fitted tree transition

Tree OBJ sidecars now require the first line:

```
# opencmr2-source-canvas-1 WIDTH HEIGHT
```

WIDTH/HEIGHT must match the original texture canvas. Geometry uses a full
source-canvas height of1: sourceU=.5+x*HEIGHT/WIDTH, sourceV=1-y. Depth is
scaled by the original card's height along its validated normal. The loader
solves an affine sourceUV-to-originalXYZ fit and applies the stage placement
basis once. This replaces the old global height scaling, not an extra scale.

Only finite planar full-canvas original cards with UV extents near0..1 qualify.
Cropped atlas variants, non-planar cards, invalid geometry, canvas mismatch
and legacy/headerless assets fall back to the original object. Fits and failed
fits are cached per mesh for the stage lifetime. Spectator animation is
unchanged and does not need this header.

The runtime safety/fit change alone is not a working new-tree installation.
The previous64-tree public patcher recipe emitted headerless ring assets,
which this loader deliberately rejects. Do not add a header to those old assets:
their geometry/UV contract differs and they must be regenerated. Their
spectators still load. Three private experimental candidates (md1/md4/fir1)
use the new contract; fr1 and cropped variants remain original. The migrated
source-only patcher generates those3 high-source mappings with4x interpolation,
not private restored RGB. Low variants are unverified and
remain original. Native Mac appearance/performance remain pending. Private
restored texture RGB is not reproducible from this source-only patcher
without its private restoration inputs. No derived artwork is included in Git.


## France1 road texture test (experimental)

`--textures france1` prepares2xfiltered `t_1a/b` high/low original textures with
full mip chains. `--textures original` or no flag retains original texture
selection. Exact supported DDS bytes and original France1 C3D are required;
other stages stay original. This is interpolation, not new/restored detail.
No generic filtering is applied to foliage, people, UI, fonts or other atlases.
Run the binary with your existing data/profile selection, for example:

```sh
opencmr2 --data /path/to/owned/game --textures france1 --prepare-only
opencmr2 --data /path/to/owned/game --textures france1 --roads curved --trees crossed --spectators animated
```

The second command combines opt-in tests; curved geometry currently supports
only the exact high-detailFrance1 quartet. Low-detailFrance1 geometry remains
original. The texture choice covers its own high+low DDS identities separately.
Private caches are versioned, source/output-hash checked and never replace old
packs. Source directories, saved driver and original texture dimensions remain
unchanged; renderer replacement is removed at stage teardown. Python with Pillow
must already exist; this texture helper does not install anything. A complete
playable installation is needed for visual comparison. Linux tests and upload
readback do not establish native Mac appearance, sampler seam quality or frame
time. Hedge/bank depth is a separate experiment, not included in this option.

## Wiper and driving-key remapping

Options > Controls now has a separate Wiper key row. Select it, release the
confirm key, then press a new physical key. Escape cancels. Keys already used
by driving are refused instead of erasing acceleration/steering bindings.
This option saves separately from the original nine-key Controller.rcf record.
Driving keys stay on the selected controller's configuration/bindings page.
The default is physical W (DIK17); an explicit saved wiper key is retained.
If an active keyboard uses W for driving and no explicit wiper key is saved,
the implicit W shortcut is suppressed. Choose an unused key in Controls.
A rising edge toggles continuous wiping, even with dirt strength zero.
Native menu rendering and navigation still require verification on real game data.
