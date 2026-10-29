#include "FovDolly.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
int main()
{
    // The saved view and a narrower view never move the camera.
    CHECK(FovDolly_Widening(65536, 80, 80) == 65536);
    CHECK(FovDolly_Widening(65536, 80, 60) == 65536);
    CHECK(FovDolly_Scale(65536, 100) == 65536);
    // Preview 150 over a saved 80: things look 150/80 = 1.875x smaller, so full compensation puts the camera at 1/1.875 = 0.533.
    int w = FovDolly_Widening(65536, 80, 150);
    CHECK(w == (int)(65536LL * 150 / 80));
    int s = FovDolly_Scale(w, 100);
    CHECK(s > 34900 && s < 35100);                      // 0.533 * 65536 = 34953
    // The car's screen size is proportional to (1 / widening) / distance: with full dolly it is unchanged.
    CHECK((long long)s * w / 65536 > 65536 - 40 && (long long)s * w / 65536 < 65536 + 40);
    // Speed FOV widening multiplies with the preview; amount 50 is half way; amount 0 is off.
    CHECK(FovDolly_Widening(65536 + 65536 / 4, 70, 70) == 65536 + 65536 / 4);
    int half = FovDolly_Scale(w, 50);
    CHECK(half > s && half < 65536 && half - s > (65536 - s) / 2 - 2 && half - s < (65536 - s) / 2 + 2);
    CHECK(FovDolly_Scale(w, 0) == 65536);
    CHECK(FovDolly_Scale(w, -20) == 65536 && FovDolly_Scale(w, 400) == s);
    // The camera never gets closer than the floor, however wide the view.
    CHECK(FovDolly_Scale(65536 * 20, 100) == kFovDollyMinScale);
    CHECK(kFovDollyMinScale > 65536 * 4 / 10 && kFovDollyMinScale < 65536 * 5 / 10);
    // Bad inputs fall back to no change.
    CHECK(FovDolly_Widening(0, 80, 80) == 65536 && FovDolly_Widening(65536, 0, 80) == 65536 && FovDolly_Widening(65536, 80, 0) == 65536);
    // Monotone: a wider view never moves the camera farther.
    int prev = 65536;
    for (int e = 80; e <= 150; e += 5) { int sc = FovDolly_Scale(FovDolly_Widening(65536, 80, e), 100); CHECK(sc <= prev); prev = sc; }
    if (!fails) puts("fov_dolly ok");
    return fails;
}
