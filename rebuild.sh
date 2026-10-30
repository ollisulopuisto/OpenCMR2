#!/usr/bin/env bash
#
# rebuild.sh - clean rebuild of OpenCMR2 for macOS (Apple silicon), with optional private asset preparation.
#
# No options: wipes build/macos/, reconfigures with the "macos" CMake preset, builds everything (game + tests). Same as before.
#
# Asset options (all opt-in; none is on by default). They run the game binary's own helper with --prepare-only after the build, so the
# private pack is made the same way as at launch: originals are verified by hash and never written, the cache lives under enhancements/ in your
# profile, an old cache is never overwritten, nothing is downloaded (unless you add --install-enhancement-deps), no selection is saved to the INI.
#   --data DIR                  Your owned game data folder (or set CMR2_DATA). Required with any asset option.
#   --trees crossed|shell       3D tree replacements (experimental, three verified tree cards)
#   --spectators animated       Articulated spectator poses
#   --textures france1          France1 road 2x filtered textures (no restored detail)
#   --roads curved              Experimental Ballon curved road
#   --driver-names real         Real driver names (display only)
#   --install-enhancement-deps  Allow uv to set up an isolated Python with Pillow/numpy (network); default is no downloads
#   --assets-only               Skip the rebuild and prepare with the existing binary
#   --dry-run                   Check the inputs and print the plan; build nothing, write nothing
#   -h, --help                  This text
# Missing or unsupported inputs stop the script with a message; nothing is skipped silently. Generated packs are derived game art:
# they stay on your Mac and are never committed. Native appearance and speed of every option remain unverified.
set -euo pipefail

cd "$(dirname "$0")"

usage() { sed -n '2,/^set -euo/p' "$0" | sed '$d' | sed 's/^# \{0,1\}//'; }
die() { echo "rebuild.sh: $*" >&2; exit 2; }

DATA="${CMR2_DATA:-}"
DRY=0
ASSETS_ONLY=0
FLAGS=()
need_value() { [ "$#" -ge 2 ] && [ -n "$2" ] || die "$1 needs a value (see --help)"; }
while [ "$#" -gt 0 ]; do
    case "$1" in
        -h|--help) usage; exit 0 ;;
        --data) need_value "$@"; DATA="$2"; shift 2 ;;
        --trees) need_value "$@"; case "$2" in crossed|shell) ;; *) die "--trees takes crossed or shell, got '$2'" ;; esac; FLAGS+=(--trees "$2"); shift 2 ;;
        --spectators) need_value "$@"; [ "$2" = animated ] || die "--spectators takes animated, got '$2'"; FLAGS+=(--spectators "$2"); shift 2 ;;
        --textures) need_value "$@"; [ "$2" = france1 ] || die "--textures takes france1, got '$2'"; FLAGS+=(--textures "$2"); shift 2 ;;
        --roads) need_value "$@"; [ "$2" = curved ] || die "--roads takes curved, got '$2'"; FLAGS+=(--roads "$2"); shift 2 ;;
        --driver-names) need_value "$@"; [ "$2" = real ] || die "--driver-names takes real, got '$2'"; FLAGS+=(--driver-names "$2"); shift 2 ;;
        --install-enhancement-deps) FLAGS+=(--install-enhancement-deps); shift ;;
        --assets-only) ASSETS_ONLY=1; shift ;;
        --dry-run) DRY=1; shift ;;
        *) die "unknown option '$1' (see --help)" ;;
    esac
done

PREPARE=0
for f in "${FLAGS[@]+"${FLAGS[@]}"}"; do case "$f" in --trees|--spectators|--textures|--roads|--driver-names) PREPARE=1 ;; esac; done
[ "$PREPARE" = 1 ] || { [ "${#FLAGS[@]}" -eq 0 ] || die "--install-enhancement-deps needs an asset option"; [ "$ASSETS_ONLY" = 0 ] || die "--assets-only needs an asset option"; }

BIN=build/macos/src/opencmr2
if [ "$PREPARE" = 1 ]; then
    [ -n "$DATA" ] || die "asset options need --data DIR (or CMR2_DATA)"
    [ -d "$DATA" ] || die "game data folder not found: $DATA"
    case " ${FLAGS[*]} " in
        *" --textures "*) n=$(find "$DATA" -iname frahit.bfl 2>/dev/null | wc -l | tr -d ' ')
            [ "$n" = 1 ] || die "--textures france1 needs exactly one frahit.bfl under $DATA, found $n" ;;
    esac
    case " ${FLAGS[*]} " in
        *" --roads "*) n=$(find "$DATA" -iname fra01hi.bfl 2>/dev/null | wc -l | tr -d ' ')
            [ "$n" = 1 ] || die "--roads curved needs exactly one fra01hi.bfl (Ballon) under $DATA, found $n" ;;
    esac
    case " ${FLAGS[*]} " in
        *" --trees "*|*" --spectators "*)
            for b in frahit.bfl fralot.bfl finhit.bfl finlot.bfl; do
                n=$(find "$DATA" -iname "$b" 2>/dev/null | wc -l | tr -d ' ')
                [ "$n" = 1 ] || die "need exactly one $b under $DATA, found $n (trees and spectators are fitted to the original release)"
            done ;;
    esac
fi

if [ "$DRY" = 1 ]; then
    echo "==> Dry run: nothing is built or written"
    if [ "$ASSETS_ONLY" = 1 ]; then echo "    skip rebuild (--assets-only), use $BIN"; else echo "    rebuild: rm -rf build/macos; cmake --preset macos; cmake --build --preset macos"; fi
    if [ "$PREPARE" = 1 ]; then echo "    prepare: $BIN --data \"$DATA\" ${FLAGS[*]} --prepare-only"; echo "    inputs checked: data folder present, original archives found where needed"; else echo "    no asset options: no preparation"; fi
    exit 0
fi

if [ "$ASSETS_ONLY" = 0 ]; then
    echo "==> Removing build/macos/"
    rm -rf build/macos

    echo "==> Configuring (preset: macos)"
    cmake --preset macos

    echo "==> Building"
    cmake --build --preset macos -j"$(sysctl -n hw.ncpu)"
    echo "==> Built: build/macos/src/OpenCMR2"
fi

if [ "$PREPARE" = 1 ]; then
    [ -x "$BIN" ] || die "$BIN not found; run without --assets-only first"
    echo "==> Preparing private asset pack (originals are not modified)"
    "$BIN" --data "$DATA" "${FLAGS[@]}" --prepare-only
    echo "==> Pack prepared. Launch with the same --data and flags to use it."
fi
echo "==> Done"
