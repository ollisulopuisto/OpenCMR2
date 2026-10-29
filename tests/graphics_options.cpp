// OpenCMR2: pure helpers behind the advanced graphics page's rows: the user
// draw distance ([video] video.draw_distance) and the texture filtering level
// ([video] video.anisotropy). Kept free of game and renderer includes so the
// tests can exercise the mappings alone.
#include "GraphicsOptions.h"
#include <stdio.h>

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
    Check(WINDSREEN_WIPER_KEY_DEFAULT == 0x11, "wiper default is physical W");
    Check(WindscreenWiper_Toggle(false,true) && !WindscreenWiper_Toggle(true,true), "wiper edges toggle both ways");
    Check(!WindscreenWiper_Toggle(false,false) && WindscreenWiper_Toggle(true,false), "no edge preserves state");
    // The draw distance clamps to a sane range of the stage's far plane.
    Check(View_ClampDrawDistancePercent(-100) == 25, "clamp(-100) = 25");
    Check(View_ClampDrawDistancePercent(24) == 25, "clamp(24) = 25");
    Check(View_ClampDrawDistancePercent(25) == 25, "clamp(25) = 25");
    Check(View_ClampDrawDistancePercent(100) == 100, "clamp(100) = 100");
    Check(View_ClampDrawDistancePercent(400) == 400, "clamp(400) = 400");
    Check(View_ClampDrawDistancePercent(401) == 400, "clamp(401) = 400");

    // Every offered step round-trips through the menu index, and a value
    // between steps snaps to the nearest one.
    for (int i = 0; i < DRAW_DISTANCE_COUNT; i++)
        Check(DrawDistance_PercentToIndex(DrawDistance_IndexToPercent(i)) == i, "draw distance round trip");
    Check(DrawDistance_PercentToIndex(90) == 3, "90% snaps to 100%");
    Check(DrawDistance_IndexToPercent(-1) == 25 && DrawDistance_IndexToPercent(99) == 200,
          "draw distance index clamps");

    // The filtering levels: index 0 is off (the original's point sampling),
    // the ends map back, and every level round-trips.
    Check(Anisotropy_IndexToLevel(0) == 1, "filtering index 0 is off");
    Check(Anisotropy_IndexToLevel(ANISOTROPY_LEVEL_COUNT - 1) == 16, "filtering top is 16x");
    Check(Anisotropy_LevelToIndex(1) == 0, "level 1 is off");
    Check(Anisotropy_LevelToIndex(16) == ANISOTROPY_LEVEL_COUNT - 1, "level 16 is the top index");
    for (int i = 0; i < ANISOTROPY_LEVEL_COUNT; i++)
        Check(Anisotropy_LevelToIndex(Anisotropy_IndexToLevel(i)) == i, "filtering round trip");
    Check(Anisotropy_IndexToLevel(-1) == 1 && Anisotropy_IndexToLevel(99) == 16, "filtering index clamps");

    Check(OPENCMR2_DEBUG_DRAW_DEFAULT == 1, "diagnostic overlay defaults on");
    Check(ReflectionCube_Size(0, 64) == 64, "original reflection size opt out");
    Check(ReflectionCube_Size(-1, 128) == 128, "negative uses original size");
    Check(ReflectionCube_Size(128, 64) == 128, "higher detail reflection default");
    Check(ReflectionCube_Size(65, 64) == 128, "round resolution to bounded tier");
    Check(ReflectionCube_Size(99999, 64) == 256, "reflection allocation capped");
    if (failures == 0) {
        puts("graphics options: draw distance and texture filtering mappings hold");
        return 0;
    }
    return 1;
}
