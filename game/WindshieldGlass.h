#ifndef OPENCMR2_WINDSHIELD_GLASS_H
#define OPENCMR2_WINDSHIELD_GLASS_H
// Runtime identification of the front windshield among the car's six window
// quads (CIN CarGlassProfile.windowVertices + the window descriptors in
// CarEffects.cpp). The descriptors mirror Z for the other side of the car, so
// Z is the lateral axis and no axis-name assumption is needed: the windshield
// is the window quad the cockpit eye looks through along the car's forward
// ray. Pure geometry, unit-tested in tests/windshield_glass.cpp; the
// integration feeds the CIN data and the cockpit eye/forward in car space.

struct WgVec3 { float x, y, z; };

static inline WgVec3 WgSub(WgVec3 a, WgVec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline WgVec3 WgCross(WgVec3 a, WgVec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static inline float WgDot(WgVec3 a, WgVec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

#define WG_WINDOWS 6

// One window quad in car space, built from the descriptor's four vertices
// (each Z-mirrored when its mirror flag is set).
struct WgQuad { WgVec3 v[4]; };

static inline void WindshieldGlass_BuildQuads(const int verts[WG_WINDOWS][3],
                                              const unsigned char desc[WG_WINDOWS][4],
                                              const int mirrorZ[WG_WINDOWS][4],
                                              WgQuad quads[WG_WINDOWS]) {
    for (int w = 0; w < WG_WINDOWS; w++) {
        for (int j = 0; j < 4; j++) {
            int vi = desc[w][j] % WG_WINDOWS;
            quads[w].v[j].x = (float)verts[vi][0];
            quads[w].v[j].y = (float)verts[vi][1];
            quads[w].v[j].z = mirrorZ[w][j] ? -(float)verts[vi][2] : (float)verts[vi][2];
        }
    }
}

// Ray (eye, dir) vs quad as two triangles (0,1,2) and (0,2,3); returns the
// nearest positive distance or -1. Moller-Trumbore per triangle.
static inline float WgRayTri(WgVec3 eye, WgVec3 dir, WgVec3 a, WgVec3 b, WgVec3 c) {
    const float kEps = 1e-6f;
    WgVec3 e1 = WgSub(b, a), e2 = WgSub(c, a);
    WgVec3 p = WgCross(dir, e2);
    float det = WgDot(e1, p);
    if (det > -kEps && det < kEps) return -1.0f;
    float inv = 1.0f / det;
    WgVec3 t = WgSub(eye, a);
    float u = WgDot(t, p) * inv;
    if (u < -kEps || u > 1.0f + kEps) return -1.0f;
    WgVec3 q = WgCross(t, e1);
    float v = WgDot(dir, q) * inv;
    if (v < -kEps || u + v > 1.0f + kEps) return -1.0f;
    float dist = WgDot(e2, q) * inv;
    return dist > kEps ? dist : -1.0f;
}

static inline float WgRayQuad(WgVec3 eye, WgVec3 dir, const WgQuad *q) {
    float d1 = WgRayTri(eye, dir, q->v[0], q->v[1], q->v[2]);
    float d2 = WgRayTri(eye, dir, q->v[0], q->v[2], q->v[3]);
    if (d1 < 0.0f) return d2;
    if (d2 < 0.0f) return d1;
    return d1 < d2 ? d1 : d2;
}

// The window the cockpit eye looks through: nearest quad hit along dir.
// -1 when nothing is hit (feature must stay off; never guess).
static inline int WindshieldGlass_Classify(const WgQuad quads[WG_WINDOWS],
                                           WgVec3 eye, WgVec3 dir) {
    int best = -1;
    float bestDist = 0.0f;
    for (int w = 0; w < WG_WINDOWS; w++) {
        float d = WgRayQuad(eye, dir, &quads[w]);
        if (d > 0.0f && (best < 0 || d < bestDist)) { best = w; bestDist = d; }
    }
    return best;
}

#endif

// ---- stable classification anchors -----------------------------------------
// The windshield is semantically the glass the wipers wipe: classify by the
// window quad nearest the wiper pivots (rest transforms, so head-look and
// replay cameras cannot change the answer). Falls back to the eye ray only
// when no pivots are available; a miss still returns -1.

static inline WgVec3 WgQuadCentre(const WgQuad *q) {
    WgVec3 c = {0.0f, 0.0f, 0.0f};
    for (int j = 0; j < 4; j++) { c.x += q->v[j].x; c.y += q->v[j].y; c.z += q->v[j].z; }
    c.x *= 0.25f; c.y *= 0.25f; c.z *= 0.25f;
    return c;
}

static inline float WgDistSqPointQuad(WgVec3 p, const WgQuad *q) {
    // Distance to the quad plane, plus a pull toward the centre so a point
    // beside a coplanar pair still picks its own quad.
    WgVec3 e1 = WgSub(q->v[1], q->v[0]), e2 = WgSub(q->v[3], q->v[0]);
    WgVec3 n = WgCross(e1, e2);
    float nl = WgDot(n, n);
    if (nl < 1e-6f) return 1e30f;
    WgVec3 d = WgSub(p, q->v[0]);
    float plane = WgDot(d, n) / nl;            // |n| not normalised: plane*nl=dot
    float planeDist2 = plane * plane * nl;     // (dot/|n|)^2
    WgVec3 c = WgQuadCentre(q);
    WgVec3 dc = WgSub(p, c);
    float centreDist2 = WgDot(dc, dc);
    return planeDist2 + 0.05f * centreDist2;   // plane dominates, centre breaks ties
}

static inline int WindshieldGlass_ClassifyByPivots(const WgQuad quads[WG_WINDOWS],
                                                   const WgVec3 *pivots, int pivotCount) {
    if (pivotCount <= 0) return -1;
    int best = -1;
    float bestDist = 0.0f;
    for (int w = 0; w < WG_WINDOWS; w++) {
        for (int p = 0; p < pivotCount; p++) {
            float d = WgDistSqPointQuad(pivots[p], &quads[w]);
            if (best < 0 || d < bestDist) { best = w; bestDist = d; }
        }
    }
    return best;
}

// Window index -> damage break slot, mirroring Car_QueueWindowBreak's
// breakPartIndex table: slot 0 breaks window 0, slot 1 window 1, slot 2
// windows 2+3, slot 3 windows 4+5. -1 for out-of-range input.
static inline int WindshieldGlass_SlotForWindow(int window) {
    if (window < 0 || window >= WG_WINDOWS) return -1;
    return window <= 1 ? window : window <= 3 ? 2 : 3;
}

// Damage break slot -> exterior triangle group mask, mirroring the
// StageObject_SetModelSubmeshVisibility switch (slots 0-3 on the
// node-type-7 mesh). 0 for out-of-range input (matches nothing).
static inline unsigned WindshieldGlass_GroupForSlot(int slot) {
    switch (slot) {
    case 0: return 0x100u;
    case 1: return 0x20u;
    case 2: return 0x40u;
    case 3: return 0x80u;
    }
    return 0;
}
