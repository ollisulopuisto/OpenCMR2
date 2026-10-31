#ifndef _LIGHT_FIELD_H
#define _LIGHT_FIELD_H

#include "FixedPoint.h"

// Light field (video.fog_lights): the car lights the atmosphere responds to, so
// headlights and rear lights visibly cut the fog. The per-step car-light update
// records each enabled car-light glow - world position, intensity and tint, the
// same values the glow sprites use - and the per-frame view draw projects them
// to the screen and hands them to the renderer (Gfx_SetFogLights). Render-only:
// nothing here feeds the car, the camera or physics. Covered by
// tests/light_field.cpp.
#define LIGHT_FIELD_MAX 8

struct FogLightSource {
    FixVector pos;   // world position (16.16)
    int intensity;   // 16.16, clamped to 0..0x10000
    int r, g, b;     // tint (16.16, unclamped - indicator amber exceeds 1.0 in R)
};

extern FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
extern int g_fogLightSourceCount;

inline void LightField_Clear(void) { g_fogLightSourceCount = 0; }

// Records one enabled light; drops it when the field is full. Returns the count.
inline int LightField_Add(const FixVector *pos, int intensity, int r, int g, int b) {
    if (!pos || intensity <= 0 || g_fogLightSourceCount >= LIGHT_FIELD_MAX)
        return g_fogLightSourceCount;
    FogLightSource *p = &g_fogLightSources[g_fogLightSourceCount++];
    p->pos = *pos;
    p->intensity = intensity > 0x10000 ? 0x10000 : intensity;
    p->r = r; p->g = g; p->b = b;
    return g_fogLightSourceCount;
}

#endif
