#pragma once
#include <math.h>
// Brake disc glow (video.brake_glow, default on): a per-wheel heat model, visual only.
// Pure inline model so tests/brake_glow.cpp exercises the decision logic without the
// renderer; game/BrakeGlow.cpp owns texture/draw state. No physics inputs are written.

// Fixed 16.16 reference speed for full heating: 1.5 world units/s, the game's own
// full-speed scale (the speed-driven effects in Race_ApplySpeedEffectParams normalise
// speed/1.5), so the model's speed range covers real driving.
#define BRAKE_GLOW_SPEED_REF 98304

inline float BrakeGlow_Clamp01(float v) { return v < 0.f ? 0.f : v > 1.f ? 1.f : v; }

// One tick per wheel. heat[0..1] is front-left, front-right, rear-left, rear-right
// (the pWheelNodes/physics order: axle = index < 2). brakeInput, brakeBias and speed
// are 16.16 fixed; dt is g_physicsTimeStep / 65536 (1.0 at the original 25 Hz step).
// Heating scales with pedal, axle share of the bias and speed (kinetic energy);
// cooling is proportional to the current heat (discs radiate faster when hot) plus
// airflow from speed, with a small constant drain so discs reach cold. [0,1] clamped.
inline void BrakeGlow_UpdateHeat(float heat[4], int brakeInput, int brakeBias, int speed, float dt) {
    float brake = BrakeGlow_Clamp01(brakeInput / 65536.f);
    float bias = BrakeGlow_Clamp01(brakeBias / 65536.f);
    float speed01 = BrakeGlow_Clamp01((float)speed / (float)BRAKE_GLOW_SPEED_REF);
    for (int w = 0; w < 4; w++) {
        float axle = w < 2 ? 2.f * bias : 2.f * (1.f - bias);
        float in = 0.5f * brake * axle * (0.05f + 0.95f * speed01) * dt;
        float cool = heat[w] * (0.10f + 0.45f * speed01) * dt + 0.004f * dt;
        heat[w] = BrakeGlow_Clamp01(heat[w] + in - cool);
    }
}

// Below this heat the disc draws nothing (the faint tail would only add overdraw).
#define BRAKE_GLOW_THRESHOLD 0.08f
// Disc quad radius bounds in world units. The quad scales off the measured wheel
// mesh radius (BrakeGlow_WheelExtent) so the glow stays inside the hubcap of any
// wheel model; this is only the clamp range and the fallback when no wheel of
// the car has a readable mesh (an unreadable wheel first borrows a sibling's
// extent, BrakeGlow_FillMissingExtents).
#define BRAKE_GLOW_RADIUS 0.30f
// Glow radius for a wheel of measured mesh radius wheelExtent (bounding-box half of
// the largest dimension, ie the tyre radius): a brake disc sits well inside the rim,
// roughly half the wheel face, so scale down and clamp to sane bounds. Below ~0.10
// the glow reads as a point, above 0.30 it spills past the wheel.
inline float BrakeGlow_DiscRadius(float wheelExtent) {
    float r = 0.55f * wheelExtent;
    return r < 0.10f ? 0.10f : r > BRAKE_GLOW_RADIUS ? BRAKE_GLOW_RADIUS : r;
}

// Bounding-box radius of one vertex stream: half the largest x/y/z span over
// count records of strideBytes each, the position being the three leading floats
// of a record. The engine has exactly one vertex layout (0x30 bytes, position at
// offset 0: MeshVertexF, LayoutChecks.cpp, and the deform path's DeformFloatVertex
// has the same size and leading position floats, LayoutChecks.cpp:421), so this
// reads any mesh's stream correctly. Robust to the mesh origin not being the hub.
// Returns 0 for an unreadable stream (null, empty, implausible count, or a stride
// too small to hold a position).
inline float BrakeGlow_MeshExtent(const void *verts, int strideBytes, int count) {
    if (verts == 0 || count <= 0 || count > 100000 || strideBytes < 12)
        return 0.f;
    const char *base = (const char *)verts;
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
    for (int i = 0; i < count; i++) {
        const float *p = (const float *)(base + (long)i * strideBytes);
        for (int a = 0; a < 3; a++) {
            if (p[a] < lo[a]) lo[a] = p[a];
            if (p[a] > hi[a]) hi[a] = p[a];
        }
    }
    float best = 0.f;
    for (int a = 0; a < 3; a++) {
        float d = 0.5f * (hi[a] - lo[a]);
        if (d > best) best = d;
    }
    return best;
}

