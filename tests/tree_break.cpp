// Reserved tree-fall helpers only, not proof of working destructibility.
// Real sector-object collision is covered by tree_collision_runtime.cpp.
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

    // Legacy threshold helper, deliberately NOT used to classify sector objects.
    Check(TreeBreak_ShouldBreak(0) == false, "stopped car does not break");
    Check(TreeBreak_ShouldBreak(0x1ffff) == false, "gentle tap does not break");
    Check(TreeBreak_ShouldBreak(0x20000) == true, "at threshold breaks");
    Check(TreeBreak_ShouldBreak(0x7fff0000) == true, "high speed breaks");

    if (failures == 0) {
        puts("tree break: broken flag and speed threshold hold");
        return 0;
    }
    return 1;
}
