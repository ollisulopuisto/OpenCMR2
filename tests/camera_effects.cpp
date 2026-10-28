// OpenCMR2: camera effects — lens flare draw, dirt-on-lens accumulation and
// human-operator camera shake. Pure helpers are unit-tested here; the rest is
// wired into the chase-camera build and the present pass behind
// [video] video.camera_effects.
#include "GraphicsOptions.h"
#include "FixedPoint.h"
#include <cstdio>

static int failures;
static void Check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

int main(void)
{
    // Dirt level clamps to [0, 255], accumulates on impacts, decays per frame.
    Check(LensDirt_Accumulate(0, 100) == 100, "dirt accumulates from 0");
    Check(LensDirt_Accumulate(200, 100) == 255, "dirt clamps at 255");
    Check(LensDirt_Decay(255, 10) == 245, "dirt decays per frame");
    Check(LensDirt_Decay(5, 10) == 0, "dirt decays to zero");
    Check(LensDirt_ShouldDraw(0) == false, "no dirt is invisible");
    Check(LensDirt_ShouldDraw(1) == true, "any dirt is visible");

    // Shake amplitude from speed and impact level: zero at rest, grows with
    // both, clamped to a sane max.
    Check(Shake_Amplitude(0, 0) == 0, "no shake at rest");
    Check(Shake_Amplitude(0x10000, 0) > 0, "speed alone shakes");
    Check(Shake_Amplitude(0, 100) > 0, "impact alone shakes");
    Check(Shake_Amplitude(0x10000, 100) <= SHAKE_MAX_AMPLITUDE, "shake is clamped");
    Check(Shake_Amplitude(0x10000, 100) > Shake_Amplitude(0x10000, 10),
          "harder hit shakes more");

    // Lens flare brightness from sun visibility: off below threshold, grows
    // towards 100.
    Check(Flare_Brightness(0) == 0, "no sun, no flare");
    Check(Flare_Brightness(100) == FLARE_MAX_BRIGHTNESS, "full sun, full flare");
    Check(Flare_Brightness(50) == FLARE_MAX_BRIGHTNESS / 2, "half sun, half flare");

    if (failures == 0) {
        puts("camera effects: dirt, shake and flare helpers hold");
        return 0;
    }
    return 1;
}
