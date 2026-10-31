#!/usr/bin/env python3
"""Atmosphere fog is a race effect like depth blur: both render paths must gate it on the race-world flag, or menus/splash videos get a full-screen fog wash."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need("if (raceWorld && fogEnabled && fogAmount > 0 && sampledDepth" in rd("src/render/gfx_gpu.cpp"), "SDL_GPU fog pass must check raceWorld")
g = rd("src/render/gl_effects_executor.h")
need("if (effectsRaceFrame && fogOn && fogAmount > 0) {" in g, "GL executor fog pass must check the race flag")
need("effectsRaceFrame && fogOn && fogAmount > 0 ||" in g, "GL any-effects expression must also gate fog on the race flag")
print("fog race-only wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
