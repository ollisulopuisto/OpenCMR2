# Local scenery asset patcher

Requires the source-fitted loader from PR27. This recipe creates only3
experimental high-source tree mappings, not a full64-tree replacement.

This is a procedural source patch, not an xdelta/bsdiff binary delta. It needs
original game files and cannot generate the artwork without them. GitHub gets
only this code and a filename/hash/dimension/classification index. Complete
modified assets stay in the player's separate private directory.

No textures, silhouette meshes, screenshots, BFLs, EXEs or binary extracts are
included. Local outputs are derived game artwork. This tool is not permission
to redistribute them and does not provide copyright or game-license clearance.
The new code follows this repository's GPLv3 license; that does not license the
game's artwork. No third-party executable is executed or distributed.

## One binary command (preferred)

After building a revision with enhancement CLI support, run:

```sh
./build/macos/src/opencmr2 --help-enhancements
./build/macos/src/opencmr2 --data "/path/to/owned/game-data" --trees crossed --spectators animated --driver-names real
```

The binary verifies originals, prepares or reuses a private versioned pack and
starts the game. Use `--trees shell` for the experimental tiered shell instead;
`--trees original` and `--spectators original` disable those kinds for this
launch. A newly prepared pack activates only the kinds explicitly requested,
so add both flags to enable trees and people together. `--prepare-only` prepares
and validates then exits without running the game. No selection is saved to INI.
No new launcher config or remembered path is created. Existing `--data`/folder
selection behavior stays as it was.

Newly generated packs also include source-identities.tsv with exact original DDS
byte hashes. The renderer captures identity when a texture loads, including loose
textures and stage-specific archives. Filenames alone never authorize a new-pack
replacement. Old packs without identity records keep their strict archive-based
matching; regenerate using this revision into a new cache for corrected routing.
An active draw marker proves the override was submitted, not native appearance or
frame-time acceptance. Animated spectators remain subtle sprite upper-body sway.

Generated cache is under `enhancements/` in the already-selected user profile;
this does NOT change the user/profile root. It never moves or overwrites an old
pack, original archive or save. Each reuse rechecks original hashes and every
generated file. Changed/incomplete cache fails rather than overwriting it. A
changed recipe creates a new version. Old caches are kept, not auto-deleted.
The old manual `--scenery-dir` route below still works, but don't combine it
with automatic tree/animated-spectator preparation flags.

Preparation uses the shipped source-only helper beside the executable. Source
builds also find the checkout helper and reuse its existing `.venv`. Python3.10+
with Pillow/numpy is required for scenery; no artwork is shipped or downloaded.
If dependencies are missing, the binary stops with an explanation. To explicitly
allow isolated Python/package setup using an already-installed `uv`, append
`--install-enhancement-deps` to the same binary command. This may download
Python3.12, Pillow and numpy from their package sources. It does not install uv,
change a global Python, replace an existing venv, or download game/mod assets.
Without that flag no Python/dependency download is allowed. Internet/package
availability and native Mac execution remain separate checks.

`--driver-names real` uses the15previously extracted display names directly,
without generating or changing a sidecar or player progress. `original` forces
the original table for this launch; `sidecar` selects the existing validated
`driver-names.tsv` route. No flags preserve the existing settings/sidecars.

### Local car alternatives

Use `--cars ALIAS[,ALIAS] --mods-dir "/path/to/owned/downloaded-zips"` in the
same binary command. `--help-enhancements` lists the11known alternatives.
The helper reads only exact hash-matched local ZIPs and verifies each required
member, then creates/reuses a private read-only car-file overlay. It does not
modify original or mod files and does not download anything. Keep original ZIP
filenames. Only one205and oneSubaru alternative can be selected; conflicting
choices stop before writing a pack. Six other replacement slots are independent.
No extra roster slots are added. The models, information and textures from each
selected ZIP stay together; `.cin` information may change physics/performance.
Only the supplied file variants are replaced (some are high-resolution only).
This is import/hash evidence, NOT native model, damage, physics, replay, UI or
render acceptance. HQ Cars, CMR2.5 and V6 automatic import is unsupported.

Use `--cars original` or omit the car flag next launch to use the selected
base data without this overlay. Existing mods already inside the chosen
`--data` remain part of that base, so "original" here cannot undo files you had
previously merged into that game installation. No original car restoration is
claimed. Author licenses still apply; no derived assets/readmes/Drive URLs are
included publicly, only source filenames, byte counts and hashes.

## Manual macOS / Linux fallback

Use Python 3.10+ and `uv` in the checked-out source directory:

```sh
uv venv tools/scenery-patch/.venv --python 3.12
uv pip install --python tools/scenery-patch/.venv/bin/python -r tools/scenery-patch/requirements.txt
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/convert.py --input-dir /path/to/owned/game-data --output-dir /path/to/private/user-dir/scenery-pack --check-only
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/convert.py --input-dir /path/to/owned/game-data --output-dir /path/to/private/user-dir/scenery-pack
```

`uv pip` installs dependencies into this local environment. Dependency install
may use package repositories; the patcher itself has no network calls. These
commands are designed for Mac/Linux; actual Mac execution remains untested.

