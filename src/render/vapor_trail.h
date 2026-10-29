#ifndef OPENCMR2_VAPOR_TRAIL_H
#define OPENCMR2_VAPOR_TRAIL_H
// Vapor trails (visual only, stylised): thin streaks shed from the rear corners of the followed car. Four anchors on the car's own vertices
// (rear half, shoulder height and roof height, left and right of its direction of travel) are recorded in WORLD space a few times a second;
// the recorded points stay where they were while the car moves on, so they form lines behind the car that follow its real path through turns.
// Each frame the recorded points are projected with the view's own matrices and the shader draws soft lines through them. No airflow simulation,
// no effect on the car, its physics or the camera. Row-vector matrices (D3D convention). World y is up (checked against the stage capture: the car's y is its height).
#include <math.h>
static const int kVaporAnchors = 4;
static const int kVaporDepth = 24;                      // history samples per anchor
static const float kVaporSampleSeconds = 1.0f / 30.0f;  // one sample every 1/30 s: 24 samples = 0.8 s of trail
static const int kVaporPoints = 7;                      // points sent to the shader per anchor (the current anchor + 6 decimated history samples)
static const int kVaporPickIndex[kVaporPoints] = { 0, 1, 3, 6, 10, 16, 23 };   // history index (0 = newest) of each sent point
struct VaporBounds {
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
    int count = 0;
    void Add(const float p[3]) { for (int i = 0; i < 3; ++i) { if (p[i] < lo[i]) lo[i] = p[i]; if (p[i] > hi[i]) hi[i] = p[i]; } ++count; }
    bool Valid() const { return count > 0 && hi[1] > lo[1]; }
    void Centre(float c[3]) const { for (int i = 0; i < 3; ++i) c[i] = 0.5f * (lo[i] + hi[i]); }
};
// World position of model-space point p under a row-major world matrix.
inline void Vapor_ToWorld(const float *world, const float p[3], float out[3])
{
    for (int c = 0; c < 3; ++c) out[c] = p[0] * world[c] + p[1] * world[4 + c] + p[2] * world[8 + c] + world[12 + c];
}
// Horizontal unit direction of travel from two car centres; false when the car barely moved (the caller keeps its previous direction).
inline bool Vapor_Direction(const float from[3], const float to[3], float minMove, float d[3])
{
    float x = to[0] - from[0], z = to[2] - from[2], len = sqrtf(x * x + z * z);
    if (!(len > minMove)) return false;
    d[0] = x / len; d[1] = 0.0f; d[2] = z / len;
    return true;
}
struct VaporAnchors {
    float p[kVaporAnchors][3] = {};
    float best[kVaporAnchors] = { -1e30f, -1e30f, -1e30f, -1e30f };
    bool ok[kVaporAnchors] = {};
};
// Considers one world-space car vertex. d = horizontal direction of travel (unit), the lateral axis is up x d. Only the rear half counts.
// Slots: 0 left shoulder, 1 right shoulder (height >= 35% of the car), 2 left roof, 3 right roof (height >= 80%). The extreme lateral vertex wins.
inline void Vapor_ConsiderAnchor(VaporAnchors &a, const float p[3], const float centre[3], const float d[3], const VaporBounds &b)
{
    float height = b.hi[1] - b.lo[1];
    if (!(height > 0.0f)) return;
    float h = (p[1] - b.lo[1]) / height;
    float rx = p[0] - centre[0], rz = p[2] - centre[2];
    if (rx * d[0] + rz * d[2] > 0.0f) return;                 // front half: skip
    float lat = rx * d[2] - rz * d[0];                          // dot(p - centre, up x d), up = (0,1,0): positive on one side, negative on the other
    for (int s = 0; s < kVaporAnchors; ++s) {
        float minH = s < 2 ? 0.35f : 0.80f;
        if (h < minH) continue;
        float score = (s & 1) ? -lat : lat;
        if (score > a.best[s]) { a.best[s] = score; a.p[s][0] = p[0]; a.p[s][1] = p[1]; a.p[s][2] = p[2]; a.ok[s] = true; }
    }
}
// Ring of world-space history per anchor.
struct VaporHistory {
    float pts[kVaporDepth][kVaporAnchors][3] = {};
    bool ok[kVaporDepth][kVaporAnchors] = {};
    int head = 0, count = 0;   // head = index of the newest sample
    float sinceSample = 0.0f;
    void Reset() { head = 0; count = 0; sinceSample = 0.0f; for (int i = 0; i < kVaporDepth; ++i) for (int k = 0; k < kVaporAnchors; ++k) ok[i][k] = false; }
    void Push(const VaporAnchors &a)
    {
        head = (head + 1) % kVaporDepth;
        for (int k = 0; k < kVaporAnchors; ++k) { ok[head][k] = a.ok[k]; for (int c = 0; c < 3; ++c) pts[head][k][c] = a.p[k][c]; }
        if (count < kVaporDepth) ++count;
    }
    // Advances the clock by dt seconds (a gap above maxGap resets the history: pause, hitch, camera restart). Pushes a sample once per kVaporSampleSeconds.
    void Step(float dt, float maxGap, const VaporAnchors &a)
    {
        if (!(dt >= 0.0f) || dt > maxGap) Reset(); else sinceSample += dt;
        if (count == 0 || sinceSample >= kVaporSampleSeconds) { Push(a); sinceSample = 0.0f; }
    }
    // Sample `age` steps old (0 = newest); false when it does not exist yet or that anchor was not found.
    bool Get(int age, int anchor, float out[3]) const
    {
        if (age < 0 || age >= count) return false;
        int i = ((head - age) % kVaporDepth + kVaporDepth) % kVaporDepth;
        if (!ok[i][anchor]) return false;
        for (int c = 0; c < 3; ++c) out[c] = pts[i][anchor][c];
        return true;
    }
};
// Projects a world point; false when it is at or behind the near plane. u,v = 0..1 screen (v downwards), z = view depth, scaleV = screen heights per world unit at this depth.
inline bool Vapor_Project(const float *view, const float *proj, const float p[3], float vpX, float vpY, float vpW, float vpH, float targetW, float targetH,
                          float *u, float *v, float *z, float *scaleV)
{
    float e[4];
    for (int c = 0; c < 4; ++c) e[c] = p[0] * view[c] + p[1] * view[4 + c] + p[2] * view[8 + c] + view[12 + c];
    float cx = e[0] * proj[0] + e[1] * proj[4] + e[2] * proj[8] + e[3] * proj[12];
    float cy = e[0] * proj[1] + e[1] * proj[5] + e[2] * proj[9] + e[3] * proj[13];
    float cw = e[0] * proj[3] + e[1] * proj[7] + e[2] * proj[11] + e[3] * proj[15];
    if (!(cw > 1e-3f) || !(targetW > 0.0f) || !(targetH > 0.0f)) return false;
    *u = (vpX + (cx / cw * 0.5f + 0.5f) * vpW) / targetW;
    *v = (vpY + (0.5f - cy / cw * 0.5f) * vpH) / targetH;
    *z = e[2];
    *scaleV = proj[5] * 0.5f / cw * (vpH / targetH);
    return true;
}
// Width and opacity of a trail point at history index `age` (0..kVaporDepth-1) for strength 0..1: thin and bright at the car, wider and fainter with age.
inline float Vapor_HalfWidth(float carHeight, float scaleV, int age) { return 0.012f * carHeight * scaleV * (1.0f + 1.2f * (float)age / (float)(kVaporDepth - 1)); }
inline float Vapor_Alpha(float strength, int age)
{
    float t = (float)age / (float)(kVaporDepth - 1), f = 1.0f - t;
    strength = strength < 0.0f ? 0.0f : strength > 1.0f ? 1.0f : strength;
    return strength * f * f * 0.9f;
}
#endif
