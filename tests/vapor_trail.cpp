#include "render/vapor_trail.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
static void box(VaporBounds &b, float cx, float cz) { for (int i = 0; i < 8; ++i) { float p[3] = { cx + ((i & 1) ? 2.2f : -2.2f), (i & 2) ? 1.4f : 0.0f, cz + ((i & 4) ? 0.9f : -0.9f) }; b.Add(p); } }
int main()
{
    // Direction: horizontal, unit length, refused when the car barely moved, y ignored.
    float a[3] = { 0, 5, 0 }, b[3] = { 3, 9, 4 }, d[3];
    CHECK(Vapor_Direction(a, b, 0.1f, d) && fabsf(d[0] - 0.6f) < 1e-5f && fabsf(d[2] - 0.8f) < 1e-5f && d[1] == 0.0f);
    float c[3] = { 0.01f, 0, 0 }; CHECK(!Vapor_Direction(a, c, 0.1f, d));
    // Anchors on a box car (4.4 long on x, 1.4 high, 1.8 wide on z) moving along +x: only the REAL rear half counts, one anchor per side and band.
    VaporBounds vb; box(vb, 10, 0); float centre[3]; vb.Centre(centre);
    float dir[3] = { 1, 0, 0 };
    VaporAnchors an;
    for (int i = 0; i < 8; ++i) { float p[3] = { 10 + ((i & 1) ? 2.2f : -2.2f), (i & 2) ? 1.4f : 0.0f, (i & 4) ? 0.9f : -0.9f }; Vapor_ConsiderAnchor(an, p, centre, dir, vb); }
    for (int k = 0; k < 4; ++k) { CHECK(an.ok[k]); CHECK(an.p[k][0] < 10.0f); CHECK(an.p[k][1] > 0.4f); }   // rear half (x < centre), above the lowest 35%
    CHECK(an.p[0][2] * an.p[1][2] < 0 && an.p[2][2] * an.p[3][2] < 0);                            // opposite sides
    // The same car moving along -x puts the anchors at the other end: the axis comes from the motion, not from the model.
    VaporAnchors bn; float rdir[3] = { -1, 0, 0 };
    for (int i = 0; i < 8; ++i) { float p[3] = { 10 + ((i & 1) ? 2.2f : -2.2f), (i & 2) ? 1.4f : 0.0f, (i & 4) ? 0.9f : -0.9f }; Vapor_ConsiderAnchor(bn, p, centre, rdir, vb); }
    for (int k = 0; k < 4; ++k) CHECK(bn.ok[k] && bn.p[k][0] > 10.0f);
    // Low vertices (wheels, shadow) never become anchors.
    VaporAnchors low; float lp[3] = { 8.0f, 0.1f, 0.9f }; Vapor_ConsiderAnchor(low, lp, centre, dir, vb); CHECK(!low.ok[0] && !low.ok[1] && !low.ok[2] && !low.ok[3]);
    // History: samples every 1/30 s, newest first, ring wraps, a gap resets it.
    VaporHistory h; VaporAnchors s;
    for (int k = 0; k < 4; ++k) { s.ok[k] = true; }
    for (int step = 0; step < 60; ++step) { for (int k = 0; k < 4; ++k) { s.p[k][0] = (float)step; s.p[k][1] = 0; s.p[k][2] = (float)k; } h.Step(1.0f / 30.0f, 0.25f, s); }
    float g[3]; CHECK(h.count == kVaporDepth && h.Get(0, 2, g) && g[0] == 59.0f && g[2] == 2.0f);
    CHECK(h.Get(10, 0, g) && g[0] == 49.0f);
    CHECK(!h.Get(kVaporDepth, 0, g) && !h.Get(-1, 0, g));
    h.Step(0.5f, 0.25f, s); CHECK(h.count == 1);                                                   // hitch/pause: restart, no line across the gap
    h.Step(-1.0f, 0.25f, s); CHECK(h.count == 1);                                                  // bad dt: reset, then one sample
    // A frame faster than the sample interval does not push.
    VaporHistory h2; h2.Step(0.0f, 0.25f, s); h2.Step(0.005f, 0.25f, s); CHECK(h2.count == 1);
    VaporAnchors none; VaporHistory h3; h3.Step(0.0f, 0.25f, none); CHECK(!h3.Get(0, 0, g));        // anchor not found = no point
    // Projection: camera at origin looking down +z, point at z=10 half a unit right.
    float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 }, P[16] = { 2,0,0,0, 0,2,0,0, 0,0,1,1, 0,0,-0.1f,0 };
    float u, v, z, sv, p1[3] = { 0.5f, 0.0f, 10.0f }, pb[3] = { 0, 0, -3 };
    CHECK(Vapor_Project(I, P, p1, 0, 0, 1000, 500, 1000, 500, &u, &v, &z, &sv) && fabsf(u - (0.5f + 0.5f * 0.1f)) < 1e-4f && fabsf(v - 0.5f) < 1e-4f && fabsf(z - 10.0f) < 1e-5f);
    CHECK(fabsf(sv - 2.0f * 0.5f / 10.0f) < 1e-5f);                                                // 0.1 screen heights per world unit
    CHECK(!Vapor_Project(I, P, pb, 0, 0, 1000, 500, 1000, 500, &u, &v, &z, &sv));                  // behind the camera
    float p2[3] = { 0.0f, 1.0f, 10.0f }; Vapor_Project(I, P, p2, 0, 0, 1000, 500, 1000, 500, &u, &v, &z, &sv); CHECK(v < 0.5f);   // above the axis = smaller v (y down)
    // Width and opacity: thin and strong at the car, wider and fainter with age, zero strength = invisible, never negative.
    CHECK(Vapor_HalfWidth(1.4f, 0.1f, 0) < Vapor_HalfWidth(1.4f, 0.1f, kVaporDepth - 1) && Vapor_HalfWidth(1.4f, 0.1f, 0) > 0.0f);
    CHECK(Vapor_Alpha(1.0f, 0) > Vapor_Alpha(1.0f, 10) && Vapor_Alpha(1.0f, kVaporDepth - 1) == 0.0f && Vapor_Alpha(0.0f, 0) == 0.0f && Vapor_Alpha(5.0f, 0) <= 0.22f + 1e-6f && Vapor_Alpha(1.0f, 0) > 0.1f);
    // Picks stay inside the ring and increase.
    for (int j = 0; j < kVaporPoints; ++j) { CHECK(kVaporPickIndex[j] >= 0 && kVaporPickIndex[j] < kVaporDepth); if (j) CHECK(kVaporPickIndex[j] > kVaporPickIndex[j - 1]); }
    if (!fails) puts("vapor_trail ok");
    return fails;
}
