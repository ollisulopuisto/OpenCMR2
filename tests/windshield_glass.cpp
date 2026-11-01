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

static void test_pivots_pick_front(void) {
    WgQuad q[WG_WINDOWS];
    WindshieldGlass_BuildQuads(kVerts, kDesc, kMirror, q);
    // Wiper pivots at the base of the front windshield, one per side.
    WgVec3 pivots[2] = {{78.0f, 30.0f, 20.0f}, {78.0f, 30.0f, -20.0f}};
    CHECK(WindshieldGlass_ClassifyByPivots(q, pivots, 2) == 0);
}

static void test_pivots_rear_pick_rear(void) {
    WgQuad q[WG_WINDOWS];
    WindshieldGlass_BuildQuads(kVerts, kDesc, kMirror, q);
    WgVec3 pivots[1] = {{-58.0f, 30.0f, 0.0f}};
    int hit = WindshieldGlass_ClassifyByPivots(q, pivots, 1);
    CHECK(hit == 1 || hit == 4 || hit == 5);   // rear glass, never front
    CHECK(hit != 0);
}

static void test_no_pivots_misses_safely(void) {
    WgQuad q[WG_WINDOWS];
    WindshieldGlass_BuildQuads(kVerts, kDesc, kMirror, q);
    CHECK(WindshieldGlass_ClassifyByPivots(q, 0, 0) == -1);
}

static void test_window_slot_mapping(void) {
    CHECK(WindshieldGlass_SlotForWindow(0) == 0);
    CHECK(WindshieldGlass_SlotForWindow(1) == 1);
    CHECK(WindshieldGlass_SlotForWindow(2) == 2);
    CHECK(WindshieldGlass_SlotForWindow(3) == 2);
    CHECK(WindshieldGlass_SlotForWindow(4) == 3);
    CHECK(WindshieldGlass_SlotForWindow(5) == 3);
    CHECK(WindshieldGlass_SlotForWindow(6) == -1);
    CHECK(WindshieldGlass_SlotForWindow(-1) == -1);
    CHECK(WindshieldGlass_GroupForSlot(0) == 0x100u);
    CHECK(WindshieldGlass_GroupForSlot(1) == 0x20u);
    CHECK(WindshieldGlass_GroupForSlot(2) == 0x40u);
    CHECK(WindshieldGlass_GroupForSlot(3) == 0x80u);
    CHECK(WindshieldGlass_GroupForSlot(4) == 0);
    CHECK(WindshieldGlass_GroupForSlot(-1) == 0);
}

int main()
{
    test_mirror();
    test_front();
    test_rear();
    test_roof();
    test_edge();
    test_pivots_pick_front();
    test_pivots_rear_pick_rear();
    test_no_pivots_misses_safely();
    test_window_slot_mapping();
    if (fails) { std::printf("%d failures\n", fails); return 1; }
    std::printf("windshield_glass ok\n");
    return 0;
}
