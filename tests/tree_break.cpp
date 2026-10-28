// OpenCMR2: breakable trees — a hard hit smashes a tree (it stops colliding
// and is no longer drawn); a light tap still stops the car. Pure helpers are
// unit-tested here; the rest is wired into the sector-object collision and
// draw loops behind [video] video.tree_break.
#include "port/types.h"
#include "Sector.h"
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
    // The broken flag bit: clear by default, settable, readable on a StageObject.
    StageObject obj = {};
    Check((obj.flags & STAGE_OBJECT_BROKEN) == 0, "tree not broken by default");
    obj.flags |= STAGE_OBJECT_BROKEN;
    Check((obj.flags & STAGE_OBJECT_BROKEN) != 0, "tree broken after set");
    obj.flags &= ~STAGE_OBJECT_BROKEN;
    Check((obj.flags & STAGE_OBJECT_BROKEN) == 0, "tree not broken after clear");

    // Speed threshold: below = no break (car stops), at/above = break (tree smashed).
    Check(TreeBreak_ShouldBreak(0) == false, "stopped car does not break");
    Check(TreeBreak_ShouldBreak(0xbfff) == false, "gentle tap does not break");
    Check(TreeBreak_ShouldBreak(0xc000) == true, "at threshold breaks");
    Check(TreeBreak_ShouldBreak(0x1ffff) == true, "above current threshold breaks");
    Check(TreeBreak_ShouldBreak(0x7fff0000) == true, "high speed breaks");

    if (failures == 0) {
        puts("tree break: broken flag and speed threshold hold");
        return 0;
    }
    return 1;
}