Input must contain exactly one each of `frahit.bfl`, `fralot.bfl`, `finhit.bfl`
and `finlot.bfl`, recursively. All194 supported texture hashes and dimensions
are checked before output. Other releases reject rather than guess. Symlink
archives/directories are not followed. Output must be outside the input tree
and new or empty. Allow about110MB plus temporary working space. Originals are
never written. Temporary output is committed only after successful generation.

The output has3 archive-qualified tree mappings: `frahit.bfl/tre_md1.dds`,
`frahit.bfl/tre_md4.dds`, `finhit.bfl/tre_fir1.dds`. All other61 individual
trees, including low variants and rejected fr1, remain original, along with
76forest strips and6shadow masks. Full-canvas geometry is fitted by the engine;
cropped/nonplanar/invalid cards fall back to originals. Each tree includes
versioned source-canvas header and recipe metadata. Atlas uses continuous
cross-projection rather than repeated stripes; trunk samples opaque source
stem pixels.4x premultiplied interpolation is NOT generated new detail. The
private restored-detail candidate is a separate pack and is not bundled here.

All48 spectator atlases still have12frames each, manifest and coverage.
People have upper-body sway and2x nearest upscale, not new3Dmodels or arm
animation. Atlas layout/feet stay fixed. `ticks=5` is interpreted by the
engine loader, not render FPS. Missing/legacy tree assets never disable
working spectators.

## Use and undo

Build current `main`; the source-fit and separate read-only asset-root loaders
are merged. Generate into a NEW folder outside the original game installation.
Do not overwrite a previous pack. For example, after the setup commands above:

```sh
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/convert.py --input-dir "/path/to/owned/game-data" --output-dir "/path/to/private-assets/scenery-pack-sourcefit-v1" --tree-geometry crossed --check-only
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/convert.py --input-dir "/path/to/owned/game-data" --output-dir "/path/to/private-assets/scenery-pack-sourcefit-v1" --tree-geometry crossed
./build/macos/src/opencmr2 --data "/path/to/owned/game-data" --scenery-dir "/path/to/private-assets/scenery-pack-sourcefit-v1"
```

Replace these example paths with your own. Omit `--tree-geometry crossed` for
the default shell experiment. Neither option is native-accepted. The new folder
must contain `manifest.tsv` directly, not another nested `scenery-pack` folder.
Do NOT add `--user-dir` to load artwork: it selects a different driver/save and
settings profile. `--scenery-dir` only reads assets and keeps the default
profile, progress, settings and logs where they already are. It does not copy,
move, migrate or overwrite anything. A previously chosen explicit `--user-dir`
can stay only when you intentionally want that profile.

The earlier pack had112entries (64trees+48spectators). This recipe produces51
(3experimental trees+48spectators). Headerless old tree OBJ files are rejected
on purpose; adding a header by hand cannot repair their geometry. Regenerate
from owned originals into the new folder. The existing pack remains usable
for its spectators, but it is not compatible tree geometry.

A parsed manifest is not proof of an active tree. Check stage-run logs for
`tree sidecar active (source-fitted successful override draw)`. Spectators have
a separate success line. The loader names rejected archive/source/asset and
reason, such as missing source-canvas header, dimensions, invalid source card,
or texture preparation. Cropped/nonplanar cards deliberately stay original,
so even a valid3mapping pack will not replace every placement.

To undo, launch without `--scenery-dir` (or select the previous folder). Existing
relative-pack settings can still load another pack; `scenery_overrides=0`
disables all overrides if needed. Do not change save roots as an undo step.
Keep generated artwork out of Git and public releases.

Source hashes originate in four privately supplied original archives.
The3 deterministic tree candidates were offscreen-GPU checked from their
exact original high-source RGBA, original XYZ/UV fit and stage scale, at four
angles. Those pixels are not native gameplay acceptance. This source revision also tests missing, corrupt and duplicate inputs,
unsafe paths, hash/dimension mismatch, no-output checks and atomic generation
with synthetic artwork. Native game matching, terrain, fog, shadows and Mac
performance are separate acceptance gates, not proven by generation success.

Run the synthetic safety tests with the same environment:

```sh
uv run --no-project --python tools/scenery-patch/.venv/bin/python tools/scenery-patch/test_convert.py
```

## Optional fixed-cutout experiment

Add `--tree-geometry crossed` to generate the same3 verified source mappings
as four fixed intersecting planes instead of the default silhouette shell.
It is not camera-facing and is not a full3Dtree. Source proportions/placements
still use PR27's source fit. It uses64triangles for fir1 and80 for broadleaf
including one opaque stem, rather than roughly1400-1800shell triangles, but
transparent overdraw and native frame time have not been measured.

The shell has visible horizontal tiers; the crossed candidate avoids that
terraced silhouette but can show intersecting canopy seams and shape changes
as the view rotates. It is not an accepted visual upgrade. Broadleaf canopy
alpha excludes its lower central stem and bottom6% to avoid repeated trunks
and source grass specks; the separate stem samples original opaque pixels.
Fir1 keeps original alpha. These masks are explicit modifications, not exact
alpha preservation. Textures are still4x interpolation, not restored detail.
No low variants or rejected fr1 are added. Recipe metadata records the mode.
