// OpenCMR2: motion blur — [video] video.motion_blur strength (0 off to
// MOTION_BLUR_MAX) mapped to a previous-frame blend factor. Pure helper,
// unit-tested here; the post-process blend is wired into the present pass.
#include "GraphicsOptions.h"
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
    // Off at zero and below, full at the max and above.
    Check(MotionBlur_Mix(0) == 0, "strength 0 is no blur");
    Check(MotionBlur_Mix(-5) == 0, "negative is no blur");
    Check(MotionBlur_Mix(MOTION_BLUR_MAX) == 255, "max strength is full mix");
    Check(MotionBlur_Mix(MOTION_BLUR_MAX + 10) == 255, "above max clamps");

    // Linear in between: half strength is about half mix.
    Check(MotionBlur_Mix(50) == (50 * 255) / MOTION_BLUR_MAX, "half strength");
    Check(MotionBlur_Mix(25) == (25 * 255) / MOTION_BLUR_MAX, "quarter strength");
    Check(MotionBlur_Mix(75) == (75 * 255) / MOTION_BLUR_MAX, "three-quarter strength");

    // Monotonic: more strength never gives less mix.
    int prev = 0;
    for (int s = 0; s <= MOTION_BLUR_MAX; s += 7) {
        int m = MotionBlur_Mix(s);
        Check(m >= prev, "mix is monotonic in strength");
        prev = m;
    }

    if (failures == 0) {
        puts("motion blur: strength-to-mix mapping holds");
        return 0;
    }
    return 1;
}
