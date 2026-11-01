// Unit checks for the touch pad overlay's generated sprites: the ring is a
// soft band at the rim with a transparent center and corners, the dot is a
// soft-edged filled disc. Shapes only; recording is exercised on device.
#include "touch_overlay.h"

#include <cstdio>
#include <cstdlib>

static int failures = 0;
#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                                \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

static unsigned char AlphaAt(const unsigned char *buf, int size, float nx, float ny)
{
    int x = (int)((nx + 1.0f) * 0.5f * size);
    int y = (int)((ny + 1.0f) * 0.5f * size);
    if (x >= size) x = size - 1;
    if (y >= size) y = size - 1;
    return buf[(y * size + x) * 4 + 3];
}

int main()
{
    const int size = 128;
    unsigned char *buf = new unsigned char[size * size * 4];

    TouchOverlay_FillRingBGRA(buf, size);
    CHECK(AlphaAt(buf, size, 0.0f, 0.0f) == 0);        // center transparent
    CHECK(AlphaAt(buf, size, 0.99f, 0.99f) == 0);      // corner transparent (r > 1)
    CHECK(AlphaAt(buf, size, 0.82f, 0.0f) > 200);      // rim band opaque
    CHECK(AlphaAt(buf, size, 0.0f, 0.82f) > 200);      // rim on the y axis too
    CHECK(AlphaAt(buf, size, 0.5f, 0.0f) < 60);        // between center and rim faint

    TouchOverlay_FillDotBGRA(buf, size);
    CHECK(AlphaAt(buf, size, 0.0f, 0.0f) == 255);      // center solid
    CHECK(AlphaAt(buf, size, 0.3f, 0.3f) == 255);      // inside the solid core
    CHECK(AlphaAt(buf, size, 0.99f, 0.0f) == 0);       // edge transparent
    CHECK(AlphaAt(buf, size, 0.99f, 0.99f) == 0);      // corner transparent

    // Presentation mapping: window pixels -> content space through the same
    // rect the executor's final blit uses, with the clamp bar policy.
    float m[4];
    // Stretched: window IS the content - identity mapping.
    TouchOverlay_MapPadToContent(500.0f, 250.0f, 100.0f, 100.0f, 1000, 500, 1000, 500, true, m);
    CHECK(m[0] == 500.0f && m[1] == 250.0f && m[2] == 100.0f && m[3] == 100.0f);
    // Letterboxed: 4:3 content (720x540) in a 2:1 window (2000x1000):
    // scale = min(2000/720, 1000/540) = 1.85, rect = (333,0,1333,1000).
    TouchOverlay_MapPadToContent(1000.0f, 500.0f, 100.0f, 100.0f, 2000, 1000, 720, 540, false, m);
    CHECK(m[0] > 359.0f && m[0] < 361.0f);   // window center -> content center
    CHECK(m[1] > 269.0f && m[1] < 271.0f);
    CHECK(m[2] > 53.0f && m[2] < 55.0f);     // radius scales with the picture (720/1333)
    // Bar policy: a finger in the left bar clamps the ring to the left edge,
    // fully on-picture (center clamped to the radius).
    TouchOverlay_MapPadToContent(0.0f, 500.0f, 100.0f, 100.0f, 2000, 1000, 720, 540, false, m);
    CHECK(m[0] > 53.0f && m[0] < 55.0f);     // == scaled radius: ring hugs the edge
    // Resize regression: same finger fraction after the window doubles moves
    // the mapped point proportionally, content size unchanged.
    TouchOverlay_MapPadToContent(2000.0f, 1000.0f, 100.0f, 100.0f, 4000, 2000, 720, 540, false, m);
    CHECK(m[0] > 359.0f && m[0] < 361.0f && m[1] > 269.0f && m[1] < 271.0f);
    // The clamp flag reports bar fingers so the "hide" policy can skip them.
    int cl = TouchOverlay_MapPadToContent(0.0f, 500.0f, 100.0f, 100.0f, 2000, 1000, 720, 540, false, m);
    CHECK(cl != 0);
    cl = TouchOverlay_MapPadToContent(1000.0f, 500.0f, 100.0f, 100.0f, 2000, 1000, 720, 540, false, m);
    CHECK(cl == 0);
    // Degenerate sizes yield zeros, never NaN.
    TouchOverlay_MapPadToContent(1.0f, 1.0f, 1.0f, 1.0f, 0, 0, 720, 540, false, m);
    CHECK(m[0] == 0.0f && m[2] == 0.0f);

    // BGR channels stay white; only alpha carries the shape.
    TouchOverlay_FillRingBGRA(buf, size);
    for (int i = 0; i < size * size; i += 257)
        CHECK(buf[i * 4 + 0] == 255 && buf[i * 4 + 1] == 255 && buf[i * 4 + 2] == 255);

    delete[] buf;
    std::printf("touch_overlay_texture ok\n");
    return 0;
}
