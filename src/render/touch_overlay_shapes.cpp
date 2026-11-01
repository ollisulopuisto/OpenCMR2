#include "touch_overlay.h"

#include <math.h>

// ---- generated shapes (pure, unit-tested) ---------------------------------

static unsigned char SmoothStep(float lo, float hi, float v)
{
    if (v <= lo)
        return 0;
    if (v >= hi)
        return 255;
    float t = (v - lo) / (hi - lo);
    t = t * t * (3.0f - 2.0f * t);
    return (unsigned char)(t * 255.0f + 0.5f);
}

void TouchOverlay_FillRingBGRA(unsigned char *out, int size)
{
    // Band centred at 82% of the half-size, feathered over 6% either side.
    const float rim = 0.82f, feather = 0.06f;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float nx = (x + 0.5f) / size * 2.0f - 1.0f;
            float ny = (y + 0.5f) / size * 2.0f - 1.0f;
            float r = sqrtf(nx * nx + ny * ny);
            float rise = SmoothStep(rim - feather, rim, r);
            float fall = 255 - SmoothStep(rim, rim + feather, r);
            float a = rise * fall / 255.0f; // product normalized: no u8 overflow at the peak
            unsigned char *px = out + (y * size + x) * 4;
            px[0] = px[1] = px[2] = 255;
            px[3] = (unsigned char)a;
        }
    }
}

void TouchOverlay_FillDotBGRA(unsigned char *out, int size)
{
    // Solid to 55% of the half-size, feathered to zero at 90%.
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float nx = (x + 0.5f) / size * 2.0f - 1.0f;
            float ny = (y + 0.5f) / size * 2.0f - 1.0f;
            float r = sqrtf(nx * nx + ny * ny);
            unsigned char a = 255 - SmoothStep(0.55f, 0.9f, r);
            unsigned char *px = out + (y * size + x) * 4;
            px[0] = px[1] = px[2] = 255;
            px[3] = a;
        }
    }
}