// A wheel whose mesh is unreadable (extent 0) borrows the largest measured
// sibling extent: the four wheels of a car share a tyre radius, so any readable
// sibling beats the oversized fixed fallback. All-zero stays zero.
inline void BrakeGlow_FillMissingExtents(float extent[4]) {
    float best = 0.f;
    for (int w = 0; w < 4; w++)
        if (extent[w] > best) best = extent[w];
    if (best <= 0.f)
        return;
    for (int w = 0; w < 4; w++)
        if (extent[w] <= 0.f) extent[w] = best;
}

// Depth of the disc behind the wheel face. The quad used to sit in the hub's own plane, level with (and
// drawn over) the hubcap. A real disc lives inboard of the rim, so the quad moves toward the car body
// along the axle, by this share of the wheel radius; the depth test then lets the spokes and hubcap hide
// it and the glow shows only through the gaps. insetPercent is video.brake_glow_inset (default 100, 0 = old
// position, max 400).
#define BRAKE_GLOW_INSET_FRACTION 0.25f
// Fills outOffset with the world-space shift for one wheel: axleDir = the car body's lateral axis in world units (spin-invariant; the wheel node's own rows turn with spin),
// pointed toward the car centre (carPos is the car body position in the same world units). Returns 0
// (no shift) when the inset is off, the wheel radius is unknown, or the wheel sits on the car's centre line.
inline int BrakeGlow_InboardOffset(const float world[16], const float axleDir[3], const float carPos[3], float wheelExtent, int insetPercent, float outOffset[3]) {
    outOffset[0] = outOffset[1] = outOffset[2] = 0.f;
    if (insetPercent <= 0 || !(wheelExtent > 0.f))
        return 0;
    if (insetPercent > 400) insetPercent = 400;
    const float *axle = axleDir, *pos = world + 12;
    float len = sqrtf(axle[0] * axle[0] + axle[1] * axle[1] + axle[2] * axle[2]);
    if (!(len > 1e-4f))
        return 0;
    float side = 0.f;                                   // + when the car centre lies along +axle
    for (int a = 0; a < 3; a++)
        side += axle[a] / len * (carPos[a] - pos[a]);
    if (side > -1e-3f && side < 1e-3f)
        return 0;
    float k = (side > 0.f ? 1.f : -1.f) * BRAKE_GLOW_INSET_FRACTION * (insetPercent / 100.f) * wheelExtent / len;
    for (int a = 0; a < 3; a++)
        outOffset[a] = axle[a] * k;
    return 1;
}

// Builds the disc quad for one wheel from its node's world matrix (the SceneNode
// worldF layout: rows right/up/forward, position in [12..14]). The quad spans the
// up/forward plane, ie the plane the disc rotates in when the axle is the node's
// right axis. Returns 1 when the quad should be drawn (heat above threshold) and
// fills outPos (4 world corners), outUV and outAlpha (the heat, for the vertex
// colour). Pure so tests pin the geometry without a GL context.
inline int BrakeGlow_BuildDiscQuad(const float world[16], float radius, float heat,
                                   float outPos[4][3], float outUV[4][2], float *outAlpha) {
    if (heat <= BRAKE_GLOW_THRESHOLD)
        return 0;
    const float *up = world + 4, *fwd = world + 8, *pos = world + 12;
    // Corner signs in (forward, up) order; uv follows the same sweep.
    static const float kSign[4][2] = { {-1, 1}, {1, 1}, {1, -1}, {-1, -1} };
    static const float kUV[4][2] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
    for (int v = 0; v < 4; v++) {
        for (int a = 0; a < 3; a++)
            outPos[v][a] = pos[a] + kSign[v][0] * radius * fwd[a] + kSign[v][1] * radius * up[a];
        outUV[v][0] = kUV[v][0];
        outUV[v][1] = kUV[v][1];
    }
    *outAlpha = heat;
    return 1;
}

// Renderer-side entry points (game/BrakeGlow.cpp).
void BrakeGlow_TickStep(int car, float dtSteps);
void BrakeGlow_Draw(void);
void BrakeGlow_Reset(void);
