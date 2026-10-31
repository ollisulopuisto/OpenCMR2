#include "port/types.h"
#include "LightField.h"
#include <cstdio>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
int g_fogLightSourceCount = 0;
int main(){
    // Clear/add/count, cap at LIGHT_FIELD_MAX, intensity clamped, zero/negative dropped.
    LightField_Clear();
    CHECK(g_fogLightSourceCount == 0);
    FixVector pos = {0x10000, 0x20000, 0x30000};
    FixVector dir = {0, 0, 0x10000};
    CHECK(LightField_Add(&pos, &dir, 0x8000, 0x10000, 0x9000, 0x7500) == 1);
    CHECK(g_fogLightSources[0].intensity == 0x8000);
    CHECK(g_fogLightSources[0].pos.x == 0x10000 && g_fogLightSources[0].r == 0x10000);
    CHECK(g_fogLightSources[0].dir.z == 0x10000);
    // Overbright sources (hazard flash) clamp to full.
    LightField_Add(&pos, nullptr, 0x20000, 0, 0, 0);
    CHECK(g_fogLightSources[1].dir.x == 0 && g_fogLightSources[1].dir.z == 0);   // unknown direction zeroes
    CHECK(g_fogLightSources[1].intensity == 0x10000);
    // Dead lights are not recorded.
    int before = g_fogLightSourceCount;
    CHECK(LightField_Add(&pos, &dir, 0, 1, 1, 1) == before);
    CHECK(LightField_Add(&pos, &dir, -0x4000, 1, 1, 1) == before);
    CHECK(LightField_Add(nullptr, &dir, 0x8000, 1, 1, 1) == before);
    // The field caps at LIGHT_FIELD_MAX and keeps the first lights recorded.
    for (int i = 0; i < 16; ++i) LightField_Add(&pos, &dir, 0x1000, i, i, i);
    CHECK(g_fogLightSourceCount == LIGHT_FIELD_MAX);
    // Clear resets for the next step.
    LightField_Clear();
    CHECK(g_fogLightSourceCount == 0);
    CHECK(LightField_Add(&pos, &dir, 0x10000, 1, 2, 3) == 1);
    if (!fails) std::puts("light_field ok");
    return fails;
}
