#ifndef OPENCMR2_WEATHER_GLINT_H
#define OPENCMR2_WEATHER_GLINT_H
#include <cmath>
// Screen-space weather glints (video.snow_glint): the port-layer precipitation
// overlay responds to light, matching the game-side flake response. Flakes
// twinkle toward the sun (forward scatter, where real snow glints) and
// brighten inside projected car light beams, tinted per light. Shared by the
// OpenGL executor (RenderWeather) and the SDL_GPU renderer (RenderParticles);
// pure maths, covered by tests/weather_glint.cpp and pixel-pinned through the
// GL executor by tests/gl_effects_runtime.cpp.

// Per-flake phase: stable per particle, animates as its life counts down.
inline float WeatherGlint_Phase(int index, float life) {
    return index * 2.399963f + life * 5.0f;
}

// Proximity 0..1 to the sun's screen position, aspect-corrected: forward
// scatter means flakes between the view and the sun glint most.
inline float WeatherGlint_SunProximity(float px, float py, float sunX, float sunY, int sunVisible, float aspect) {
    if (!sunVisible) return 0.f;
    float dx = (px - sunX) * aspect, dy = py - sunY;
    float d = std::sqrt(dx * dx + dy * dy);
    const float R = 0.35f;
    if (d >= R) return 0.f;
    float t = 1.f - d / R;
    return t * t;
}

// Brightness multiplier: a quiet base shimmer (facet glinting) plus a strong
// sun-ward forward-scatter lobe. Mean ~1 away from the sun.
inline float WeatherGlint_Twinkle(float phase, float sunProx) {
    float base = 0.9f + 0.2f * std::sin(phase);
    return base + sunProx * (0.5f + 0.5f * std::sin(phase * 1.7f)) * 1.6f;
}

// Additive tinted boost of a screen point inside one projected car light's
// glow: radial falloff within a radius that grows with intensity.
inline void WeatherGlint_LightBoost(float px, float py, float aspect,
                                    float lx, float ly, float lint,
                                    float lr, float lg, float lb,
                                    float *r, float *g, float *b) {
    if (!(lint > 0.f)) return;
    float dx = (px - lx) * aspect, dy = py - ly;
    float d = std::sqrt(dx * dx + dy * dy);
    float radius = 0.05f + 0.12f * lint;
    if (d >= radius) return;
    float t = 1.f - d / radius;
    float boost = lint * t * t * 0.9f;
    *r += boost * lr;
    *g += boost * lg;
    *b += boost * lb;
}
#endif
