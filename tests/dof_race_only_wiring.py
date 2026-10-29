#!/usr/bin/env python3
"""The depth blur / tunnel pass is a race effect: the SDL_GPU gate and the GL executor must both receive the race-world flag."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need("sampledDepth && backDepth, raceWorld)" in rd("src/render/gfx_gpu.cpp"), "SDL_GPU Dof_Gate must get raceWorld")
need("effectsRaceFrame=c.raceWorld;" in rd("src/render/gfx_gl.cpp"), "GL must set the race flag per frame")
g = rd("src/render/gl_effects_executor.h")
need("bool depthBlurOn = effectsRaceFrame && dofOn" in g and "bool tunnelOn = effectsRaceFrame &&" in g, "GL depth blur and tunnel must both check the race flag")
need("raceFrame = true" in rd("src/render/dof_gate.h"), "Dof_Gate raceFrame parameter missing")
print("dof race-only wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
