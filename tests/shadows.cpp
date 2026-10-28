// OpenCMR2: directional shadows — [video] video.shadows (0 off / 1 on) and
// shadow depth bias (0..SHADOW_BIAS_MAX). Pure helpers, unit-tested here; the
// shadow map pass and fragment-shader sampling are wired into the renderer.
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
    // Bias clamps to [0, SHADOW_BIAS_MAX].
    Check(Shadow_Bias(0) == 0, "bias 0");
    Check(Shadow_Bias(-5) == 0, "negative bias clamps to 0");
    Check(Shadow_Bias(SHADOW_BIAS_MAX) == SHADOW_BIAS_MAX, "max bias");
    Check(Shadow_Bias(SHADOW_BIAS_MAX + 1) == SHADOW_BIAS_MAX, "above max clamps");
    Check(Shadow_Bias(37) == 37, "mid bias");

    if (failures == 0) {
        puts("shadows: bias clamping holds");
        return 0;
    }
    return 1;
}
