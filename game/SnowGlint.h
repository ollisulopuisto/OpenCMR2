#ifndef _SNOW_GLINT_H
#define _SNOW_GLINT_H

#include "FixedPoint.h"
#include <cmath>
#include <cstring>
struct Texture;
#include "Particle.h"

// Snow glints (video.snow_glint): the snow surface and airborne flakes respond
// to light. Ground glints are the physical glitter model: a ground sample
// sparkles when the half-vector between sun and view aligns with a micro-facet
// of the surface - here the true ground normal (from the collision mesh, so
// untouched originals drive it) jittered by a deterministic per-position facet
// seed, so glints stick to the ground and flicker as the view moves, exactly
// like real snow. Head and rear lights add a cone boost from the light field
// (game/LightField.h): samples and flakes inside a light's cone brighten.
// Render-only: nothing here feeds the car, the camera or physics. Pure maths,
// covered by tests/snow_glint.cpp.
constexpr int SNOW_GLINT_TYPE = 0x23;   // appended after the air-mote slot
constexpr int SNOW_GLINT_LIFE = 10;     // 25 Hz steps (0.4 s) per glint
constexpr int SNOW_GLINT_CAP = 24;      // live glints at most
constexpr int SNOW_GLINT_ALPHA_MAX = 200;

// Deterministic micro-facet seed for a quantized ground position: the glint
// belongs to the ground, not to the frame, so it must not move with the camera.
inline unsigned SnowGlint_FacetSeed(int worldX16, int worldZ16) {
    unsigned qx = (unsigned)(worldX16 >> 17);   // 2-world-unit cells
    unsigned qz = (unsigned)(worldZ16 >> 17);
    unsigned h = qx * 0x8da6b343u ^ qz * 0xd8163841u ^ 0x9e3779b9u;
    h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; h *= 0x297a2d39u; h ^= h >> 15;
    return h ? h : 1u;
}

// Specular glitter alpha 0..1: half-vector alignment of sun and view against the
// facet-jittered ground normal. Sharp lobe (pow 48) reads as sparkle, not sheen.
inline float SnowGlint_Specular(float nx, float ny, float nz,
                                float sunX, float sunY, float sunZ,
                                float viewX, float viewY, float viewZ,
                                unsigned facetSeed) {
    float jl = ((facetSeed & 0xff) - 127.5f) / 127.5f * 0.45f;
    float jh = (((facetSeed >> 8) & 0xff) - 127.5f) / 127.5f * 0.45f;
    nx += jl; nz += jh;
    float nl = std::sqrt(nx*nx + ny*ny + nz*nz);
    if (nl < 1e-5f) return 0.f;
    nx /= nl; ny /= nl; nz /= nl;
    float hx = sunX + viewX, hy = sunY + viewY, hz = sunZ + viewZ;
    float hl = std::sqrt(hx*hx + hy*hy + hz*hz);
    if (hl < 1e-5f) return 0.f;
    hx /= hl; hy /= hl; hz /= hl;
    float d = nx*hx + ny*hy + nz*hz;
    if (d <= 0.f) return 0.f;
    float s = 1.f;
    for (int i = 0; i < 5; ++i) s *= d;          // d^5 pairs with the jitter spread
    return s * s > 1.f ? 1.f : s * s;            // d^10: soft enough on real normals
}

// Cone boost 0..1 of a world point inside a car light's beam: angle inside a
// ~20-degree cone, linear reach falloff to 30 world units. Light positions and
// directions come from the glow records (LightField).
inline float SnowGlint_ConeBoost(float px, float py, float pz,
                                 float lx, float ly, float lz,
                                 float dx, float dy, float dz, float intensity) {
    if (!(intensity > 0.f)) return 0.f;
    float vx = px - lx, vy = py - ly, vz = pz - lz;
    float dist = std::sqrt(vx*vx + vy*vy + vz*vz);
    if (dist < 1e-3f || dist > 30.f) return 0.f;
    float dl = std::sqrt(dx*dx + dy*dy + dz*dz);
    if (dl < 1e-5f) return 0.f;
    float cosA = (vx*dx + vy*dy + vz*dz) / (dist * dl);
    if (cosA < 0.94f) return 0.f;                 // ~20 degrees
    float angle = (cosA - 0.94f) / 0.06f;         // 0 at the rim, 1 on the axis
    return intensity * angle * (1.f - dist / 30.f);
}

// Airborne flake twinkle: per-flake phase from its sway cycle (deterministic per
// flake), sun-gated, boosted looking toward the sun (forward scatter is where
// real snow glints). Returns a brightness multiplier, mean ~1.
inline float SnowGlint_FlakeTwinkle(unsigned flakeSeed, float phaseTurns, float forwardScatter) {
    float t = std::sin(flakeSeed * 12.9898f + phaseTurns * 6.2831853f);
    float base = 0.85f + 0.3f * t;
    float glint = forwardScatter > 0.f ? forwardScatter * (0.5f + 0.5f * t) : 0.f;
    float m = base + glint * 2.0f;
    return m < 0.3f ? 0.3f : m > 2.4f ? 2.4f : m;
}

// Glint fade envelope over SNOW_GLINT_LIFE steps: fast in, fast out.
inline int SnowGlint_FadeAlpha(int elapsedSteps, int peak) {
    if (elapsedSteps < 0 || elapsedSteps > SNOW_GLINT_LIFE) return 0;
    int fadeIn = elapsedSteps < 2 ? elapsedSteps : 2;
    int fadeOut = SNOW_GLINT_LIFE - elapsedSteps;
    if (fadeOut > 4) fadeOut = 4;
    return peak * fadeIn * fadeOut / (2 * 4);
}

// Per-particle peak alpha is stamped at spawn (pad0x5c, as 0..255); the step
// applies the envelope. Glints do not move.
inline void SnowGlint_Step(Particle &p) {
    int peak = 0;
    std::memcpy(&peak, p.pad0x5c, 4);
    if (peak < 0) peak = 0;
    if (peak > SNOW_GLINT_ALPHA_MAX) peak = SNOW_GLINT_ALPHA_MAX;
    p.vector0x28 = p.sourceVector;
    p.type0x52 = p.type0x56;
    p.type0x56 = (BYTE)SnowGlint_FadeAlpha(SNOW_GLINT_LIFE - (p.age >> 16), peak);
}

extern int g_snowGlintNextPeak;
void SnowGlint_Update(Particle *, ParticleType *, SceneNode *);
void SnowGlint_Init(Particle *, ParticleType *, void *);
void SnowGlint_Spawn(FixVector *world, int peakAlpha);

#endif
