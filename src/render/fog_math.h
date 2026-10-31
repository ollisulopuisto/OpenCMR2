#ifndef OPENCMR2_FOG_MATH_H
#define OPENCMR2_FOG_MATH_H
#include <cmath>
// Optional atmosphere fog (video.fog, default off): a post-process distance fog with a
// ground-mist gradient and a sun-ward glow. Visual only: it reads the rendered scene, its
// depth and the projected sun position; it never touches game state. These pure functions
// are the reference for shaders/fog.frag and are covered by tests/fog_math.cpp.

// Density from video.fog_amount (0..200 accepted; the menu offers 0..100).
// 100 is the deliberately heavy "overdo first, then bring it down" starting look.
inline float Fog_Density(int amountPercent) {
    int a = amountPercent < 0 ? 0 : amountPercent > 200 ? 200 : amountPercent;
    return a / 100.f * 0.03f;   // at 100: 23 world units -> 50% fog, 60 -> 84%, 150 -> 99%
}
// View distance from hardware depth (same formula as dof.frag). 0 = projection unknown.
inline float Fog_ViewZ(float depth, float nearZ, float farZ) {
    if (!(nearZ > 0.f) || !(farZ > nearZ)) return 0.f;
    float denom = farZ - depth * (farZ - nearZ);
    if (denom < 1e-6f) denom = 1e-6f;
    return nearZ * farZ / denom;
}
// Exponential distance fog on a real view distance.
inline float Fog_Factor(float viewZ, float density) {
    if (!(viewZ > 0.f) || !(density > 0.f)) return 0.f;
    return 1.f - std::exp(-density * viewZ);
}
// Fallback when the projection is unknown: raw hardware depth is hyperbolic, so with a
// near 0.1 / far 150 projection depth 0.990 is about 9 units, 0.9993 about 60, 1.0 the far
// plane. Ramp over that top slice; the curve is a stand-in, not a measured distance.
inline float Fog_FactorRawDepth(float depth, float density) {
    if (!(density > 0.f)) return 0.f;
    float t = (depth - 0.990f) / 0.010f;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    return 1.f - std::exp(-density * 150.f * t * t);
}
// Ground-mist gradient on the screen v coordinate (0 top, 1 bottom): 0.55x at the top of
// the frame, 1.45x at the bottom, smoothstepped. Mirrors the shader's smoothstep exactly.
inline float Fog_HeightScale(float uvY) {
    float t = (uvY - 0.15f) / 0.70f;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    t = t * t * (3.f - 2.f * t);
    return 0.55f + 0.9f * t;
}
// The sky (far plane) hazes over but never becomes a flat wall.
inline float Fog_SkyCap(float factor, float depth) {
    if (depth < 0.9999f) return factor;
    return factor < 0.85f ? factor : 0.85f;
}
// Sun-ward glow of the fog ("light cutting through"): cubic falloff from the sun's
// projected position. dx/dy are aspect-corrected uv offsets; strength is 0..1
// (video.fog_glow / 100). At full strength the glow reaches about 0.6 screen heights.
inline float Fog_SunGlow(float dx, float dy, float strength) {
    if (!(strength > 0.f)) return 0.f;
    float d = std::sqrt(dx * dx + dy * dy);
    float g = 1.f - d * 1.6f;
    if (g < 0.f) return 0.f;
    return strength * g * g * g;
}
// Fog colour: cool grey-blue away from the sun, warm inside the glow. glow is Fog_SunGlow
// clamped to 0..1 for the tint; the shader additionally brightens by 1 + glow * 0.6.
inline void Fog_Color(float glow, float *r, float *g, float *b) {
    float t = glow < 0.f ? 0.f : glow > 1.f ? 1.f : glow;
    *r = 0.62f + (0.98f - 0.62f) * t;
    *g = 0.66f + (0.87f - 0.66f) * t;
    *b = 0.72f + (0.70f - 0.72f) * t;
}
#endif
