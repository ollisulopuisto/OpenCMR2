#include "render/car_region.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
static void ident(float *m) { for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f; }
// Perspective like the game's: row vectors, w = z (view looks down +z).  x' = x*sx, y' = y*sy.
static void proj(float *m, float sx, float sy) { for (int i = 0; i < 16; ++i) m[i] = 0; m[0] = sx; m[5] = sy; m[10] = 1.0f; m[11] = 1.0f; m[14] = -0.1f; }
int main()
{
    float w[16], v[16], p[16]; ident(w); ident(v); proj(p, 1.0f, 1.0f);
    CarRegion r; CHECK(!r.Valid());
    // A box 2 wide, 1 tall, centred at x=0 y=-0.5, depths 4..6 on a 1000x500 target, full viewport.
    float pts[8][3] = { {-1,-1,4}, {1,-1,4}, {-1,0,4}, {1,0,4}, {-1,-1,6}, {1,-1,6}, {-1,0,6}, {1,0,6} };
    for (auto &q : pts) CarRegion_Add(r, w, v, p, q, 0, 0, 1000, 500, 1000, 500);
    CHECK(r.Valid() && r.count == 8);
    // Nearest edge: x -1/4 = -0.25 -> u = 0.375; x 1/4 -> 0.625. Hmm widest on screen is the near face.
    CHECK(fabsf(r.u0 - 0.375f) < 1e-4f && fabsf(r.u1 - 0.625f) < 1e-4f);
    // y: near face -1/4 .. 0 -> ndc -0.25..0 -> v = 0.5 - ndc*0.5 = 0.625 .. 0.5 (v goes down, so the lower part of the car has larger v)
    CHECK(fabsf(r.v0 - 0.5f) < 1e-4f && fabsf(r.v1 - 0.625f) < 1e-4f);
    CHECK(fabsf(r.zMin - 4.0f) < 1e-5f && fabsf(r.zMax - 6.0f) < 1e-5f);
    // A half-width viewport on the left half of the target halves u.
    CarRegion h; for (auto &q : pts) CarRegion_Add(h, w, v, p, q, 0, 0, 500, 500, 1000, 500);
    CHECK(fabsf(h.u0 - 0.1875f) < 1e-4f);
    // Vertices behind the camera are ignored, not mirrored onto the screen.
    CarRegion b; float behind[3] = {0, 0, -3}; CarRegion_Add(b, w, v, p, behind, 0, 0, 1000, 500, 1000, 500); CHECK(b.count == 0 && !b.Valid());
    // Bad target size is ignored.
    CarRegion z; CarRegion_Add(z, w, v, p, pts[0], 0, 0, 1000, 500, 0, 0); CHECK(z.count == 0);
    // World matrix translation moves the region.
    float w2[16]; ident(w2); w2[12] = 1.0f; CarRegion t; for (auto &q : pts) CarRegion_Add(t, w2, v, p, q, 0, 0, 1000, 500, 1000, 500);
    CHECK(t.u0 > r.u0 + 0.1f);
    // Mask: inside the rectangle and depth range = 1; behind the car (deeper) or outside the rectangle = 0; feather is smooth.
    CHECK(CarRegion_Mask(r, 0.5f, 0.55f, 0.50f, 0.45f, 0.55f, 0.02f, 0.012f) > 0.99f);
    CHECK(CarRegion_Mask(r, 0.5f, 0.55f, 0.80f, 0.45f, 0.55f, 0.02f, 0.012f) < 0.01f);   // background behind the car
    CHECK(CarRegion_Mask(r, 0.5f, 0.55f, 0.10f, 0.45f, 0.55f, 0.02f, 0.012f) < 0.01f);   // something well in front of the car
    CHECK(CarRegion_Mask(r, 0.9f, 0.55f, 0.50f, 0.45f, 0.55f, 0.02f, 0.012f) < 0.01f);   // outside the rectangle
    float m = CarRegion_Mask(r, r.u1 + 0.01f, 0.55f, 0.50f, 0.45f, 0.55f, 0.02f, 0.012f); CHECK(m > 0.05f && m < 0.95f);
    CHECK(CarRegion_Mask(CarRegion(), 0.5f, 0.5f, 0.5f, 0.45f, 0.55f, 0.02f, 0.012f) == 0.0f);   // no region = nothing excluded
    // The fast per-draw path matches the single-vertex path on a non-trivial transform (rotated world, offset camera, off-centre viewport).
    {
        float W[16], V[16], P[16]; ident(W); ident(V); proj(P, 1.3f, 1.9f);
        W[0] = 0.8f; W[1] = 0.6f; W[4] = -0.6f; W[5] = 0.8f; W[12] = 0.7f; W[13] = -0.4f; W[14] = 3.0f; V[12] = -0.2f; V[13] = 0.3f; V[14] = 1.5f;
        struct Vx { float x, y, z, rest[9]; } vx[6] = { {-1,-1,1}, {1,-1,2}, {-1,1,3}, {1,1,1.5f}, {0.3f,0.2f,-9.0f}, {0.5f,-0.5f,0.5f} };   // one vertex is behind the camera
        CarRegion a, b;
        for (auto &q : vx) { float pp[3] = { q.x, q.y, q.z }; CarRegion_Add(a, W, V, P, pp, 100, 50, 800, 400, 1000, 500); }
        CarRegion_AddDraw(b, W, V, P, (const unsigned char *)vx, sizeof(Vx), 6, 100, 50, 800, 400, 1000, 500);
        CHECK(a.count == b.count && a.count >= 4);
        CHECK(fabsf(a.u0 - b.u0) < 1e-4f && fabsf(a.u1 - b.u1) < 1e-4f && fabsf(a.v0 - b.v0) < 1e-4f && fabsf(a.v1 - b.v1) < 1e-4f && fabsf(a.zMin - b.zMin) < 1e-4f && fabsf(a.zMax - b.zMax) < 1e-4f);
        CarRegion c; CarRegion_AddDraw(c, W, V, P, (const unsigned char *)vx, sizeof(Vx), 6, 0, 0, 1000, 500, 0, 0); CHECK(c.count == 0);
        CarRegion e; CarRegion_AddDraw(e, W, V, P, (const unsigned char *)vx, sizeof(Vx), 0, 0, 0, 1000, 500, 1000, 500); CHECK(!e.Valid());
    }
    if (!fails) puts("car_region ok");
    return fails;
}
