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
    Check(Shadow_FilterRadius(0.f) == SHADOW_FILTER_MIN_RADIUS, "zero softness still filters");
    Check(Shadow_FilterRadius(3.f) == 3.f, "larger softness kept");
    if (failures == 0) { puts("shadow_filter: ok"); return 0; }
    return 1;
}
