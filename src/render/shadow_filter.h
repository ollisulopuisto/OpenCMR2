#pragma once
// Shadow-map filter taps. src/render/shaders/fixed.frag repeats this maths in GLSL;
// keep the two in step. A golden-angle (Vogel) disk spreads taps evenly, so a
// per-pixel rotation turns shadow-map stair-steps into fine noise.
#include <cmath>
constexpr int SHADOW_FILTER_TAPS = 12;
constexpr float SHADOW_FILTER_MIN_RADIUS = 1.5f; // texels
// Filter radius in shadow-map texels for the softness setting (0..4 texels).
inline float Shadow_FilterRadius(float softnessTexels) {
    return softnessTexels > SHADOW_FILTER_MIN_RADIUS ? softnessTexels : SHADOW_FILTER_MIN_RADIUS;
}
// Offset of tap i (unit disk), rotated by rotation radians.
inline void Shadow_FilterTap(int i, float rotation, float *x, float *y) {
    const float golden = 2.39996323f;
    float r = std::sqrt((i + 0.5f) / SHADOW_FILTER_TAPS);
    float a = i * golden + rotation;
    *x = std::cos(a) * r;
    *y = std::sin(a) * r;
}
