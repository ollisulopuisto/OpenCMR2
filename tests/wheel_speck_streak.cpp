// Pure maths of the wheel-speck wedge streak (game/DustPlume.h
// WheelSpeck_StreakParams): angle quadrants, motion gates, stretch ramp.
#include "port/types.h"
#include "DustPlume.h"
#include <cstdio>
#include <cmath>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main()
{
    int ang; float len, alpha;
    // Motion to the right -> angle 0 (or 4096 wrap), stretch applies.
    CHECK(WheelSpeck_StreakParams(2.f, 0.f, 10.f, 100, &ang, &len, &alpha));
    CHECK(ang == 0 || ang == 4096 - 0 || (ang & 0xfff) == 0);
    CHECK(len > 3.9f && len <= 4.01f);           // full stretch at len >= 2
    CHECK(alpha > 0.8f && alpha < 0.9f);
    // Motion straight up -> quarter turn (1024).
    CHECK(WheelSpeck_StreakParams(0.f, 1.f, 10.f, 100, &ang, &len, &alpha));
    CHECK(ang == 1024);
    // Motion down-left -> 5/8 turn (2560).
    CHECK(WheelSpeck_StreakParams(-1.f, -1.f, 10.f, 100, &ang, &len, &alpha));
    CHECK(ang == 2560);
    // Negative angle wrap: down-right -> 7/8 turn (3584).
    CHECK(WheelSpeck_StreakParams(1.f, -1.f, 10.f, 100, &ang, &len, &alpha));
    CHECK(ang == 3584);
    // Half ramp: len 1 -> f 0.5 -> 1 + (0.5 + 1.25) = 2.75x.
    CHECK(WheelSpeck_StreakParams(1.f, 0.f, 10.f, 100, &ang, &len, &alpha));
    CHECK(std::fabs(len - 2.75f) < 0.01f);
    // Stretch percent scales the added length, clamped at 100.
    CHECK(WheelSpeck_StreakParams(2.f, 0.f, 10.f, 50, &ang, &len, &alpha));
    CHECK(std::fabs(len - 2.5f) < 0.01f);
    CHECK(WheelSpeck_StreakParams(2.f, 0.f, 10.f, 300, &ang, &len, &alpha));
    CHECK(len > 3.9f && len <= 4.01f);
    // Gates: no stretch, behind the camera plane, no screen motion.
    CHECK(!WheelSpeck_StreakParams(2.f, 0.f, 10.f, 0, &ang, &len, &alpha));
    CHECK(!WheelSpeck_StreakParams(2.f, 0.f, 0.f, 100, &ang, &len, &alpha));
    CHECK(!WheelSpeck_StreakParams(2.f, 0.f, -5.f, 100, &ang, &len, &alpha));
    CHECK(!WheelSpeck_StreakParams(0.f, 0.f, 10.f, 100, &ang, &len, &alpha));
    if (!fails) std::puts("wheel_speck_streak ok");
    return fails;
}
