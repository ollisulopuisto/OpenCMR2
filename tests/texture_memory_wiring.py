#!/usr/bin/env python3
"""Texture memory overlay wiring: the F10 panel shows live texture retention from
Gfx_GetTextureMemoryStats so the 10 GB memory report can be attributed. The panel
budget test owns row accounting; this pins the data path."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
rdc = (r / "game/RallyData.cpp").read_text()
gfx_h = (r / "src/port/gfx.h").read_text()
core = (r / "src/render/gfx_core.cpp").read_text()
need("Gfx_GetTextureMemoryStats(&texCpu,&texGpu,&texCount)" in rdc, "panel reads the texture memory stats")
need('snprintf(text,sizeof(text),"Textures %d | cpu %lluM | gpu ~%lluM"' in rdc, "panel prints the texture memory row")
need("Gfx_GetTextureMemoryStats" in gfx_h, "stats function declared in gfx.h")
need("for (GfxTexture *t : s_textures)" in core, "stats walk the live texture registry")
print("ok texture memory wiring" if ok else "FAILED")
sys.exit(0 if ok else 1)
