// Dirt cone / rooster tail: emission gates and launch geometry (pure helpers).
#include "WheelParticleEmission.h"
#include <cstdio>
#include <cmath>
int main() {
    int fail = 0;
    auto check = [&](bool v, const char *s) { if (!v) { puts(s); ++fail; } };
    // Gates: off, airborne, hard surface, slow.
    check(WheelCone_Count(130 << 16, true, true, 0, 0) == 0, "amount 0 is off");
    check(WheelCone_Count(130 << 16, false, true, 0, 100) == 0, "airborne emits nothing");
    check(WheelCone_Count(130 << 16, true, false, 0, 100) == 0, "hard surface emits nothing");
    check(WheelCone_Count(20 << 16, true, true, 0, 100) == 0, "20 km/h and below: no tail");
    // Faster means more, more amount means more, always capped.
    int fast = 0, mid = 0, big = 0;
    for (unsigned r = 0; r < 100; ++r) {
        fast += WheelCone_Count(140 << 16, true, true, r << 8, 100);
        mid += WheelCone_Count(50 << 16, true, true, r << 8, 100);
        big += WheelCone_Count(140 << 16, true, true, r << 8, 300);
        check(WheelCone_Count(140 << 16, true, true, r << 8, 9999) <= 8, "cap of 8 per wheel step");
    }
    check(fast > mid, "faster wheel throws more");
    check(big > 2 * fast, "amount 300 throws much more than 100");
    check(fast >= 150 && fast <= 250, "about 2 chips per step at 100 percent and 140 km/h");
    check(WheelCone_Count(-(140 << 16), true, true, 0, 100) > 0, "reverse wheel speed counts");
    // Geometry: car driving +x at 1.3 units per step.
    float ox, oy, oz;
    check(!WheelCone_Velocity(0.f, 0.f, .5f, .5f, .5f, &ox, &oy, &oz), "stationary wheel has no direction");
    float minLift = 9, maxLift = 0, minBack = 9, maxBack = 0, maxSide = 0;
    for (int a = 0; a <= 10; ++a)
        for (int b = 0; b <= 10; ++b)
            for (int c = 0; c <= 10; ++c) {
                check(WheelCone_Velocity(1.3f, 0.f, a / 10.f, b / 10.f, c / 10.f, &ox, &oy, &oz), "moving wheel launches");
                minLift = std::fmin(minLift, oy); maxLift = std::fmax(maxLift, oy);
                minBack = std::fmin(minBack, -ox); maxBack = std::fmax(maxBack, -ox);
                maxSide = std::fmax(maxSide, std::fabs(oz));
            }
    check(minBack > 0.38f && maxBack < 0.72f, "thrown backwards against travel");
    check(minLift >= 1.3f * 0.25f - 1e-4f && maxLift <= 1.3f * 0.60f + 1e-4f, "lift scales with speed");
    check(minLift > 0.15f * 2.f, "well above the old flat 0.15 chip lift");
    check(maxSide > 0.25f && maxSide <= 1.3f * 0.25f + 1e-4f, "fans out sideways");
    // Direction follows the wheel: driving +z throws toward -z.
    WheelCone_Velocity(0.f, 1.3f, .5f, .5f, .5f, &ox, &oy, &oz);
    check(oz < -0.3f && std::fabs(ox) < 0.01f, "rotates with heading");
    // Budget: the batch is cut to the room left under the 60% chip cap (1200 pool -> 720 chips).
    check(WheelCone_BudgetedCount(6, 0, 1200) == 6, "plenty of room keeps the batch");
    check(WheelCone_BudgetedCount(6, 719, 1200) == 1, "one slot left allows one chip");
    check(WheelCone_BudgetedCount(6, 720, 1200) == 0, "at the cap nothing spawns");
    check(WheelCone_BudgetedCount(6, 900, 1200) == 0, "over the cap never goes negative");
    if (!fail) puts("dirt_cone: ok");
    return fail;
}
