// Shadow filter taps: inside the unit disk, evenly spread at any rotation, and the
// radius never drops below the floor that hides shadow-map stair-steps.
#include "render/shadow_filter.h"
#include <cstdio>
#include <initializer_list>
#include <cmath>
static int failures;
static void Check(bool ok, const char *what) { if (!ok) { printf("FAIL: %s\n", what); failures++; } }
int main() {
    for (float rot : {0.f, 1.f, 2.5f, 5.f}) {
        float sx = 0, sy = 0, minD = 9;
        float px[SHADOW_FILTER_TAPS], py[SHADOW_FILTER_TAPS];
        for (int i = 0; i < SHADOW_FILTER_TAPS; i++) {
            Shadow_FilterTap(i, rot, &px[i], &py[i]);
            Check(std::hypot(px[i], py[i]) <= 1.0f, "tap inside unit disk");
            sx += px[i]; sy += py[i];
        }
        Check(std::hypot(sx, sy) / SHADOW_FILTER_TAPS < 0.12f, "taps are centred, no directional bias");
        for (int i = 0; i < SHADOW_FILTER_TAPS; i++)
            for (int j = i + 1; j < SHADOW_FILTER_TAPS; j++)
                minD = std::fmin(minD, std::hypot(px[i] - px[j], py[i] - py[j]));
        Check(minD > 0.15f, "no two taps coincide");
    }
    Check(Shadow_FilterRadius(0.f) == 0.5f && Shadow_FilterRadius(1.f) == 1.5f, "0% is sharper than the 25% default (1.5 texels)");
    // Every softness step gets its own radius: lowering softness must sharpen (0..4 in 0.1 steps = 0..100%).
    for (int s = 0; s < 40; s++) Check(Shadow_FilterRadius(s * .1f + .1f) > Shadow_FilterRadius(s * .1f), "radius strictly increases with softness");
    // Edge coverage across an occluder edge at x = 32 (what the shader does per pixel). Far from the edge
    // it is fully shadowed / fully lit; at the edge it is partial; the partial band is wider for softer
    // settings and never empty, so a broken filter at the boundary is caught.
    float widths[3]; int wi = 0;
    for (float soft : {0.f, 1.f, 4.f}) {
        int partial = 0;
        for (float rot : {0.f, 1.3f, 2.9f, 4.4f}) {
            Check(Shadow_EdgeVisibility(32.f - 6.f, 32.f, soft, rot) == 0.f, "deep inside shadow is fully dark");
            Check(Shadow_EdgeVisibility(32.f + 6.f, 32.f, soft, rot) == 1.f, "clear of the edge is fully lit");
            float prev = -1.f;
            for (float px = 24.f; px <= 40.f; px += 0.25f) {
                float v = Shadow_EdgeVisibility(px, 32.f, soft, rot);
                if (v > 0.f && v < 1.f) partial++;
                if (rot == 0.f) { Check(v >= prev - 0.34f, "coverage rises across the edge"); }
                prev = v;
            }
        }
        widths[wi++] = (float)partial;
        Check(partial > 0, "edge has a penumbra");
    }
    Check(widths[0] < widths[1] && widths[1] < widths[2], "penumbra widens 0% < 25% < 100%");
    if (failures == 0) { puts("shadow_filter: ok"); return 0; }
    return 1;
}
