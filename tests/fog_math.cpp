#include <initializer_list>
#include "render/fog_math.h"
#include <cstdio>
#include <cmath>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    // Density clamps to 0..200 percent; 100 = the deliberate heavy starting look.
    CHECK(Fog_Density(-5) == 0.f);
    CHECK(Fog_Density(0) == 0.f);
    CHECK(std::fabs(Fog_Density(100) - 0.03f) < 1e-7f);
    CHECK(std::fabs(Fog_Density(200) - 0.06f) < 1e-7f);
    CHECK(std::fabs(Fog_Density(900) - 0.06f) < 1e-7f);
    // At 100 percent the curve hits its documented anchors (23 units ~50%, 60 ~84%, 150 ~99%).
    float d = Fog_Density(100);
    CHECK(std::fabs(Fog_Factor(23.f, d) - 0.50f) < 0.02f);
    CHECK(std::fabs(Fog_Factor(60.f, d) - 0.83f) < 0.02f);
    CHECK(Fog_Factor(150.f, d) > 0.97f);
    CHECK(Fog_Factor(0.f, d) == 0.f && Fog_Factor(-3.f, d) == 0.f && Fog_Factor(10.f, 0.f) == 0.f);
    CHECK(Fog_Factor(50.f, Fog_Density(200)) > Fog_Factor(50.f, Fog_Density(50)));
    // ViewZ round trip: depth from near/far recovers the distance (the dof.frag formula).
    for (float nearZ : {0.1f, 0.5f, 1.0f}) for (float farZ : {150.f, 400.f}) {
        for (float z = nearZ * 1.01f; z < farZ; z *= 1.7f) {
            float depth = farZ / (farZ - nearZ) * (1.f - nearZ / z);
            float got = Fog_ViewZ(depth, nearZ, farZ);
            CHECK(std::fabs(got - z) / z < 0.001f);
        }
    }
    CHECK(Fog_ViewZ(0.5f, 0.f, 0.f) == 0.f);          // unknown projection
    CHECK(Fog_ViewZ(0.5f, 2.f, 1.f) == 0.f);          // far <= near rejected
    // Raw-depth fallback: nothing below depth 0.990, heavier toward the far plane, zero density zero.
    CHECK(Fog_FactorRawDepth(0.989f, 0.03f) == 0.f);
    CHECK(Fog_FactorRawDepth(1.0f, 0.f) == 0.f);
    CHECK(Fog_FactorRawDepth(0.995f, 0.03f) > 0.f);
    CHECK(Fog_FactorRawDepth(0.995f, 0.03f) < Fog_FactorRawDepth(1.0f, 0.03f));
    CHECK(Fog_FactorRawDepth(1.0f, 0.03f) < 1.f);     // exponential never quite reaches 1
    // Height gradient: 0.55 at the top, 1.45 at the bottom, monotonic between.
    CHECK(std::fabs(Fog_HeightScale(0.f) - 0.55f) < 1e-6f);
    CHECK(std::fabs(Fog_HeightScale(1.f) - 1.45f) < 1e-6f);
    CHECK(Fog_HeightScale(0.3f) < Fog_HeightScale(0.6f));
    // Sky cap applies only at the far plane.
    CHECK(Fog_SkyCap(0.99f, 1.0f) == 0.85f);
    CHECK(Fog_SkyCap(0.99f, 0.9f) == 0.99f);
    CHECK(Fog_SkyCap(0.5f, 1.0f) == 0.5f);
    // Sun glow: 1 at the sun, 0 at and beyond about 0.625 aspect-corrected screen heights, cubic between.
    CHECK(std::fabs(Fog_SunGlow(0.f, 0.f, 1.f) - 1.f) < 1e-6f);
    CHECK(Fog_SunGlow(0.625f, 0.f, 1.f) == 0.f);
    CHECK(Fog_SunGlow(1.f, 0.f, 1.f) == 0.f);
    CHECK(Fog_SunGlow(0.3f, 0.f, 1.f) > 0.f && Fog_SunGlow(0.3f, 0.f, 1.f) < 1.f);
    CHECK(Fog_SunGlow(0.f, 0.f, 0.f) == 0.f);
    CHECK(Fog_SunGlow(0.1f, 0.f, 0.5f) < Fog_SunGlow(0.1f, 0.f, 1.f));
    // Colour: cool at zero glow, warm at full glow, channels inside 0..1.
    float r0, g0, b0, r1, g1, b1;
    Fog_Color(0.f, &r0, &g0, &b0); Fog_Color(1.f, &r1, &g1, &b1);
    CHECK(b0 > r0);                  // cool
    CHECK(r1 > b1);                  // warm
    for (float t : {0.f, .25f, .5f, .75f, 1.f, 1.5f, -0.5f}) {
        float r, g, b; Fog_Color(t, &r, &g, &b);
        CHECK(r >= 0.f && r <= 1.f && g >= 0.f && g <= 1.f && b >= 0.f && b <= 1.f);
    }
    if (!fails) std::puts("fog_math ok");
    return fails;
}
