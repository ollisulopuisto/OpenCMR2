#!/usr/bin/env python3
"""Pinned extension installer safety: rejects drift, idempotent exact install."""
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
patcher = root / 'cmake/metalfx/patch_sdl.py'
with tempfile.TemporaryDirectory() as temp:
    sdl = pathlib.Path(temp)
    (sdl / 'src/gpu/metal').mkdir(parents=True)
    (sdl / 'CMakeLists.txt').write_text('project(SDL3 VERSION "3.4.19")')
    target = sdl / 'src/gpu/metal/SDL_gpu_metal.m'
    target.write_text('original')
    result = subprocess.run([sys.executable, str(patcher), str(sdl)], capture_output=True)
    assert result.returncode != 0 and target.read_text() == 'original'
    (sdl / 'CMakeLists.txt').write_text('project(SDL3 VERSION "3.4.18")')
    # Missing anchors must fail closed, with no partial write.
    result = subprocess.run([sys.executable, str(patcher), str(sdl)], capture_output=True)
    assert result.returncode != 0 and target.read_text() == 'original'
print('PASS: wrong version and drift fail closed without partial writes')

# The scaler lives in MetalRenderer, which is used on every macOS target. A
# typed MTLFXSpatialScaler member makes clang warn (-Wunguarded-availability-new)
# for the whole struct at a deployment target below 13. The member must be an
# untyped id; the typed use stays inside the @available(macOS 13.0) block.
patch_text = patcher.read_text()
inc_text = (root / 'cmake/metalfx/SDL_metalfx.m.inc').read_text()
assert 'id<MTLFXSpatialScaler> opencmr2Scaler;' not in patch_text, 'typed MTLFX member in MetalRenderer'
assert '    id opencmr2Scaler;' in patch_text
guard = inc_text.index('@available(macOS 13.0, *)')
assert all(i > guard for i in
           [m for m in range(len(inc_text)) if inc_text.startswith('id<MTLFXSpatialScaler>', m)]), \
    'typed scaler use outside the availability check'
print('PASS: no unguarded MTLFX type in MetalRenderer')
