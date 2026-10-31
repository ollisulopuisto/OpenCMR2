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
    FixVector dir;   // world beam direction (16.16, zero when unknown)
    int intensity;   // 16.16, clamped to 0..0x10000
    int r, g, b;     // tint (16.16, unclamped - indicator amber exceeds 1.0 in R)
};

extern FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
extern int g_fogLightSourceCount;

inline void LightField_Clear(void) { g_fogLightSourceCount = 0; }

// Records one enabled light; drops it when the field is full. Returns the count.
inline int LightField_Add(const FixVector *pos, const FixVector *dir, int intensity, int r, int g, int b) {
    if (!pos || intensity <= 0 || g_fogLightSourceCount >= LIGHT_FIELD_MAX)
        return g_fogLightSourceCount;
    FogLightSource *p = &g_fogLightSources[g_fogLightSourceCount++];
    p->pos = *pos;
    p->dir = dir ? *dir : FixVector{0, 0, 0};
    p->intensity = intensity > 0x10000 ? 0x10000 : intensity;
    p->r = r; p->g = g; p->b = b;
    return g_fogLightSourceCount;
}


// Near-plane clip of a view-space beam segment (video.fog_lights projection):
// the lamp is in front (lampView->z >= 0x10000), the beam end is behind the
// camera (endView->z < 0x10000). Returns the segment's intersection with the
// projection threshold plane so the screen wedge keeps its true direction
// instead of the behind-camera sentinel (-100,-100 px), which mirrored rear
// beams toward the upper-left corner. 16.16 fixed point, 64-bit intermediates.
inline void LightField_ClipBeamToNearPlane(FixVector *out, const FixVector *lampView, const FixVector *endView) {
    int dz = lampView->z - endView->z;      // > 0: lamp in front, end behind
    int num = lampView->z - 0x10000;
    out->x = lampView->x + (int)(((__int64)(endView->x - lampView->x) * num) / dz);
    out->y = lampView->y + (int)(((__int64)(endView->y - lampView->y) * num) / dz);
    out->z = 0x10000;
}

#endif
