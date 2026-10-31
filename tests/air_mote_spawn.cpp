#include <initializer_list>
#include "port/types.h"
#include "AirMote.h"
#include <cstdio>
#include <cmath>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    // Cap and budget: base 48 at amount 100, up to 144 at 300, last 64 pool slots stay original-only.
    CHECK(AirMote_Cap(100) == 48);
    CHECK(AirMote_Cap(0) == 0);
    CHECK(AirMote_Cap(300) == 144);
    CHECK(AirMote_Cap(900) == 144);
    CHECK(!AirMote_HasBudget(0, 0, 64, 100));          // pool of 64 leaves no original reserve
    CHECK(!AirMote_HasBudget(400 - 64, 0, 400, 100));  // only 64 slots left: refuse
    CHECK(AirMote_HasBudget(100, 47, 400, 100));
    CHECK(!AirMote_HasBudget(100, 48, 400, 100));      // at the base cap
    CHECK(!AirMote_HasBudget(100, 100, 400, 100));     // above base cap needs amount > 100...
    CHECK(AirMote_HasBudget(100, 100, 400, 300));      // ...and a pool less than half full
    CHECK(!AirMote_HasBudget(250, 100, 400, 300));     // pool half full: no motes past base
    // Spawn ring: distance 3..24 world units, height 0.3..4.0, any angle.
    for (float u1 : {0.f, .25f, .5f, .75f, 1.f}) for (float u2 : {0.f, .5f, 1.f}) for (float u3 : {0.f, .5f, 1.f}) {
        float dx, dz, h;
        AirMote_SpawnOffset(u1, u2, u3, &dx, &dz, &h);
        float dist = std::sqrt(dx*dx + dz*dz);
        CHECK(dist >= 3.f - 1e-3f && dist <= 24.f + 1e-3f);
        CHECK(h >= 0.3f - 1e-6f && h <= 4.f + 1e-6f);
    }
    { float dx, dz, h; AirMote_SpawnOffset(0.f, 0.f, 0.f, &dx, &dz, &h); CHECK(std::fabs(dx - 3.f) < 1e-3f && std::fabs(dz) < 1e-3f && std::fabs(h - 0.3f) < 1e-6f); }
    // sqrt distribution keeps density uniform over the disc area: E[dist^2] lands mid-ring.
    double mean2 = 0; int n = 0;
    for (int i = 0; i < 2000; ++i) {
        float u2 = (i + 0.5f) / 2000.f, dx, dz, h;
        AirMote_SpawnOffset(0.31f, u2, 0.7f, &dx, &dz, &h);
        mean2 += dx*dx + dz*dz; ++n;
    }
    mean2 /= n;
    double expect = 9.0 + 126.0*(2.0/3.0) + 441.0/2.0;  // E[(3+21*sqrt(U))^2] = 9 + 126*E[sqrt U] + 441*E[U]
    CHECK(std::fabs(mean2 - expect) / expect < 0.03);
    // Rate: 0.32 per step at 100 (= 8/s at 25 Hz), scales with amount, clamps.
    CHECK(std::fabs(AirMote_SpawnRate(100) - 0.32f) < 1e-7f);
    CHECK(AirMote_SpawnRate(0) == 0.f);
    CHECK(std::fabs(AirMote_SpawnRate(300) - 0.96f) < 1e-6f);
    CHECK(std::fabs(AirMote_SpawnRate(900) - 0.96f) < 1e-6f);
    // Step: drift eases toward wind/16 with a slow sink; alpha fades in and out, never negative, never above AIR_MOTE_ALPHA_MAX.
    for (int ageSteps : {0, 15, 30, 75, 100, 149, 150, 200}) {
        Particle p = {};
        p.age = ageSteps << 16;
        p.position = {0, 0, 0};
        p.sourceVector = {100 << 16, 50 << 16, -30 << 16};
        FixVector wind = {0x10000, 0, -0x8000};
        int before = p.sourceVector.y;
        AirMote_Step(p, wind);
        CHECK(p.type0x56 <= AIR_MOTE_ALPHA_MAX);
        CHECK(p.vector0x28.x == p.sourceVector.x && p.vector0x28.z == p.sourceVector.z);
        if (ageSteps > 0) CHECK(p.sourceVector.y <= before);   // sinks (or is still easing toward the sink)
        int alpha = p.type0x56;
        if (ageSteps == 0) CHECK(alpha == 0);                  // brand-new mote is invisible
        if (ageSteps == 150) CHECK(alpha == 0);                // dead of old age
        if (ageSteps == 75) CHECK(alpha > 0);
    }
    if (!fails) std::puts("air_mote_spawn ok");
    return fails;
}
