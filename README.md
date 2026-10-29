<p align="center">
  <img src="docs/logo.jpeg" width="320" alt="OpenCMR2 logo">
</p>

# OpenCMR2

[![Build](https://github.com/pablocpas/OpenCMR2/actions/workflows/build.yml/badge.svg)](https://github.com/pablocpas/OpenCMR2/actions/workflows/build.yml)

OpenCMR2 is a work-in-progress native port of **Colin McRae Rally 2.0** (PC, 2000)
to modern platforms, built on the [CMR2Decomp](https://github.com/pablocpas/CMR2Decomp)
decompilation. It runs the original game code natively on Linux, Windows,
macOS and Android. There is no emulation and no Wine or DirectX wrapper: DirectDraw,
Direct3D 7, DirectSound, DirectInput, DirectPlay and Bink are replaced with
SDL3, the SDL GPU API (Vulkan, Direct3D 12, Metal) or OpenGL 3.3 and the
port's own code.

https://github.com/user-attachments/assets/3c7322dc-576b-4fb0-8d6a-4d17de6fe17d

*Colin McRae Rally 2.0 running on Linux with OpenCMR2.*

> [!WARNING]
> This repository contains no game assets. You need your own copy of the
> original PC version of Colin McRae Rally 2.0 to play.

## Status

Everything tested so far works: the intro videos, the menus, the stages and
network races over LAN. The project is still in development, so you may find
bugs. If you do, please [report them](https://github.com/pablocpas/OpenCMR2/issues).
The game code follows CMR2Decomp, and its progress is brought into the port
as it is made.

| Platform | Architecture | Status |
| --- | --- | --- |
| Linux | x86-64 | Plays stages and matches the original physics |
| Linux | i686 | Plays stages, reference build with the original memory layout |
| Linux | ARM64 | Builds and passes the tests, gameplay not yet tested |
| Windows | x86-64 | Plays stages and matches the original physics (tested under Wine) |
| macOS | ARM64 | Builds and passes the tests, gameplay not yet tested |
| Android | ARM64, x86-64 | Plays stages with on-screen controls (Android 10 or later, Vulkan) |

### Features

* Native SDL3 platform layer for windowing, input, audio and files, with saves
  and settings in the user directory
* SDL GPU renderer (Vulkan, Direct3D 12, Metal) reproducing the Direct3D 7
  fixed-function pipeline in shaders, and an OpenGL 3.3 renderer with the same
  shaders for GPUs without Vulkan or Direct3D 12 (Linux and Windows)
* Fullscreen on the desktop resolution or a resizable window, switched with
  Alt+Enter
* Keyboard, mouse and gamepads through SDL3, and on-screen controls on Android

### Keyboard remapping

Use the existing **Options > Controls** screen: choose a keyboard, select the
control, release the confirm key, then press its new key. Escape cancels capture
without changing the binding. Assigning a key clears its previous action in the
same configuration. Accept the controls edits through the normal menu save flow;
these remain in the existing `Configuration/Controller.rcf`, with no format or
profile migration. Menu arrows, Enter, Escape and text entry are unchanged.

For temporary bindings, the binary also accepts repeated `--bind-key` options:

```sh
./build/macos/src/opencmr2 --data "/path/to/owned/game-data" \
  --bind-key left=A --bind-key right=D --bind-key accelerate=W --bind-key brake=S
./build/macos/src/opencmr2 --help-controls
```

This is an example, not a new default. Actions are `left`, `right`, `accelerate`,
`brake`, `handbrake`, `gear-up`, `gear-down`, `camera`, and `look-back`. Keys use
SDL physical scancode names such as `W`, `Space`, `Up`, or `"Left Shift"`.
`none` unbinds an action. Prefix an action with `2:` to select the second local
keyboard configuration, for example `--bind-key "2:brake=Keypad 2"`. Escape is
reserved. Explicit duplicate keys on one keyboard are rejected; swaps work.
Unspecified actions keep saved keys, except a key reassigned to another action
is cleared there for this launch. Temporary overrides take priority over in-game
edits until restart, but are never serialized to the controls file. Omit them on
the next launch to return to your saved bindings. In-game edits still save normally.

* Bink intro and country videos with a built-in decoder
* Multiplayer over LAN and the internet using UDP instead of DirectPlay
* Folder dialog on first launch to locate the game installation
* In-race HUD scaled with the screen and kept at its edges on wide screens,
  with a configurable size
* Fixes for some original bugs, such as German in the language menu loading
  Spanish

The improvements of [SilentPatch](https://cookieplmonster.github.io/mods/colin-mcrae-rally-2/)
are built in as well:

* Widescreen resolutions up to the desktop resolution, with the correct
  perspective instead of a stretched picture
* Configurable field of view
* High and variable frame rates without changing the game speed or the
  physics, verified against the original game

## Approach

The game code in `game/` comes from CMR2Decomp and is kept as close to the
original as possible. Every change the port needs is marked with a `PORT:`
comment. The rest of the port (`src/`) implements the platform APIs the game
expects on top of SDL3.

Accuracy is tested against the original executable rather than assumed:

* The fixed-point helpers are compared with the original x87 assembly
* The startup math tables (16,896 entries) are compared with dumps from the
  original game
* 500 vehicle updates of a Finland stage, driven by recorded input, are
  compared field by field with the original game + SilentPatch at 30, 60,
  120, 144 and 240 FPS and at a variable frame rate. All of them match bit
  for bit ([details](docs/PHYSICS_PARITY.md)).

Graphical enhancements will be added one at a time and kept optional, so the
original look stays available.

## Running

Download the package for your platform from
[Releases](https://github.com/pablocpas/OpenCMR2/releases):

| Platform | Package |
| --- | --- |
| Windows x64 | `OpenCMR2-Windows-x64.zip` |
| Linux x86-64 | `OpenCMR2-Linux-x86_64.tar.gz` |
| Linux ARM64 | `OpenCMR2-Linux-arm64.tar.gz` |
| Linux 32-bit | `OpenCMR2-Linux-i686.tar.gz` |
| macOS (Apple silicon) | `OpenCMR2-macOS-arm64.zip` |
| Android | `OpenCMR2-Android.apk` |

Extract it and run `opencmr2`. On first launch, choose the folder of your PC
installation. The installation itself, a folder inside it or a parent folder
all work. The location is checked and remembered, and the game files stay
where they are.

If you have the CD or an ISO, install the game or extract it first: OpenCMR2
reads the installed folders `Game`, `FrontEnd`, `Sounds`, `CountrySpecific`
and `SetupRep`, and does not start when one of them is missing.

On Android, install the APK and copy the installation folder to the
device first. On first launch, choose it in the system's folder picker: the
game files are copied into the app once (about 700 MB), as Android gives
apps no direct access to other folders. The on-screen controls press the
default keyboard keys, so they work in the menus and on the stages, and
hide while a gamepad is used, external or built into a handheld. Android selects
an SDL gamepad automatically; Start pauses the game. The launcher has a
**Controls** screen, usable by touch before starting the game, to test a pad and
correct individual buttons, sticks or triggers. These corrections are saved by
device and preserve the rest of SDL’s automatic layout and analog input.
**Restore automatic mapping** removes the corrections. You can also keep the
touch controls visible or choose to play with touch controls only. Android Back
pauses the game; a controller’s Back/Select follows its SDL mapping. Connect or
disconnect controllers in the menus (or pause first during a stage) to rescan.
Controller firmware must expose its input to Android; buttons intercepted by
system shortcuts cannot be mapped by the app.

The Linux packages are built on Ubuntu 24.04 and need a distribution of
similar age or newer. The macOS package is not signed: after extracting it,
run `xattr -dr com.apple.quarantine OpenCMR2-macOS-arm64` once.

Command-line options:

| Option | Description |
| --- | --- |
| `--data <dir>` | Use this game installation and skip the folder dialog |
| `--user-dir <dir>` | Keep saves, settings and logs in this directory |

The installation can also be set with the `OPENCMR2_DATA` environment variable
or `[paths] data` in `opencmr2.ini`, or copied into a `Data` folder next to the
executable, which is found without asking.

### Optional real-driver names

The real-driver names are a separate local patch, not an edit to the original
game files. Generate `driver-names.tsv` in a private user directory and launch
with that `--user-dir`. Without the sidecar, the original names remain.
See [enable, mapping and undo instructions](tools/driver-name-patch/README.md).

## Configuration

Settings are stored in `opencmr2.ini` in the user directory
(`~/Library/Application Support/OpenCMR2/` on macOS,
`~/.local/share/OpenCMR2/` on Linux,
`%APPDATA%\OpenCMR2\` on Windows; files from the older
`OpenCMR2/OpenCMR2` folder are moved there).
It is created with comments on first launch. The Screen page of the
graphics options sets `hud_scale`, `menu_scale`, `vsync` and `fullscreen` in
the game.

| Setting | Default | Description |
| --- | --- | --- |
| `[video] fullscreen` | `1` | Fullscreen (`1`) or window (`0`), changed and saved by Alt+Enter |
| `[video] fov` | `70` | Field of view in degrees, 30 to 150 (70 is the original) |
| `[video] run_in_background` | `0` | Keep running when the window loses focus |
| `[video] car_detail` | `1` | Every car with the model and textures of the player's car (`1`), or the original's lower detail for the other cars (`0`); applies with Car Detail set to High |
| `[video] hud_scale` | `100` | Size of the in-race HUD in percent of the original's at the screen's height; `0` keeps the original's layout in pixels |
| `[video] hud_aspect` | `16:9` | Widest shape the HUD spreads over, centred on wider screens: an aspect ratio such as `16:9` or `21:9`, or `full` for the whole screen |
| `[video] menu_scale` | `100` | Size of the menus in percent of the original's at the screen's height; `0` keeps the original's layout in pixels |
| `[video] outline_fonts` | `1` | Draw the text from outlines, sharp at any size (`1`), or from the original's bitmaps (`0`) |
| `[video] vector_flags` | `1` | Keep the original dotted menu flags, drawing each dot's circular edge at screen resolution when enlarged (`1`), or magnify the original dot bitmap (`0`); native-size dots and small statistics flags keep their original pixels |
| `[video] vector_icons` | `1` | Trace the original co-driver arrows and menu arrows/checkmarks/crosses/boxes/circles for sharp enlarged edges (`1`), or keep the bitmaps (`0`); native-size icons keep their original pixels |
| `[video] vector_maps` | `1` | Sharp enlarged arcade minimaps, car markers and circular points in circuit-selection maps (`1`), or keep the bitmaps (`0`); original routes, point grids, colours and native-size pixels are preserved |
| `[video] vsync` | `1` | Wait for the display's refresh (`1`) or draw frames as soon as they are ready (`0`) |
| `[video] graphics_api` | `auto` | Renderer: `sdl_gpu` (Vulkan, Direct3D 12, Metal), `opengl` (OpenGL 3.3, Linux and Windows), or `auto` for SDL GPU when the GPU has a Vulkan, Direct3D 12 or Metal driver (not a software one) and OpenGL otherwise |
| `[video] window_scale` | `1` | Size of the window in multiples of the game's resolution, applied when leaving fullscreen |
| `[video] smooth_scaling` | `1` | Smooth the picture when it is scaled to the window or screen (`1`), or keep sharp pixels (`0`) |
| `[video] anisotropy` | `1` | Anisotropic texture filtering, `2` to `16`; `1` is off, as in the original |
| `[video] motion_blur` | `1` | Blend each frame into the previous one for motion ghosting: `0` off, `1` on (SDL_GPU renderer only) |
| `[video] bloom` | `1` | Make bright pixels bleed into their neighbours with a soft glow: `0` off, `1` on (SDL_GPU renderer only) |
| `[video] particles` | `1` | Rain or snow falling through the scene: `0` off, `1` rain, `2` snow |
| `[video] shadows` | `1` | Directional light shadow map darkening occluded surfaces: `0` off, `1` on (SDL_GPU renderer only) |
| `[video] stretch_movies` | `0` | Stretch the movies over the whole window (`1`) instead of keeping their 4:3 proportions |
| `[input] key_repeat_delay` | `750` | Milliseconds before a key held in the menus repeats |
| `[input] key_repeat_interval` | `100` | Milliseconds between the repeats of a held key |
| `[install] language` | `English` | English, American, French, German, Italian, Spanish or Polish |
| `[install] sku` | `EUROPE` | Game edition: `EUROPE`, `AMERICA`, `POLAND` or `JAPAN` |
| `[install] menu_shortcut` | asked | Entry in the applications menu (Linux) or Start menu (Windows): `1` keeps it pointing at this copy, `0` removes it |
| `[net] ...` | | Network play, see [NETWORK.md](docs/NETWORK.md) |

### Startup diagnostics

`opencmr2.log` in the selected user directory prints a one-time options summary
at startup. `requested` means configuration, not proof of an effect running.
Follow-up lines report the initialized renderer, chosen shader format and
MetalFX extension availability. MetalFX's existing first-presentation line
reports `encoded` or `fallback`; availability alone is not an encoded frame.

Driver names report custom validated names or an original-name fallback reason
(missing/invalid/disabled sidecar). Scenery reports a manifest's parsed mapping
count, not loaded textures. Trees and spectators each log their first successful
stage override; failed asset preparation logs a fallback once per stage load.
No per-frame option dump, generated artwork or message contents are logged.
No performance or native visual-correctness claim follows from these messages.

## Building

OpenCMR2 builds with Clang, CMake 3.24+, Ninja and glslang, using the presets
in `CMakePresets.json`:

| Preset | Target |
| --- | --- |
| `linux` | Linux, 64-bit (x86-64 or ARM64) |
| `windows-x64` | Windows x64 (`clang++` with the MSVC runtime) |
| `macos` | macOS, Apple silicon |
| `linux-x86` | Linux, 32-bit x86 (reference build for the tests) |

OpenCMR2 needs SDL 3.4 or later. An installed SDL3 of that version is used;
otherwise CMake downloads and builds it and places it next to the executable
(to force this, pass `-DOPENCMR2_VENDORED_SDL=ON`). Start by cloning the repository:

```sh
git clone https://github.com/ollisulopuisto/OpenCMR2.git
cd OpenCMR2
```

### Linux

Install the dependencies.

Fedora (SDL3-devel is used when it is 3.4 or later):

```sh
sudo dnf install clang cmake ninja-build glslang SDL3-devel
```

Ubuntu 24.04 or Debian, where SDL3 is built from source with these
development libraries:

```sh
sudo apt install clang cmake ninja-build glslang-tools pkg-config \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxtst-dev \
    libxkbcommon-dev libwayland-dev wayland-protocols libegl1-mesa-dev \
    libasound2-dev libpulse-dev libudev-dev libdbus-1-dev
```

Then build:

```sh
cmake --preset linux
cmake --build --preset linux
```

The executable is `build/linux/src/opencmr2`.

### macOS (Apple silicon, including M1/M2)

Run these commands in Terminal. Install Apple's Command Line Tools first if
`clang` is not available (`xcode-select --install`), and install
[Homebrew](https://brew.sh/) if `brew` is not available. The tools and source
build do not include the original game data.

From a fresh clone, enter the `OpenCMR2` directory as shown above. For an
existing checkout, enter that checkout instead; its folder name may differ.
The commands below must run from the directory containing `CMakePresets.json`.

```sh
brew install cmake ninja glslang spirv-cross python
cmake --preset macos \
  -DOPENCMR2_VENDORED_SDL=ON \
  -DOPENCMR2_SHADERS_MSL=ON \
  -DOPENCMR2_RENDERER_GL=OFF \
  -DOPENCMR2_METALFX=ON
cmake --build --preset macos
ctest --preset macos
```

This builds `build/macos/src/opencmr2` for ARM64 with Clang. It uses the pinned
vendored SDL, including this fork's optional MetalFX spatial extension. CMake
needs network access to fetch SDL if its sources are not cached. SPIRV-Cross
from Homebrew translates the shaders locally instead of fetching that tool.
MetalFX SDK absence produces a warning and falls back to ordinary Metal;
compiling MetalFX support does not force it on at runtime. Passing unit tests
is not proof that native graphics or gameplay work on your Mac.

#### Launch with your original game installation

Replace `/path/to/owned/CMR2` with your installed/extracted PC game folder,
containing `Game`, `FrontEnd`, `Sounds`, `CountrySpecific` and `SetupRep`.
Keep the quotes when the path contains spaces. The original files are not
needed to compile; they are needed to play.

```sh
./build/macos/src/opencmr2 --data "/path/to/owned/CMR2"
```

Without `--data`, the first-launch folder picker asks for that installation.
Normal Mac settings, saves and logs live in
`~/Library/Application Support/OpenCMR2/`. You can use a separate profile
without changing the existing one:

```sh
./build/macos/src/opencmr2 --data "/path/to/owned/CMR2" --user-dir "$HOME/OpenCMR2-private"
```

This creates separate settings/saves/logs; it does not copy your existing
profile. The chosen user directory must be outside the original game folder.

#### Recover an earlier build with MSL disabled

If startup prints `SDL_CreateGPUDevice: No supported SDL_GPU backend found!`,
first check how the executable was built. SDL_GPU Metal requires MSL shaders;
a build with `OPENCMR2_SHADERS_MSL=OFF` only advertises SPIR-V and is not a
usable default Mac renderer. This error alone does not prove the GPU lacks
Metal. Changing the resolution cannot supply the missing shader format.

Install `spirv-cross` and repeat the configure/build commands above in the same
checkout. The explicit `OPENCMR2_SHADERS_MSL=ON` overrides an earlier cached
OFF value; running the bare preset does not reset cached options. There is no
need to delete the build directory, original data or player profile. OpenGL
OFF is the normal Mac setting. If fetching SDL fails, fix that network error
rather than disabling MSL to bypass it.

For ordinary Metal without this fork's MetalFX extension, an installed
SDL 3.4+ can be used with `OPENCMR2_METALFX=OFF` and
`OPENCMR2_VENDORED_SDL=OFF`; MSL must stay ON. This is a separate fallback,
not the MetalFX build above.

#### Optional enhancements from the command line

```sh
./build/macos/src/opencmr2 --help-enhancements
./build/macos/src/opencmr2 --data "/path/to/owned/game-data" --trees crossed --spectators animated --driver-names real
```

The binary verifies originals, prepares/reuses private versioned assets, and
starts the game. `--trees shell` selects the experimental tiered shell instead
of fixed crossed cutouts. Only3high-source tree mappings are generated; people
have four forearm poses on one verified atlas and limited sway on the other47,
not3Dmodels. Neither tree mode has native
visual acceptance. `original` disables that kind for this launch, and selections
are not written to settings. No `--user-dir` switch is needed: driver progress
and saves stay in the current/default profile.

Scenery requires supported owned archives and Python3.10+ with Pillow/numpy.
An existing converter `.venv` is reused. If dependencies are missing, opt in to
isolated uv setup with `--install-enhancement-deps` on the same command. No
Python/packages are downloaded without that explicit flag; uv itself must be
installed already. No proprietary asset downloads or public artwork are added.
`--prepare-only` validates/prepares and exits without launching.

`--cars LIST --mods-dir PATH` selects hash-verified local owned ZIP car
alternatives; help lists names and conflict groups. Slots are replacements,
not an expanded roster. Selected skins also supply car display labels from
verified archive metadata, for example Skoda Fabia WRC Red Bull in the Focus
Alternate 1 slot. Exact variants stay independent. `--car-names original`
restores the original labels while keeping the selected assets; `skin` is the
default with a selected overlay. Labels do not change slot IDs, unlocks,
physics or saves. Native compatibility, label fit and physics are unverified.
`--effect dust_volume=off` (or another listed effect) overrides a GPU option
for this launch; runtime logs still determine actual backend support.
`--driver-names real` selects the extracted15display names, not saved player
progress. Omit flags to preserve existing settings/mods.

See the [scenery and car preparation guide](tools/scenery-patch/README.md) for
requirements, cache/failure behavior, car caveats and the old manual converter
route. Existing `--scenery-dir PATH` remains a read-only pack selection; don't
combine it with automatic preparation flags. No original data or nonempty pack
is overwritten. Generated files stay in a separate private versioned cache.

Complete generated assets stay private and out of Git/public releases.
Generation success does not verify native placement, appearance or M2
performance. To undo automatic selections, omit those flags on the next launch. Existing
manual sidecars/settings remain as they were. See the
[pack format and limits](docs/SCENERY-PACK.md).

### Windows

Install Visual Studio 2022 with its "C++ Clang tools" component (or LLVM),
Ninja and [glslang](https://github.com/KhronosGroup/glslang/releases). From an
"x64 Native Tools" command prompt:

```bat
cmake --preset windows-x64
cmake --build --preset windows-x64
```

`clang-cl` is not supported, as the build uses GNU-style options. To
cross-build from Linux with LLVM-MinGW, see [WINDOWS.md](docs/WINDOWS.md).

### Android

The Gradle project in `android/` builds the same CMake project with the
Android NDK. It needs JDK 17 or later, the Android SDK (with `ANDROID_HOME`
set or `sdk.dir` in `android/local.properties`) and glslang on the `PATH`.
Gradle downloads SDL's source and fetches the NDK, CMake and SDK platform
when the SDK licences are accepted:

```sh
cd android
./gradlew assembleRelease
```

The APK is `android/app/build/outputs/apk/release/app-release.apk`. It is
signed with the release key given by `OPENCMR2_KEYSTORE`,
`OPENCMR2_KEYSTORE_PASSWORD`, `OPENCMR2_KEY_ALIAS` and
`OPENCMR2_KEY_PASSWORD` (or the `opencmr2.keystore` Gradle properties), and
with the debug key otherwise.

### Tests

```sh
ctest --preset linux
```

The 32-bit `linux-x86` build keeps the original data layout and is the
reference for the tests against the original game. Its dependencies and
the rest of the development workflow are described in
[DEVELOPMENT.md](docs/DEVELOPMENT.md).

## Documentation

| Document | Contents |
| --- | --- |
| [ARCHITECTURE.md](docs/ARCHITECTURE.md) | Source layout, platform APIs and renderer design |
| [DEVELOPMENT.md](docs/DEVELOPMENT.md) | Tests, syncing with CMR2Decomp, repository conventions |
| [PHYSICS_PARITY.md](docs/PHYSICS_PARITY.md) | Physics and timing comparison with the original game |
| [ORIGINAL_REFERENCE.md](docs/ORIGINAL_REFERENCE.md) | Recording the original game for the comparisons |
| [BASELINE.md](docs/BASELINE.md) | Stage capture, replay and CPU profiling |
| [NETWORK.md](docs/NETWORK.md) | Network play setup and protocol |
| [VIDEO.md](docs/VIDEO.md) | Bink decoder |
| [WINDOWS.md](docs/WINDOWS.md) | Release packages and cross-building for Windows |
| [64bit-raw-access.md](docs/64bit-raw-access.md) | 64-bit memory access audit |

The full index is in [docs/README.md](docs/README.md).

## Credits

* [CMR2Decomp](https://github.com/pablocpas/CMR2Decomp), the decompilation
  this port is built on
* Silent, for [SilentPatch](https://cookieplmonster.github.io/mods/colin-mcrae-rally-2/),
  whose improvements are built in and which is the reference for the
  comparisons with the original game
* The [SDL](https://libsdl.org) and [ENet](http://enet.bespin.org) projects
* Filippo Scognamiglio for [RadialGamePad](https://github.com/Swordfish90/RadialGamePad),
  the on-screen controls on Android
* Konstantin Shishkov, Peter Ross and Daniel Verkamp for FFmpeg's Bink
  decoder, which `src/video/` is adapted from

## Licence

OpenCMR2 is licensed under the [GPL-3.0](LICENSE). Bundled third-party code
keeps its own licence: SDL3 (zlib), ENet (MIT), Potrace (GPL-2.0-or-later),
the Bink decoder (LGPL-2.1) and RadialGamePad (GPL-3.0).

OpenCMR2 is an unofficial project, not affiliated with or endorsed by
Codemasters or Electronic Arts. Colin McRae Rally 2.0 is a trademark of its
respective owners.


The spectator option currently articulates the raised forearm on one verified
France atlas using the original demo recipe, not all crowd limbs. Other atlases
retain subtle cutout sway. `--tree-wind off` disables the new base-fixed2Dwind
(default on with a selected tree mode). Wind does not turn cards into3Dtrees.
See the preparation guide for exact coverage and the runtime draw counters.

### Experimental curved Ballon roads

Pass `--roads curved` to the OpenCMR2 binary to prepare and select matching
curved render/contact geometry. `--roads original` or no road flag keeps the
original geometry. `--help-enhancements` lists the option; add `--prepare-only`
to prepare/verify without starting the game. Keep your usual data/profile flags;
no separate save profile is needed, and original game files are never modified.

This requires an existing Python 3.10+ environment with numpy. No dependency or
asset downloads are performed. Only the exact supported original France Ballon
high-detail archive is accepted. Other stages remain original. Missing or changed
source/preparation errors stop that launch with a diagnostic; runtime pair/source
mismatch falls back to both original render and contact geometry together.

This is experimental, not native acceptance: FIN barriers are unchanged and may
not align with moved boundaries. Native Mac driving, culling and performance have
not been verified. The source-only converter and cache require your owned data;
no game artwork or stage bytes are included in this repository.
