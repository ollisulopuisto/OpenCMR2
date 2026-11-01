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

    // BGR channels stay white; only alpha carries the shape.
    TouchOverlay_FillRingBGRA(buf, size);
    for (int i = 0; i < size * size; i += 257)
        CHECK(buf[i * 4 + 0] == 255 && buf[i * 4 + 1] == 255 && buf[i * 4 + 2] == 255);

    delete[] buf;
    std::printf("touch_overlay_texture ok\n");
    return 0;
}
