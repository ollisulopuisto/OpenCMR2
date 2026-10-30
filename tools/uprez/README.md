# Local 2x texture baseline (reconstructed)

`uprez.py` makes a 2x alpha-aware Lanczos copy of textures you already own. `test_uprez.py` has seven unit tests.

**Provenance.** The original converter and tests of 29 October 2026 were not recovered as files. This is a reconstruction from the record of the code that was written then, not the original bytes. The original private manifests and the generated PNGs are not included and are not in this repository.

**What it does.** For each row of an inventory JSON (`archive`, `texture`, `path`, `sha256`) it checks the source hash, writes `<output>/<archive>/<texture stem>.png` at twice the size, and writes `manifest.json`. Premultiplied alpha is used so hidden colour does not bleed into edges.

**Safety.** Originals are only read and are re-hashed after processing. The output folder must not exist. Unsafe or duplicate output paths and wrong hashes are refused before anything is written.

**Limits.** Interpolation only, not detail restoration, this generic script is not runtime integration. Atlas-cell bleed, alpha-test coverage, mips, UVs, seams and native performance are unverified. Whole-atlas filtering can change alpha-test silhouettes, so this is not an installable pack. The game does not directly load these generic PNG outputs. The integrated `--textures uprez` path instead prepares and loads the exact-source Finland1/France1 opaque ground subset with full mips. Unknown/cutout/atlas sources stay original. You supply the inventory and the extracted source images yourself; the tool needs Pillow. There is no rebuild.sh option for it because the extraction step that makes the inventory is not part of this repository. Do not publish generated art.

```sh
(cd tools/uprez && python3 -m unittest test_uprez)
python3 tools/uprez/uprez.py --inventory inventory.json --output /path/to/new/output
```

For the France 1 road textures the game already has its own opt-in path: `--textures france1` (see tools/scenery-patch).
