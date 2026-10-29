# Development workflow

Build requirements and platform presets are in the root [README](../README.md).
CI builds each supported platform using the same presets.

## Build and test

On Linux, run the native build and 32-bit reference build:

```sh
cmake --preset linux
cmake --build --preset linux
ctest --preset linux

cmake --preset linux-x86
cmake --build --preset linux-x86
ctest --preset linux-x86
```

The 32-bit build needs i386 libraries. On Ubuntu 24.04 (the CI environment),
SDL3 is built from source:

```sh
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install clang g++-multilib ninja-build glslang-tools pkg-config \
    libx11-dev:i386 libxext-dev:i386 libxrandr-dev:i386 libxcursor-dev:i386 \
    libxi-dev:i386 libxss-dev:i386 libxtst-dev:i386 libxkbcommon-dev:i386 \
    libwayland-dev:i386 wayland-protocols libegl1-mesa-dev:i386 \
    libasound2-dev:i386 libudev-dev:i386 libdbus-1-dev:i386
export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig:/usr/share/pkgconfig
cmake --preset linux-x86 -DOPENCMR2_VENDORED_SDL=ON
```

On Fedora (`SDL3-devel.i686` is used when it is 3.4 or later):

```sh
sudo dnf install clang gcc-c++ glibc-devel.i686 libstdc++-devel.x86_64 \
    libstdc++-devel.i686 SDL3-devel.i686 ninja-build glslang
```

To include real-stage physics, runtime, lighting and movie regressions,
configure with your own original game data, then rebuild and test:

```sh
cmake --preset linux -DOPENCMR2_BINK_TEST_DATA_DIR="$HOME/cmr2game"
cmake --build --preset linux
ctest --preset linux
```

Use the same option for `linux-x86`. LAN game tests are registered for native
builds when game data is supplied; network tests require loopback UDP access.
See [PHYSICS_PARITY.md](PHYSICS_PARITY.md) and [BASELINE.md](BASELINE.md) for
the fixture scope and recording commands.

The `game_data_setup` test uses synthetic archive placeholders and simulated
desktop dialogs with SDL's dummy video driver. It covers first-run selection,
missing files, reading archives and writing saves under UTF-8 paths, remembered
settings, cancellation and a `Data` folder beside the executable. It also
checks selecting subfolders or parent folders, ambiguous installations and
symlink cycles on every CI platform, without original assets or a display
server.

## Sanitizers

The `linux-debug` preset builds with AddressSanitizer and UBSan. With the
vendored SDL, the shared ASan runtime must be linked into both SDL and the
game, and found at run time:

```sh
cmake --preset linux-debug -DOPENCMR2_VENDORED_SDL=ON \
    -DOPENCMR2_BINK_TEST_DATA_DIR="$HOME/cmr2game" \
    "-DCMAKE_SHARED_LINKER_FLAGS=-shared-libasan -fsanitize=address,undefined" \
    "-DCMAKE_EXE_LINKER_FLAGS=-shared-libasan" \
    "-DCMAKE_CXX_FLAGS=-fsanitize-recover=address" "-DCMAKE_C_FLAGS=-fsanitize-recover=address"
cmake --build --preset linux-debug --target stage_probe
LD_LIBRARY_PATH="$(dirname "$(clang -print-file-name=libclang_rt.asan.so)")" \
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 \
    python3 tests/runtime_scenarios.py --probe build/linux-debug/tests/stage_probe --data "$HOME/cmr2game"
```

Each scenario's `run.log` holds the reports. Some of the original's code
reads a dword and keeps its low byte, running past its variable without
changing the value, or ends a loop at `&a[n]` and indexes the members after
an array; `tools/sanitizer-ignorelist.txt` lists those functions with the
access, and UBSan's function type check is off because the game calls its
callback tables through generic types. The scenarios run without reports, so
any new one is worth a look. The 60-second scenario timeout may
need raising under the sanitizers.

## Source sync and generated files

