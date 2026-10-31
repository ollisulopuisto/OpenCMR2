#ifndef OPENCMR2_FOG_UNIFORMS_H
#define OPENCMR2_FOG_UNIFORMS_H
// The ONE packer for the FogUniforms std140 block (shaders/fog.frag). Both
// executors (SDL_GPU in gfx_gpu.cpp, GL in gl_effects_executor.h) call this:
// the 23-vs-27-vec4 stack overflow came from two hand-copied fill loops whose
// size constant drifted from the shader when beams[8] was added. Layout:
// fog, sun, extra, lights[8], lightInfo[8], beams[8] = 27 vec4 = 108 floats
// (kFogUniformFloats). Every float is written explicitly - no zero-init
// reliance - so the pushed block never carries stale or unwritten lanes.
#include "port/gfx.h"
#include "render/fog_math.h"

struct FogUniformParams {
    float density;        // Fog_Density(amount)
    float glow;           // fog_glow / 100
    float nearZ, farZ;    // recovered projection, 0 = unknown (GL path)
    float sunX, sunY;     // projected sun uv (v downwards)
    float sunVisible;     // 0/1
    float aspect;         // backWidth / backHeight
    float clearDistance;  // world units, 0 = off
    float lightStrength;  // video.fog_lights 0..2
};

// withViewZ: the SDL_GPU path passes each light's viewZ for the depth gate;
// the GL path has no near/far recovery and writes 0 (shader skips the gate).
inline void FogUniforms_Pack(float *out, const FogUniformParams &p,
                             const GfxFogLight *lights, int lightCount, bool withViewZ) {
    out[0] = p.density;        out[1] = p.glow;        out[2] = p.nearZ;      out[3] = p.farZ;
    out[4] = p.sunX;           out[5] = p.sunY;        out[6] = p.sunVisible; out[7] = p.aspect;
    out[8] = p.clearDistance;  out[9] = p.lightStrength; out[10] = 0.0f;      out[11] = 0.0f;
    for (int i = 0; i < 8; i++) {
        const bool on = i < lightCount && lights != nullptr;
        const GfxFogLight *L = on ? &lights[i] : nullptr;
        out[12 + i*4 + 0] = on ? L->x : 0.0f;
        out[12 + i*4 + 1] = on ? L->y : 0.0f;
        out[12 + i*4 + 2] = on ? L->intensity : 0.0f;
        out[12 + i*4 + 3] = 0.0f;
        out[44 + i*4 + 0] = (on && withViewZ) ? L->viewZ : 0.0f;
        out[44 + i*4 + 1] = on ? L->r : 0.0f;
        out[44 + i*4 + 2] = on ? L->g : 0.0f;
        out[44 + i*4 + 3] = on ? L->b : 0.0f;
        out[76 + i*4 + 0] = on ? L->beamX : 0.0f;
        out[76 + i*4 + 1] = on ? L->beamY : 0.0f;
        out[76 + i*4 + 2] = on ? L->beamLen : 0.0f;
        out[76 + i*4 + 3] = 0.0f;
    }
}
#endif
