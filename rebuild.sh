#!/usr/bin/env bash
#
# rebuild.sh — clean rebuild of OpenCMR2 for macOS (Apple silicon)
#
# Wipes build/macos/, reconfigures with the "macos" CMake preset, and builds
# everything (game + tests). Re-run any time you want a from-scratch build.
set -euo pipefail

cd "$(dirname "$0")"

echo "==> Removing build/macos/"
rm -rf build/macos

echo "==> Configuring (preset: macos)"
cmake --preset macos

echo "==> Building"
cmake --build --preset macos -j"$(sysctl -n hw.ncpu)"

echo "==> Done: build/macos/src/OpenCMR2"
