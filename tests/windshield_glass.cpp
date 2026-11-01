// Runtime windshield classifier (game/WindshieldGlass.h): descriptor
// mirroring, ray classification front vs rear, safe no-hit behaviour.
#include "WindshieldGlass.h"
#include <cstdio>
#include <cmath>

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
static int Approx(float a, float b) { return std::fabs(a - b) < 1e-3f; }

// Synthetic cabin, forward +X, lateral Z, up Y: front windshield plane x=80,
// rear window plane x=-60, side panels z=+/-30, glass band y=30..60.
static const int kVerts[WG_WINDOWS][3] = {
    {80, 60, 30},   // 0: front top right
    {80, 30, 30},   // 1: front bottom right
    {-60, 60, 30},  // 2: rear top right
    {-60, 30, 30},  // 3: rear bottom right
    {0, 0, 0},      // 4: unused
    {0, 0, 0},      // 5: unused
};
static const unsigned char kDesc[WG_WINDOWS][4] = {
    {0, 1, 1, 0},   // 0: front windshield (real descriptor layout)
    {2, 3, 3, 2},   // 1: rear window, second pair mirrored
    {0, 2, 3, 1},   // 2: right side panel
    {0, 2, 3, 1},   // 3: left side panel (mirrored)
    {2, 3, 3, 2},   // 4: rear right quarter (unmirrored)
    {2, 3, 3, 2},   // 5: rear left quarter (mirrored)
};
static const int kMirror[WG_WINDOWS][4] = {
    {0, 0, 1, 1}, {0, 0, 1, 1}, {0, 0, 0, 0},
    {1, 1, 1, 1}, {0, 0, 0, 0}, {1, 1, 1, 1},
};

static void test_mirror(void) {
    WgQuad q[WG_WINDOWS];
    WindshieldGlass_BuildQuads(kVerts, kDesc, kMirror, q);
    CHECK(Approx(q[0].v[0].z, 30.0f));
    CHECK(Approx(q[0].v[2].z, -30.0f));  // mirrored copy of vert 1
    CHECK(Approx(q[0].v[3].z, -30.0f));  // mirrored copy of vert 0
    CHECK(Approx(q[2].v[1].z, 30.0f));   // unmirrored side panel
    CHECK(Approx(q[3].v[1].z, -30.0f));  // mirrored left panel
}

static void test_front(void) {
    WgQuad q[WG_WINDOWS];
    WindshieldGlass_BuildQuads(kVerts, kDesc, kMirror, q);
    WgVec3 eye{0.0f, 45.0f, 0.0f};
    WgVec3 fwd{1.0f, 0.0f, 0.0f};
    CHECK(WindshieldGlass_Classify(q, eye, fwd) == 0);
}

static void test_rear(void) {
    WgQuad q[WG_WINDOWS];
    WindshieldGlass_BuildQuads(kVerts, kDesc, kMirror, q);
    WgVec3 eye{0.0f, 45.0f, 0.0f};
    WgVec3 back{-1.0f, 0.0f, 0.0f};
    int hit = WindshieldGlass_Classify(q, eye, back);
    CHECK(hit == 1 || hit == 4);   // rear quads, never the front windshield
    CHECK(hit != 0);
}

static void test_roof(void) {
    WgQuad q[WG_WINDOWS];
    WindshieldGlass_BuildQuads(kVerts, kDesc, kMirror, q);
    WgVec3 eye{0.0f, 45.0f, 0.0f};
    WgVec3 up{0.0f, 1.0f, 0.0f};
    CHECK(WindshieldGlass_Classify(q, eye, up) == -1);   // safe off
}

static void test_edge(void) {
    WgQuad q[WG_WINDOWS];
    WindshieldGlass_BuildQuads(kVerts, kDesc, kMirror, q);
    WgVec3 eye{0.0f, 45.0f, 0.0f};
    WgVec3 sideways{0.0f, 0.0f, 1.0f};   // parallel to front/rear glass
    int hit = WindshieldGlass_Classify(q, eye, sideways);
    CHECK(hit != 0);   // may hit a side panel, never the front windshield
}

int main()
{
    test_mirror();
    test_front();
    test_rear();
    test_roof();
    test_edge();
    if (fails) { std::printf("%d failures\n", fails); return 1; }
    std::printf("windshield_glass ok\n");
    return 0;
}
