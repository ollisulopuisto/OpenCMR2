// ParticleAmount.h dial maths: the wheel spray interval scaling and exhaust
// smoke puff count stay bounded, frame-rate independent (the callers emit on
// the fixed 25 Hz effects step) and byte-compatible with the original ranges.
#include "ParticleAmount.h"
#include <cstdio>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main()
{
    // Interval scaling: original intervals are 2..10, stored in char.
    CHECK(ParticleAmount_ScaledInterval(4, 100) == 4);    // 100% = original behaviour
    CHECK(ParticleAmount_ScaledInterval(4, 200) == 2);    // 200% default = twice the spray
    CHECK(ParticleAmount_ScaledInterval(10, 200) == 5);
    CHECK(ParticleAmount_ScaledInterval(2, 200) == 1);    // clamped, never zero
    CHECK(ParticleAmount_ScaledInterval(2, 300) == 1);
    CHECK(ParticleAmount_ScaledInterval(4, 300) == 1);
    CHECK(ParticleAmount_ScaledInterval(10, 300) == 3);
    CHECK(ParticleAmount_ScaledInterval(4, 50) == 8);     // below 100% thins the spray
    CHECK(ParticleAmount_ScaledInterval(127, 10) == 127); // hard floor: in char range
    CHECK(ParticleAmount_ScaledInterval(1, 0) == 10);     // amount 0 is handled by the caller's skip
    // Puff count: original is exactly 1 per qualifying exhaust per step.
    CHECK(ParticleAmount_PuffCount(100) == 1);
    CHECK(ParticleAmount_PuffCount(200) == 2);            // 200% default = double puffs
    CHECK(ParticleAmount_PuffCount(300) == 3);            // capped
    CHECK(ParticleAmount_PuffCount(999) == 3);            // out-of-range clamps
    CHECK(ParticleAmount_PuffCount(0) == 0);              // off
    CHECK(ParticleAmount_PuffCount(99) == 0);             // below 100%: no puff
    CHECK(ParticleAmount_PuffCount(-5) == 0);
    if (!fails) std::puts("particle_amount ok");
    return fails ? 1 : 0;
}
