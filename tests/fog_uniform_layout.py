#!/usr/bin/env python3
"""Drift guard: the FogUniforms block in shaders/fog.frag, the CPU constant
kFogUniformFloats in fog_math.h, and both executor call sites must agree.
A new member added to the shader without growing the CPU side fails here."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False

frag = (r / "src/render/shaders/fog.frag").read_text()
block = re.search(r"uniform FogUniforms \{(.*?)\};", frag, re.S)
need(block is not None, "FogUniforms block must exist in fog.frag")
vec4s = 0
for name, count in re.findall(r"vec4\s+(\w+)(?:\[(\d+)\])?;", block.group(1)):
    vec4s += int(count) if count else 1

mh = (r / "src/render/fog_math.h").read_text()
m = re.search(r"kFogUniformFloats = (\d+) \* 4", mh)
need(m is not None, "kFogUniformFloats must be declared as N * 4 in fog_math.h")
cpu_vec4s = int(m.group(1)) if m else -1
need(vec4s == cpu_vec4s, f"shader declares {vec4s} vec4, CPU constant says {cpu_vec4s}")

gpu = (r / "src/render/gfx_gpu.cpp").read_text()
gl = (r / "src/render/gl_effects_executor.h").read_text()
need("FogUniforms_Pack(fogUniforms, fogP, fogLights, fogLightCount)" in gpu,
     "SDL_GPU executor must pack via the shared packer")
need("FogUniforms_Pack(u, fogP, fogLights, fogLightCount)" in gl,
     "GL executor must pack via the shared packer")
need("fogUniforms[kFogUniformFloats]" in gpu and "u[kFogUniformFloats]" in gl,
     "both executors must size their arrays from kFogUniformFloats")
print("fog uniform layout ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
