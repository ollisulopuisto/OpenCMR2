#include "port/types.h"
#include "LightField.h"
#include <cstdio>
#include <cmath>
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
    // Near-plane beam clip: lamp in front, end behind - intersection lands on
    // the threshold plane and keeps the segment's lateral direction.
    {
        FixVector lamp = {0x20000, 0x30000, 0x80000};   // 8 units in front
        FixVector end  = {-0x60000, 0x70000, -0x80000}; // 8 behind, mirrored x/y
        FixVector clip;
        LightField_ClipBeamToNearPlane(&clip, &lamp, &end);
        CHECK(clip.z == 0x10100);
        // t = (8-1)/(8+8) = 7/16 along lamp->end
        CHECK(clip.x == 0x20000 + (int)(((long long)(-0x60000 - 0x20000) * (0x80000 - 0x10100)) / 0x100000));
        CHECK(clip.y == 0x30000 + (int)(((long long)(0x70000 - 0x30000) * (0x80000 - 0x10100)) / 0x100000));
        // Straight-ahead beam ending just behind the camera clips to the axis:
        // the wedge collapses toward the screen centre, never to a corner.
        FixVector lamp2 = {0, 0, 0x18000};
        FixVector end2 = {0, 0, -0x8000};
        LightField_ClipBeamToNearPlane(&clip, &lamp2, &end2);
        CHECK(clip.x == 0 && clip.y == 0 && clip.z == 0x10100);
        // Wide lateral offsets survive clipping with correct sign (no mirroring):
        // end up-left behind the camera keeps the clipped point up-left.
        FixVector lamp3 = {0, 0, 0x40000};
        FixVector end3 = {0x40000, -0x40000, -0x40000}; // view y down = up-left end
        LightField_ClipBeamToNearPlane(&clip, &lamp3, &end3);
        CHECK(clip.x > 0 && clip.y < 0);
    }
    // Screen-space wedge regression (the 2026-11-01 streak): replicate the
    // exact per-light projection sequence from StageObject's fog-light block
    // (same formulas as FixMatrix_ProjectWorldPointToView/ProjectToScreen:
    // symmetric perspective, screen = halfRes * clip / z, y negated), with a
    // chase camera 5 units behind a lamp whose beam runs 30 units backwards.
    {
        const int resX = 1920, resY = 1080;
        auto project=[&](const FixVector &vView, int out[2]){   // view space -> 16.16 px, sentinel on behind
            if (vView.z < 0x10000) { out[0] = -0x640000; out[1] = -0x640000; return; }
            // 16.16 px like ProjectToScreen: ratio (x/z) * halfRes, all in 16.16.
            out[0] = (int)(((long long)vView.x * (resX / 2) * 65536) / vView.z) + ((resX / 2) << 16);
            out[1] = (int)(((long long)(-vView.y) * (resY / 2) * 65536) / vView.z) + ((resY / 2) << 16);
        };
        // Level rear beam 2 units left of the camera axis: true wedge is
        // purely horizontal (lamp and clipped end share the height).
        FixVector lampView = {-0x20000, 0, 0x50000};            // 2 left, 5 ahead
        FixVector endView  = {-0x20000, 0, -0x1e0000 + 0x50000};// 30 behind the lamp
        int lamp[2]; project(lampView, lamp);
        int sentinel[2]; project(endView, sentinel);
        CHECK(sentinel[0] == -0x640000 && sentinel[1] == -0x640000);          // old code used this
        // Old wedge direction: lamp (left-centre) -> sentinel corner (up-left):
        float oldDx = (float)(sentinel[0] - lamp[0]) * resY / resX, oldDy = (float)(sentinel[1] - lamp[1]);
        CHECK(oldDy < -100.f * 65536.f);                                     // the false upward streak
        // Fixed path: clip at the near plane, then project.
        FixVector clip; LightField_ClipBeamToNearPlane(&clip, &lampView, &endView);
        int end[2]; project(clip, end);
        CHECK(end[0] != -0x640000);
        float newDy = (float)(end[1] - lamp[1]);
        CHECK(newDy > -2.f * 65536.f && newDy < 2.f * 65536.f);              // level beam: no vertical drift
        CHECK(end[0] < lamp[0]);                                             // true direction: further left
        // Upward-tilted rear beam keeps its legitimate upward slope (no flattening):
        FixVector tiltEnd = {-0x20000, 0x60000, -0x1e0000 + 0x50000};
        LightField_ClipBeamToNearPlane(&clip, &lampView, &tiltEnd);
        project(clip, end);
        CHECK(end[1] < lamp[1] - (50 << 16));                                // tilts up on screen: correct
        // R look-back mirror case: camera faces the other way, headlight beam
        // crosses the near plane the same way; clip still holds direction.
        FixVector lampR = {0x20000, 0, 0x50000};
        FixVector endR  = {0x20000, 0, -0x1e0000 + 0x50000};
        LightField_ClipBeamToNearPlane(&clip, &lampR, &endR);
        project(clip, end);
        int lamp2[2]; project(lampR, lamp2);
        CHECK(end[0] > lamp2[0]);                                            // true direction: further right
        CHECK(end[1] > lamp2[1] - (2 << 16) && end[1] < lamp2[1] + (2 << 16));
    }
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
    // Full field: a priority light (headlight) replaces a non-priority one; a non-priority light is lost and counted.
    LightField_Clear();
    for (int i = 0; i < LIGHT_FIELD_MAX; ++i) LightField_Add(&pos, &dir, 0x1000, 1, 1, 1);
    LightField_Add(&pos, &dir, 0x1000, 2, 2, 2);
    CHECK(g_fogLightDropped == 1 && g_fogLightDroppedPrio == 0);
    CHECK(LightField_Add(&pos, &dir, 0x8000, 7, 7, 7, 1) == LIGHT_FIELD_MAX);
    CHECK(g_fogLightSources[LIGHT_FIELD_MAX - 1].r == 7 && g_fogLightDropped == 1);
    for (int i = 0; i < LIGHT_FIELD_MAX; ++i) LightField_Add(&pos, &dir, 0x8000, 8, 8, 8, 1);   // fills the other slots with priority
    LightField_Add(&pos, &dir, 0x8000, 9, 9, 9, 1);
    CHECK(g_fogLightDroppedPrio == 2);   // 7 replacements fit, the 8th and 9th priority lights are lost
    // Clear resets for the next step.
    LightField_Clear();
    CHECK(g_fogLightSourceCount == 0);
    CHECK(LightField_Add(&pos, &dir, 0x10000, 1, 2, 3) == 1);
    // Tail lamp beam: short, splayed outward per side, tilted down, off at 0.
    {
        FixVector back = {-0x10000, 0, 0};                 // lamp points backward (-x)
        FixVector leftOff = {-0x20000, 0, -0x8000}, rightOff = {-0x20000, 0, 0x8000};
        FixVector d1, d2; int len1, len2; float k1, k2;
        CHECK(!LightField_TailBeam(&back, &leftOff, 0, &d1, &len1, &k1));
        CHECK(LightField_TailBeam(&back, &leftOff, 100, &d1, &len1, &k1));
        CHECK(LightField_TailBeam(&back, &rightOff, 100, &d2, &len2, &k2));
        CHECK(len1 == 6 * 0x10000 && len2 == len1);                 // 6 units, not the 30-unit searchlight
        CHECK(d1.z * d2.z < 0 && d1.z == -d2.z);                    // lamps splay to opposite sides
        CHECK(d1.x < 0 && d2.x < 0 && d1.x == d2.x);                // both still point backward
        CHECK(d1.y < 0);                                            // tilted to the ground
        CHECK(d1.z < 0 && leftOff.z < 0);                           // splays toward its own side
        double n = std::sqrt((double)d1.x * d1.x + (double)d1.y * d1.y + (double)d1.z * d1.z) / 65536.0;
        CHECK(n > 0.999 && n < 1.001);                              // unit vector
        CHECK(k1 < 1.f && k1 > 0.f);                                // dimmer than a headlamp
        FixVector up = {0, 0x10000, 0};
        CHECK(!LightField_TailBeam(&up, &leftOff, 100, &d1, &len1, &k1));   // no heading
        CHECK(LightField_TailBeam(&back, &leftOff, 300, &d1, &len1, &k1) && len1 == 18 * 0x10000);
        CHECK(LightField_TailBeam(&back, &leftOff, 9999, &d1, &len1, &k1) && len1 == 18 * 0x10000);
        // Heading rotates the splay: a lamp pointing +z splays along x.
        FixVector backZ = {0, 0, 0x10000}; FixVector offZ = {0x8000, 0, 0x20000};
        CHECK(LightField_TailBeam(&backZ, &offZ, 100, &d1, &len1, &k1) && d1.z > 0 && d1.x != 0);
        // beamLength rides along through LightField_Add; default stays 0 (30 units).
        LightField_Clear();
        FixVector p0 = {0, 0, 0};
        LightField_Add(&p0, &back, 0x8000, 0x10000, 0, 0, 0, 6 * 0x10000);
        LightField_Add(&p0, &back, 0x8000, 0x10000, 0, 0);
        CHECK(g_fogLightSources[0].beamLength == 6 * 0x10000 && g_fogLightSources[1].beamLength == 0);
        LightField_Clear();
    }
    if (!fails) std::puts("light_field ok");
    return fails;
}
