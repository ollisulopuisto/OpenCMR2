#pragma once
// Shadow-map filter taps. src/render/shaders/fixed.frag repeats this maths in GLSL;
// keep the two in step. A golden-angle (Vogel) disk spreads taps evenly, so a
// per-pixel rotation turns shadow-map stair-steps into fine noise.
#include <cmath>
constexpr int SHADOW_FILTER_TAPS = 12;
constexpr float SHADOW_FILTER_BASE_RADIUS = 0.5f; // texels at softness 0
// Filter radius in shadow-map texels for shadowParams.z (the Shadow softness setting, 0..4;
// the 25% default is 1.0). Strictly increasing, so lowering softness always sharpens: 0 gives
// 0.5 texel (just enough to hide the stair-step), the default gives 1.5 texels.
inline float Shadow_FilterRadius(float softness) {
    return SHADOW_FILTER_BASE_RADIUS + (softness > 0.f ? softness : 0.f);
}
// Offset of tap i (unit disk), rotated by rotation radians.
inline void Shadow_FilterTap(int i, float rotation, float *x, float *y) {
    const float golden = 2.39996323f;
    float r = std::sqrt((i + 0.5f) / SHADOW_FILTER_TAPS);
    float a = i * golden + rotation;
    *x = std::cos(a) * r;
    *y = std::sin(a) * r;
}
// Fraction of the 12 taps that see light at an occluder edge. edgeX = edge position in texels along
// x (occluded where x < edgeX); the lit tap test is the shader's: a tap is shadowed when its
// shadow-map x is left of the edge. px = pixel centre in texels.
inline float Shadow_EdgeVisibility(float px, float edgeX, float softness, float rotation) {
    float r = Shadow_FilterRadius(softness), lit = 0.f;
    for (int i = 0; i < SHADOW_FILTER_TAPS; i++) {
        float tx, ty; Shadow_FilterTap(i, rotation, &tx, &ty);
        lit += (px + tx * r) >= edgeX ? 1.f : 0.f;
    }
    return lit / SHADOW_FILTER_TAPS;
}