`game/` follows CMR2Decomp; `UPSTREAM` records the imported commit.
Use `tools/sync_upstream.py`, inspect its changes and resolve any conflicts.
Port adaptations and tests must remain valid after a sync. Port code keeps the
original's floating-point conventions: after a sync that touches it, run
`tools/ftoi_rewrite.py --port` (float-to-integer conversions) and
`tools/x87s_rewrite.py --port` (operations the original ran at x87 single
precision); both need `clang-devel`, and a second run finds nothing.

When compatibility-sensitive structures change, check the disk-layout header:

```sh
python3 tools/gen_disk_layouts.py --check
```

Regenerate it with `python3 tools/gen_disk_layouts.py` when needed and review
the diff. The raw-access audit requires `clang-devel` and writes its detailed
JSON and plugin caches under `build/`:

```sh
python3 tests/raw_access_audit.py --plugin
python3 tools/audit_raw_access.py --json build/raw-access-current.json \
  --markdown docs/64bit-raw-access.md --jobs 4
```

The Markdown audit is generated; update it with the tool. Individual reviews
in `tools/raw_access/reviewed.json` retain source guards and automatic evidence.

## Files to keep

- Keep guides in `docs/` and link them from its index. Results of a past
  investigation belong in the commit message, not in a new document.
- Keep frozen regression inputs, accepted comparisons and provenance in
  `tests/reference/`. Its recordings and audit logs support reproducible tests.
- Put temporary plans, logs, captures, build products and downloaded
  toolchains under `build/`. It is ignored by Git, as are `out/`, `dist/`,
  Python caches and local `CMakeUserPresets.json`.
- Never commit original game files. Release packages contain only the
  executable, its libraries and the licences.

Before committing, review `git status --short`, `git diff --check` and the staged
diff.

## Actions execution policy

The build workflow runs the complete existing native platform set on PRs,
main pushes and version tags. Feature-branch pushes do not run a second copy
beside their PR. A branch without a PR uses the explicit manual full-check path.
Only documentation/Markdown/licence-only automatic changes are excluded; code,
CMake, tests, scripts, assets and unknown paths still run the full matrix.
Tag pushes are not path-filtered by GitHub and retain release dependencies.

Obsolete PR runs cancel when a newer commit arrives on that same PR. Main,
version-tag and manually started checks are not cancelled by this policy. Jobs
have finite limits(45minutes Linux,60Mac/Windows,90Android,10release), and each
CTest has a120second ceiling to prevent a stuck test consuming the entire job.
Timeout failures remain failures; no test or platform is hidden to make CI green.
Revisit limits using actual failed logs if legitimate work exceeds them.

For urgent/full validation, open Actions > build > Run workflow and select the
branch. This runs all checks without creating a version tag or publishing a
release. It is not a bypass; a version tag still needs every platform job before
release publication. Branches that require named checks may need this manual
run for docs-only PRs, because path-filtered required checks can remain pending.
This change does not alter repository branch protection or permissions.

No new cache, artifact retention, upload or billing setting is introduced.
Artifacts and tag releases keep their existing policy. This removes duplicate
execution and queue pressure, not a claim of measured billed savings.
Run `python3 -m unittest discover -s tools/ci -v` with PyYAML installed and
`actionlint` when editing this policy.


### Link-interposition isolation tests

Some Linux-only tests/probes intercept original game functions with GNU ELF
`--wrap` and Itanium C++ mangled symbol names. They are enabled only on Linux
with GNU/Clang. Windows Clang uses COFF/lld-link, so `NOT MSVC` is not a valid
capability test: that linker ignores `--wrap`, breaking the harness or letting
it run without its intended isolation. Apple uses another link format too.
Portable tests and the actual game still build on Windows/Mac. The affected
advanced-menu, startup-failure, names/startup options, dust routing, scenery
capture, car-name and road probes are not native Windows/Mac coverage. A real
portable interception harness would be needed before claiming that coverage.
