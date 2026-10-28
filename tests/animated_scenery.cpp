// OpenCMR2: animated scenery idle cycles (crowds, trees) — texture frame
// cycling driven by [video] video.animated_scenery. Pure helpers are
// unit-tested here; the draw-loop wiring lives in Game.cpp.
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
    // Frame advance: timer counts down, frame wraps within frameCount.
    struct { int timer; int frame; int count; int wantFrame; int wantTimer; } cases[] = {
        { 5,  0, 4, 0, 4 },   // tick, no wrap
        { 1,  0, 4, 1, 5 },   // wrap to next frame, reset timer
        { 1,  3, 4, 0, 5 },   // wrap back to 0 at end
        { 10, 2, 4, 2, 9 },   // slow timer, no change
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int frame = cases[i].frame;
        int timer = cases[i].timer;
        frame = AnimFrame_Advance(frame, cases[i].count, &timer);
        Check(frame == cases[i].wantFrame, "anim frame advance");
        Check(timer == cases[i].wantTimer, "anim timer reset");
    }

    // Animated texture index: base + frame.
    Check(AnimTexture_Index(10, 0) == 10, "anim texture base frame 0");
    Check(AnimTexture_Index(10, 3) == 13, "anim texture base + 3");

    if (failures == 0) {
        puts("animated scenery: frame advance and texture indexing hold");
        return 0;
    }
    return 1;
}
