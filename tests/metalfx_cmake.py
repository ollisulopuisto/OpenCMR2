#!/usr/bin/env python3
"""Source invariant test, not a substitute for native AppleClang configure."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
top=(root/'CMakeLists.txt').read_text(); tests=(root/'tests/CMakeLists.txt').read_text()
assert 'option(OPENCMR2_METALFX_PROBE "Compile the standalone Objective-C++ MetalFX probe" OFF)' in top
assert 'enable_language(OBJCXX)' not in tests
assert top.index('enable_language(OBJCXX)') < top.index('add_subdirectory(src)')
assert 'if(APPLE AND NOT ANDROID AND OPENCMR2_METALFX_PROBE)' in tests
assert 'SHELL:-weak_framework MetalFX' in top and 'SHELL:-weak_framework MetalFX' in tests
assert 'MetalFX REQUIRED' not in top
print('MetalFX optional probe/CMake invariants pass; native configure remains required')
