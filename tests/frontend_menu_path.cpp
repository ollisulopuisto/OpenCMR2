// Exercises the exact inline the main menu enter calls (game/FrontendMenuPath.h):
// a draw before the first update must always find a path (native launch
// segfault: NULL+8 in FrontendAnimation_EvaluatePathSpline, .ips 2026-11-01-000237).
#include "FrontendMenuPath.h"
#include <cstdio>
static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main() {
    int storage[4] = { 10, 11, 12, 13 };
    // First entry: no path yet -> the helper installs the mode path and marks init.
    int *path = 0;
    int init = 0;
    FrontendMenuPath_Ensure(&path, &init, storage);
    CHECK(path == storage && init == 1);
    // After the update has taken over (morph buffer installed): untouched.
    int morph[4] = { 20, 21, 22, 23 };
    path = morph; init = 1;
    FrontendMenuPath_Ensure(&path, &init, storage);
    CHECK(path == morph && init == 1);
    // Cheat path installed by the update's first branch: also untouched.
    int cheat[4] = { 30, 31, 32, 33 };
    path = cheat; init = 0;   // the update's cheat branch ignores init
    FrontendMenuPath_Ensure(&path, &init, storage);
    CHECK(path == cheat && init == 0);
    std::printf(fails ? "FAILED\n" : "frontend menu path ok\n");
    return fails ? 1 : 0;
}
