// OpenCMR2: bloom — [video] video.bloom intensity (0 off to BLOOM_MAX) mapped
// to a previous-frame blend factor. Pure helper, unit-tested here; the
// bright-pass/blur/composite post-process is wired into the present pass.
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
    Check(Bloom_Intensity(0) == 0, "strength 0 is no bloom");
    Check(Bloom_Intensity(-5) == 0, "negative is no bloom");
    Check(Bloom_Intensity(BLOOM_MAX) == 255, "max strength is full intensity");
    Check(Bloom_Intensity(BLOOM_MAX + 10) == 255, "above max clamps");

    // Linear in between: half strength is about half intensity.
    Check(Bloom_Intensity(50) == (50 * 255) / BLOOM_MAX, "half strength");
    Check(Bloom_Intensity(25) == (25 * 255) / BLOOM_MAX, "quarter strength");
    Check(Bloom_Intensity(75) == (75 * 255) / BLOOM_MAX, "three-quarter strength");

    // Monotonic: more strength never gives less intensity.
    int prev = 0;
    for (int s = 0; s <= BLOOM_MAX; s += 7) {
        int m = Bloom_Intensity(s);
        Check(m >= prev, "intensity is monotonic in strength");
        prev = m;
    }

    if (failures == 0) {
        puts("bloom: strength-to-intensity mapping holds");
        return 0;
    }
    return 1;
}
