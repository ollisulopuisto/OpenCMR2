// Screen-space weather glints (video.snow_glint): pins the port-layer flake
// maths shared by the GL executor and the SDL_GPU renderer.
#include "render/weather_glint.h"
#include <cstdio>
#include <cstdlib>

static void check(bool ok, const char *msg) {
    if (!ok) { fprintf(stderr, "weather_glint: %s\n", msg); exit(1); }
}

int main() {
    // Phase: deterministic, varies per particle and over its life.
    check(WeatherGlint_Phase(3, 2.0f) == WeatherGlint_Phase(3, 2.0f), "phase deterministic");
    check(WeatherGlint_Phase(3, 2.0f) != WeatherGlint_Phase(4, 2.0f), "phase per particle");
    check(WeatherGlint_Phase(3, 2.0f) != WeatherGlint_Phase(3, 1.0f), "phase animates");

    // Sun proximity: peak at the sun, zero beyond the radius, zero when hidden,
    // aspect-corrected on x.
    check(WeatherGlint_SunProximity(0.5f, 0.5f, 0.5f, 0.5f, 1, 1.78f) == 1.f, "peak at the sun");
    check(WeatherGlint_SunProximity(0.5f + 0.36f / 1.78f, 0.5f, 0.5f, 0.5f, 1, 1.78f) == 0.f, "zero past radius");
    check(WeatherGlint_SunProximity(0.5f, 0.5f, 0.5f, 0.5f, 0, 1.78f) == 0.f, "hidden sun no proximity");
    check(WeatherGlint_SunProximity(0.5f + 0.2f / 1.78f, 0.5f, 0.5f, 0.5f, 1, 1.78f) >
          WeatherGlint_SunProximity(0.5f + 0.3f / 1.78f, 0.5f, 0.5f, 0.5f, 1, 1.78f),
          "proximity falls off with distance");

    // Twinkle: base band away from the sun, strong lobe toward it.
    for (int i = 0; i < 16; ++i) {
        float v = WeatherGlint_Twinkle(i * 0.7f, 0.f);
        check(v >= 0.69f && v <= 1.11f, "base band out of range");
        float s = WeatherGlint_Twinkle(i * 0.7f, 1.f);
        check(s >= v - 0.01f && s <= 2.71f, "sun lobe out of range");
    }

    // Light boost: peak inside the glow, zero outside, tinted, dead light dark.
    float r = 0, g = 0, b = 0;
    WeatherGlint_LightBoost(0.5f, 0.5f, 1.78f, 0.5f, 0.5f, 1.f, 1.f, 0.5f, 0.25f, &r, &g, &b);
    check(r > 0.85f && g > 0.4f && b > 0.2f, "inside beam must boost with tint");
    r = g = b = 0;
    WeatherGlint_LightBoost(0.9f, 0.5f, 1.78f, 0.5f, 0.5f, 1.f, 1.f, 1.f, 1.f, &r, &g, &b);
    check(r == 0.f && g == 0.f && b == 0.f, "outside beam must not boost");
    WeatherGlint_LightBoost(0.5f, 0.5f, 1.78f, 0.5f, 0.5f, 0.f, 1.f, 1.f, 1.f, &r, &g, &b);
    check(r == 0.f, "dead light must not boost");

    printf("weather_glint OK\n");
    return 0;
}
