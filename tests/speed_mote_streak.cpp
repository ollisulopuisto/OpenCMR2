// Pure-maths check of the speed-mote streak orientation, stretch and near fade (no renderer involved).
#include "port/types.h"
#include "DustPlume.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main() {
    CHECK(SpeedMote_Cap(100)==64 && SpeedMote_Cap(50)==64 && SpeedMote_Cap(300)==192 && SpeedMote_Cap(999)==192);
    CHECK(SpeedMote_HasBudget(0,63,400,100) && !SpeedMote_HasBudget(0,64,400,100) && SpeedMote_HasBudget(0,100,400,300) && !SpeedMote_HasBudget(0,192,400,300));
    CHECK(SpeedMote_SizePercent(100)==100 && SpeedMote_SizePercent(1)==50 && SpeedMote_SizePercent(9999)==800);
    int a; float len, al;
    // Off, behind the camera, or exactly on the view axis: stays a round dot.
    CHECK(!SpeedMote_StreakParams(5, 0, 20, 1, 0, &a, &len, &al));
    CHECK(!SpeedMote_StreakParams(5, 0, -1, 1, 100, &a, &len, &al));
    CHECK(!SpeedMote_StreakParams(0, 0, 20, 1, 100, &a, &len, &al));
    // 12-bit angle: 0 = +right, 0x400 = +up, 0x800 = -right, 0xc00 = -up (Billboard_Draw: X' = right*cos + up*sin).
    CHECK(SpeedMote_StreakParams(5, 0, 20, 0, 100, &a, &len, &al)); CHECK(a == 0);
    CHECK(SpeedMote_StreakParams(0, 5, 20, 0, 100, &a, &len, &al)); CHECK(a == 0x400);
    CHECK(SpeedMote_StreakParams(-5, 0, 20, 0, 100, &a, &len, &al)); CHECK(a == 0x800);
    CHECK(SpeedMote_StreakParams(0, -5, 20, 0, 100, &a, &len, &al)); CHECK(a == 0xc00);
    CHECK(SpeedMote_StreakParams(5, 5, 20, 0, 100, &a, &len, &al)); CHECK(a == 0x200);
    // Radial: a mote at the same direction but twice the distance from the axis has the same angle.
    int b; CHECK(SpeedMote_StreakParams(10, 10, 20, 0, 100, &b, &len, &al)); CHECK(b == a);
    // Length multiplier: 1x at stretch 0 is excluded; full effect 2x at rest, 9x at full speed; half effect scales.
    CHECK(SpeedMote_StreakParams(5, 0, 20, 0, 100, &a, &len, &al)); CHECK(std::fabs(len - 2.f) < 1e-4f);
    CHECK(SpeedMote_StreakParams(5, 0, 20, 1, 100, &a, &len, &al)); CHECK(std::fabs(len - 9.f) < 1e-4f);
    CHECK(SpeedMote_StreakParams(5, 0, 20, 1, 50, &a, &len, &al)); CHECK(std::fabs(len - 5.f) < 1e-4f);
    CHECK(SpeedMote_StreakParams(5, 0, 20, 5, 500, &a, &len, &al)); CHECK(std::fabs(len - 9.f) < 1e-4f); // clamped
    // Near fade: 0 inside 3 units, 1 from 9 units on.
    CHECK(SpeedMote_StreakParams(5, 0, 2, 1, 100, &a, &len, &al)); CHECK(al == 0.f);
    CHECK(SpeedMote_StreakParams(5, 0, 6, 1, 100, &a, &len, &al)); CHECK(std::fabs(al - 0.5f) < 1e-4f);
    CHECK(SpeedMote_StreakParams(5, 0, 30, 1, 100, &a, &len, &al)); CHECK(al == 1.f);
    std::puts("speed_mote_streak ok");
    return 0;
}
