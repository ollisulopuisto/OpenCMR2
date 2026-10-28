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

## macOS / Linux

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

The base loader from PR18 is merged into this fork; this tree recipe also
needs PR27's source-fit runtime. No manual `git apply` is needed
when building current `main`. Generate into `scenery-pack` inside a separate
private user directory, outside the original game installation. Launch with:

```sh
./build/macos/src/opencmr2 --data /path/to/owned/game-data --user-dir /path/to/private/user-dir
```

The loader's default relative `scenery-pack` path is found under that user
directory. No INI change is needed for a new profile. Do not set an absolute
`scenery_pack` path: game-file reads resolve relative to the user/data roots,
not arbitrary absolute filesystem paths. If changing the relative folder name,
use `[video] scenery_pack=relative-folder-name` in that profile's INI.

A separate user directory isolates this test's saves/settings/logs. Existing
settings and saves are left untouched, not copied into the new profile. Do not
overwrite original game data. To undo, stop using the separate profile or set
`scenery_overrides=0` in its existing `[video]` section. Keep generated artwork
out of Git and public releases.

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
