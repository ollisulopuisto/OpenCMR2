# Optional MetalFX probe toolchain

Ordinary game builds do not require Objective-C++. The standalone `.mm` probe
is now explicitly opt-in with `-DOPENCMR2_METALFX_PROBE=ON`. The SDL integration
is Objective-C, not Objective-C++. Probe language setup lives at top level and
checks compiler availability before enabling the optional language. Explicit
Objective-C++ compiler/toolchain selections are preserved.

A missing MetalFX SDK disables the optional extension with a warning, retaining
ordinary rendering. A modern SDK is needed to compile MetalFX; running it needs
macOS13+. MetalFX is weak-linked and guarded with `@available`, so merely building
with an older deployment target does not require raising the game's minimum OS.

For an existing failed probe configuration, keep the generator and build tree
and reconfigure with `-DOPENCMR2_METALFX_PROBE=OFF`. If inspecting a clean configure,
use a separate build directory rather than deleting all existing build outputs.
This does not diagnose every Ninja/AppleClang failure; preserve the actual failed
compiler command and error if configuration still fails with the probe disabled.
