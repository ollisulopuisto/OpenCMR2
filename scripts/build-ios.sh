#!/usr/bin/env bash
# Build-only preparation. Never installs, signs, uploads or registers accounts.
set -euo pipefail
cd "$(dirname "$0")/.."
case "${1:-sim}" in
  sim) preset=ios-sim; sdk=iphonesimulator;;
  device) preset=ios-device; sdk=iphoneos;;
  *) printf '%s\n' 'Usage: scripts/build-ios.sh [sim|device]' >&2; exit 2;;
esac
if [[ "$(uname -s)" != Darwin ]]; then
  printf '%s\n' 'An Apple SDK and Xcode on macOS are required. No build was attempted.' >&2
  exit 1
fi
for tool in cmake ninja xcrun glslangValidator spirv-cross; do
  command -v "$tool" >/dev/null || { printf 'Missing build tool: %s\n' "$tool" >&2; exit 1; }
done
xcrun --sdk "$sdk" --show-sdk-path >/dev/null
cmake --preset "$preset"
cmake --build --preset "$preset"
app="build/$preset/src/opencmr2.app"
test -x "$app/opencmr2"
plutil -lint "$app/Info.plist"
file "$app/opencmr2"
printf 'Build only: %s\n' "$app"
printf '%s\n' 'A simulator app cannot install on an iPhone/iPad. The device app is unsigned.'
