#ifndef OPENCMR2_CAR_REGION_H
#define OPENCMR2_CAR_REGION_H
// Where the followed car really is on screen, from its own vertices (not its origin, not a depth plane): every vertex of the car's draws is
// projected with that draw's world, view and projection matrices (row vectors, D3D convention), giving a screen rectangle and the view-depth
// range of the car's geometry. Visual only. This is a bounding rectangle plus a depth interval, NOT a per-pixel silhouette mask:
// scenery inside the rectangle and inside the car's depth range (the road around the car) counts as car, and anything in front of the car
// that overlaps it is not detected. dof.frag uses it to keep the car sharp in the whole-scene blur mode.
#include <math.h>
struct CarRegion {
    float u0 = 1e30f, v0 = 1e30f, u1 = -1e30f, v1 = -1e30f;   // 0..1 screen rectangle, v downwards like gl_FragCoord on SDL_GPU
    float zMin = 1e30f, zMax = -1e30f;                         // view-space forward depth of the car's vertices
    int count = 0;
    bool Valid() const { return count > 0 && u1 > u0 && v1 > v0 && zMax > zMin; }
};
// Adds one vertex. world/view/proj are 16 floats, row major. vpX..vpH is the draw's viewport in pixels, targetW/H the render target.
// Vertices at or behind the near plane (w <= 0) are skipped: they have no screen position.
inline void CarRegion_Add(CarRegion &r, const float *world, const float *view, const float *proj, const float p[3],
                          float vpX, float vpY, float vpW, float vpH, float targetW, float targetH)
{
    float w4[4];
    for (int c = 0; c < 4; ++c) w4[c] = p[0] * world[c] + p[1] * world[4 + c] + p[2] * world[8 + c] + world[12 + c];
    float e[4];
    for (int c = 0; c < 4; ++c) e[c] = w4[0] * view[c] + w4[1] * view[4 + c] + w4[2] * view[8 + c] + w4[3] * view[12 + c];
    float cx = e[0] * proj[0] + e[1] * proj[4] + e[2] * proj[8] + e[3] * proj[12];
    float cy = e[0] * proj[1] + e[1] * proj[5] + e[2] * proj[9] + e[3] * proj[13];
    float cw = e[0] * proj[3] + e[1] * proj[7] + e[2] * proj[11] + e[3] * proj[15];
    if (!(cw > 1e-4f) || !(targetW > 0.0f) || !(targetH > 0.0f)) return;
    float nx = cx / cw, ny = cy / cw;
    float u = (vpX + (nx * 0.5f + 0.5f) * vpW) / targetW;
    float v = (vpY + (0.5f - ny * 0.5f) * vpH) / targetH;
    if (u < r.u0) r.u0 = u;
    if (u > r.u1) r.u1 = u;
    if (v < r.v0) r.v0 = v;
    if (v > r.v1) r.v1 = v;
    if (e[2] < r.zMin) r.zMin = e[2];
    if (e[2] > r.zMax) r.zMax = e[2];
    ++r.count;
}
// Same result as calling CarRegion_Add for every vertex, but the matrices are combined once per draw (clip = p * (world*view*proj), eye depth
// = p * (world*view) column 2), so each vertex costs 12 multiplies for xyw plus 3 for depth. `stride` is the vertex size in bytes; positions are the
// first three floats of each vertex (GfxVertex).
inline void CarRegion_AddDraw(CarRegion &r, const float *world, const float *view, const float *proj, const unsigned char *vertices, unsigned stride,
                              unsigned count, float vpX, float vpY, float vpW, float vpH, float targetW, float targetH)
{
    if (!(targetW > 0.0f) || !(targetH > 0.0f)) return;
    float wv[16], m[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) wv[i * 4 + j] = world[i * 4] * view[j] + world[i * 4 + 1] * view[4 + j] + world[i * 4 + 2] * view[8 + j] + world[i * 4 + 3] * view[12 + j];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) m[i * 4 + j] = wv[i * 4] * proj[j] + wv[i * 4 + 1] * proj[4 + j] + wv[i * 4 + 2] * proj[8 + j] + wv[i * 4 + 3] * proj[12 + j];
    const float ku = vpW * 0.5f / targetW, kv = vpH * 0.5f / targetH, ou = (vpX + vpW * 0.5f) / targetW, ov = (vpY + vpH * 0.5f) / targetH;
    for (unsigned k = 0; k < count; ++k) {
        const float *p = (const float *)(vertices + (size_t)k * stride);
        float cw = p[0] * m[3] + p[1] * m[7] + p[2] * m[11] + m[15];
        if (!(cw > 1e-4f)) continue;
        float inv = 1.0f / cw;
        float u = ou + (p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12]) * inv * ku;
        float v = ov - (p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13]) * inv * kv;
        float z = p[0] * wv[2] + p[1] * wv[6] + p[2] * wv[10] + wv[14];
        if (u < r.u0) r.u0 = u;
        if (u > r.u1) r.u1 = u;
        if (v < r.v0) r.v0 = v;
        if (v > r.v1) r.v1 = v;
        if (z < r.zMin) r.zMin = z;
        if (z > r.zMax) r.zMax = z;
        ++r.count;
    }
}
// Mirrors dof.frag's carMask(): 1 inside the car rectangle widened by `feather` and inside the car's depth range widened by `zPad` (both in
// the shader's log depth position), fading to 0 over the feather. Kept here so the maths is tested.
inline float CarRegion_Mask(const CarRegion &r, float u, float v, float depthPos, float posMin, float posMax, float feather, float zPad)
{
    if (!r.Valid()) return 0.0f;
    auto edge = [](float a, float b, float x) { float t = (x - a) / (b - a); t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t; return t * t * (3.0f - 2.0f * t); };
    float inU = edge(r.u0 - feather, r.u0, u) * (1.0f - edge(r.u1, r.u1 + feather, u));
    float inV = edge(r.v0 - feather, r.v0, v) * (1.0f - edge(r.v1, r.v1 + feather, v));
    float inZ = edge(posMin - 2.0f * zPad, posMin - zPad, depthPos) * (1.0f - edge(posMax + zPad, posMax + 2.0f * zPad, depthPos));
    return inU * inV * inZ;
}
#endif
