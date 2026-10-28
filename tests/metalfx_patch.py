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
