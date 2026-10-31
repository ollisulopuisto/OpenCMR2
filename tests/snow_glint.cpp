// Snow glints (video.snow_glint): ice-crystal glinting on snowbanks and flakes.
// Pins the deterministic facet seed, the half-vector specular lobe, the
// car-light cone boost, the flake twinkle and the short fade envelope.
#include "port/types.h"
#include "SnowGlint.h"
#include <cstdio>
#include <cstdlib>

static void check(bool ok, const char *msg) {
    if (!ok) { fprintf(stderr, "snow_glint: %s\n", msg); exit(1); }
}

int main() {
    // Facet seed: deterministic, quantized to 2-world-unit cells, never zero.
    check(SnowGlint_FacetSeed(1000, 2000) == SnowGlint_FacetSeed(1000, 2000),
          "facet seed must be deterministic");
    check(SnowGlint_FacetSeed(0, 0) != SnowGlint_FacetSeed(1 << 17, 0),
          "next cell must reseed");
    check(SnowGlint_FacetSeed(0, 0) != 0, "origin seed must not be zero");

    // Specular lobe with a jitter-free facet (seed bytes centered): peaks when
    // the normal bisects sun and view, dies off-axis and when sun opposes view.
    const unsigned CENTER = 127u | (127u << 8);
    float aligned = SnowGlint_Specular(0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, CENTER);
    float offaxis = SnowGlint_Specular(0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, CENTER);
    check(aligned > 0.99f, "specular must peak on half-vector alignment");
    check(offaxis < 0.05f, "specular must die off-axis");
    check(SnowGlint_Specular(0.f, 1.f, 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, CENTER) == 0.f,
          "opposed sun and view must give zero");
    check(SnowGlint_Specular(0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, CENTER) == 0.f,
          "zero normal must give zero");

    // Cone boost: inside the beam axis brightens, behind and past reach go dark,
    // a directionless or dead light boosts nothing.
    check(SnowGlint_ConeBoost(15.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f) > 0.3f,
          "inside beam must boost");
    check(SnowGlint_ConeBoost(-15.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f) == 0.f,
          "behind the light must not boost");
    check(SnowGlint_ConeBoost(60.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f) == 0.f,
          "past reach must not boost");
    check(SnowGlint_ConeBoost(15.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f) == 0.f,
          "zero light direction must not boost");
    check(SnowGlint_ConeBoost(15.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f) == 0.f,
          "zero intensity must not boost");

    // Flake twinkle: deterministic, phase-varying, bounded; with no forward
    // scatter it stays in the quiet base band.
    check(SnowGlint_FlakeTwinkle(1234u, 0.0f, 1.f) == SnowGlint_FlakeTwinkle(1234u, 0.0f, 1.f),
          "twinkle must be deterministic");
    bool varies = false;
    for (int i = 0; i < 8; ++i)
        if (SnowGlint_FlakeTwinkle(1234u, i / 8.f, 1.f) != SnowGlint_FlakeTwinkle(1234u, 0.0f, 1.f))
            varies = true;
    check(varies, "twinkle must vary with phase");
    for (int i = 0; i < 16; ++i) {
        float v = SnowGlint_FlakeTwinkle((unsigned)i * 977u + 13u, i / 16.f, 1.f);
        check(v >= 0.3f && v <= 2.4f, "twinkle out of range");
        float q = SnowGlint_FlakeTwinkle((unsigned)i * 977u + 13u, i / 16.f, 0.f);
        check(q >= 0.3f && q <= 1.2f, "scatterless twinkle must stay in the base band");
    }

    // Fade envelope: quick fade-in, full peak, quick fade-out, dead outside life.
    check(SnowGlint_FadeAlpha(0, SNOW_GLINT_ALPHA_MAX) == 0, "new glint starts dark");
    check(SnowGlint_FadeAlpha(2, SNOW_GLINT_ALPHA_MAX) == SNOW_GLINT_ALPHA_MAX, "glint reaches peak");
    check(SnowGlint_FadeAlpha(SNOW_GLINT_LIFE, SNOW_GLINT_ALPHA_MAX) == 0, "spent glint invisible");
    check(SnowGlint_FadeAlpha(SNOW_GLINT_LIFE - 2, SNOW_GLINT_ALPHA_MAX) == SNOW_GLINT_ALPHA_MAX / 2,
          "linear fade-out");
    check(SnowGlint_FadeAlpha(-1, SNOW_GLINT_ALPHA_MAX) == 0, "negative age dark");
    check(SnowGlint_FadeAlpha(SNOW_GLINT_LIFE + 1, SNOW_GLINT_ALPHA_MAX) == 0, "past life dark");

    printf("snow_glint OK\n");
    return 0;
}
