#include "FovDolly.h"
#include "FovPreview.h"
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
    CHECK(s == kFovDollyMinScale);                      // 0.533 is closer than the floor, so the floor holds (0.35 clipped into the car natively)
    // Inside the floor the car's screen size is proportional to (1 / widening) / distance: full dolly keeps it unchanged.
    { int w2 = FovDolly_Widening(65536, 80, 100); int s2 = FovDolly_Scale(w2, 100); CHECK(s2 > 65536 * 79 / 100 && s2 < 65536 * 81 / 100); CHECK((long long)s2 * w2 / 65536 > 65536 - 40 && (long long)s2 * w2 / 65536 < 65536 + 40); }
    // Speed FOV widening multiplies with the preview; amount 50 is half way; amount 0 is off.
    CHECK(FovDolly_Widening(65536 + 65536 / 4, 70, 70) == 65536 + 65536 / 4);
    int half = FovDolly_Scale(w, 50);
    CHECK(half >= s && half < 65536);
    CHECK(FovDolly_Scale(w, 0) == 65536);
    CHECK(FovDolly_Scale(w, -20) == 65536 && FovDolly_Scale(w, 400) == FovDolly_Scale(w, kFovDollyMaxPct));
    // The camera never gets closer than the floor, however wide the view.
    CHECK(FovDolly_Scale(65536 * 20, 100) == kFovDollyMinScale);
    CHECK(kFovDollyMinScale > 65536 * 69 / 100 && kFovDollyMinScale < 65536 * 71 / 100);
    // Bad inputs fall back to no change.
    CHECK(FovDolly_Widening(0, 80, 80) == 65536 && FovDolly_Widening(65536, 0, 80) == 65536 && FovDolly_Widening(65536, 80, 0) == 65536);
    // Monotone: a wider view never moves the camera farther.
    int prev = 65536;
    for (int e = 80; e <= 150; e += 5) { int sc = FovDolly_Scale(FovDolly_Widening(65536, 80, e), 100); CHECK(sc <= prev); prev = sc; }
    // Preview easing: smooth, monotone, never overshoots, snaps on first frame / pause, reaches the target.
    CHECK(FovPreview_Ease(0, 150 * 65536, 16) == 150 * 65536);
    CHECK(FovPreview_Ease(80 * 65536, 150 * 65536, 300) == 150 * 65536 && FovPreview_Ease(80 * 65536, 150 * 65536, -1) == 150 * 65536);
    int sh = 80 * 65536, lastStep = 1 << 30;
    for (int i = 0; i < 400; ++i) {
        int n = FovPreview_Ease(sh, 150 * 65536, 16);
        CHECK(n >= sh && n <= 150 * 65536);
        if (i < 30) { CHECK(n - sh < 20 * 65536); CHECK(n - sh <= lastStep + 1); lastStep = n - sh; }   // no jump, steps shrink
        sh = n;
    }
    CHECK(sh > 149 * 65536);
    int dn = FovPreview_Ease(150 * 65536, 80 * 65536, 16); CHECK(dn < 150 * 65536 && dn > 80 * 65536);
    // Dolly follows the eased value, so the car size stays about constant through the whole transition.
    for (int e = 80 * 65536; e <= 150 * 65536; e += 5 * 65536) {
        int wd = FovDolly_Widening(65536, 80 * 65536, e), sc = FovDolly_Scale(wd, 100);
        long long size = (long long)sc * wd / 65536; if (sc > kFovDollyMinScale) CHECK(size > 65536 - 60 && size < 65536 + 60); else CHECK(size >= 65536);
    }
    if (!fails) puts("fov_dolly ok");
    return fails;
}
