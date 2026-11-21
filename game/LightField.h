#ifndef _LIGHT_FIELD_H
#define _LIGHT_FIELD_H

#include "FixedPoint.h"
#include <cmath>

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
    int beamLength;  // FOG PASS ONLY: beam run in world units (16.16) the fog pass projects; 0 = the default 30 units
    FixVector fogDir; // FOG PASS ONLY: replaces dir in the fog pass when non-zero (tail lamps); dir stays the real lamp direction for motes, glints and snow
    int fogScale;    // FOG PASS ONLY: 16.16 multiplier on intensity in the fog pass; 0 = 1.0
};

extern FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
extern int g_fogLightSourceCount;
inline int g_fogLightDropped = 0;       // lights this step that found the field full and were lost (diagnostic)
inline int g_fogLightDroppedPrio = 0;   // of those, priority lights (headlights)
inline unsigned g_fogLightPrioMask = 0; // bit i set: source i is a priority light

inline void LightField_Clear(void) { g_fogLightSourceCount = 0; g_fogLightDropped = 0; g_fogLightDroppedPrio = 0; g_fogLightPrioMask = 0; }

// Records one enabled light. When the field is full a priority light (headlight) replaces the
// last non-priority source; any other light is lost and counted. Returns the count.
inline int LightField_Add(const FixVector *pos, const FixVector *dir, int intensity, int r, int g, int b, int priority = 0, int beamLength = 0, const FixVector *fogDir = 0, int fogScale = 0) {
    if (!pos || intensity <= 0)
        return g_fogLightSourceCount;
    int slot = -1;
    if (g_fogLightSourceCount < LIGHT_FIELD_MAX) {
        slot = g_fogLightSourceCount++;
    } else {
        if (priority)
            for (int i = LIGHT_FIELD_MAX - 1; i >= 0; --i)
                if (!(g_fogLightPrioMask & (1u << i))) { slot = i; break; }
        if (slot < 0) { ++g_fogLightDropped; if (priority) ++g_fogLightDroppedPrio; return g_fogLightSourceCount; }
    }
    FogLightSource *p = &g_fogLightSources[slot];
    if (priority) g_fogLightPrioMask |= 1u << slot; else g_fogLightPrioMask &= ~(1u << slot);
    p->pos = *pos;
    p->dir = dir ? *dir : FixVector{0, 0, 0};
    p->intensity = intensity > 0x10000 ? 0x10000 : intensity;
    p->r = r; p->g = g; p->b = b;
    p->beamLength = beamLength > 0 ? beamLength : 0;
    p->fogDir = fogDir ? *fogDir : FixVector{0, 0, 0};
    p->fogScale = fogScale > 0 ? fogScale : 0;
    return g_fogLightSourceCount;
}


// Tail lights (video.tail_light_beam): rear lamps are not headlights. The shared beam wedge gave them a long red
// searchlight over the grass. A tail lamp instead lights the ground close behind the car: a short run, splayed outward
// (left lamp to the left, right lamp to the right) so the two wedges fan into a wide glow, tilted a little to the ground.
// dir is the lamp's world direction (any length), offset the lamp's world position minus the car's. percent scales the
// run (100 = 6 world units, 0 = off, the original 30-unit beam, max 300). Returns false when nothing changes.
// Pure maths, see tests/light_field.cpp.
constexpr int TAIL_BEAM_LENGTH_UNITS = 6;
constexpr float TAIL_BEAM_SPLAY = 0.61f;    // radians, about 35 degrees
constexpr float TAIL_BEAM_PITCH = 0.12f;    // downward share of the unit direction
constexpr float TAIL_BEAM_INTENSITY = 0.7f; // a tail lamp is dimmer than a headlamp
// Guard for the slot 0 = red rear lamp assumption (shipped car light data is not in the repo, so it
// cannot be checked in CI): a lamp only gets the tail treatment when it sits in the rear half of the
// car (offset from the car centre against the heading) and its beam points backward too. heading is
// Car::right, the driving direction in this code base. Anything else keeps the original beam.
inline bool LightField_IsRearLamp(const FixVector *offset, const FixVector *dir, const FixVector *heading) {
    double po = (double)offset->x * heading->x + (double)offset->y * heading->y + (double)offset->z * heading->z;
    double pd = (double)dir->x * heading->x + (double)dir->y * heading->y + (double)dir->z * heading->z;
    return po < 0.0 && pd < 0.0;
}
inline bool LightField_TailBeam(const FixVector *dir, const FixVector *offset, int percent, FixVector *outDir, int *outLength, float *outIntensityScale) {
    if (percent <= 0) return false;
    if (percent > 300) percent = 300;
    float hx = dir->x / 65536.f, hz = dir->z / 65536.f;
    float hl = std::sqrt(hx * hx + hz * hz);
    if (!(hl > 0.05f)) return false;           // pointing straight up or down: no heading to splay from
    hx /= hl; hz /= hl;
    float px = -hz, pz = hx;                   // horizontal perpendicular
    float side = (offset->x / 65536.f) * px + (offset->z / 65536.f) * pz >= 0.f ? 1.f : -1.f;
    float c = std::cos(TAIL_BEAM_SPLAY), sn = std::sin(TAIL_BEAM_SPLAY) * side;
    float flat = std::sqrt(1.f - TAIL_BEAM_PITCH * TAIL_BEAM_PITCH);
    outDir->x = (int)std::lround((hx * c + px * sn) * flat * 65536.f);
    outDir->z = (int)std::lround((hz * c + pz * sn) * flat * 65536.f);
    outDir->y = (int)std::lround(-TAIL_BEAM_PITCH * 65536.f);
    *outLength = TAIL_BEAM_LENGTH_UNITS * percent * 65536 / 100;
    *outIntensityScale = TAIL_BEAM_INTENSITY;
    return true;
}

// Near-plane clip of a view-space beam segment (video.fog_lights projection):
// the lamp is in front (lampView->z >= 0x10000), the beam end is behind the
// camera (endView->z < 0x10000). Returns the segment's intersection with the
// projection threshold plane so the screen wedge keeps its true direction
// instead of the behind-camera sentinel (-100,-100 px), which mirrored rear
// beams toward the upper-left corner. 16.16 fixed point, 64-bit intermediates.
inline void LightField_ClipBeamToNearPlane(FixVector *out, const FixVector *lampView, const FixVector *endView) {
    int dz = lampView->z - endView->z;      // > 0: lamp in front, end behind
    int num = lampView->z - 0x10100;
    out->x = lampView->x + (int)(((__int64)(endView->x - lampView->x) * num) / dz);
    out->y = lampView->y + (int)(((__int64)(endView->y - lampView->y) * num) / dz);
    // 0x10100, not 0x10000: the anchor's view->world->view round trip through
    // FixMul loses ~1 ulp, and exactly-0x10000 comes back below the in-front
    // threshold - the re-projection then returns the (-100,-100) sentinel as
    // the lamp uv (executed-scene-matrix finding, bonnet/bumper lamps).
    out->z = 0x10100;
}

#endif
