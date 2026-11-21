#ifndef OPENCMR2_WHEEL_PARTICLE_EMISSION_H
#define OPENCMR2_WHEEL_PARTICLE_EMISSION_H
#include <stdint.h>
#include <cmath>
// Visual-only emission decision. wheelKmh is this wheel's signed16.16 road
// speed, not engine torque or a rear-wheel proxy. Free-rolling wheels also
// throw dirt; never invent a four-wheel-drive classification from appearance.
inline int WheelParticle_Count(int wheelKmh, bool contact, bool looseOrWet, unsigned random)
{
    if (!contact || !looseOrWet) return 0;
    int64_t speed = wheelKmh;
    if (speed < 0) speed = -speed;
    if (speed > 0xf0000) return random % 3 == 0 ? 1 : 0;
    if (speed > 0xa0000) return random % 4 == 0 ? 1 : 0;
    return 0;
}
// Optional stronger debris (video.tire_debris, 0 = off and the original count). Visual only, same contact and surface gates as above: a grounded
// wheel on a loose or wet surface above 10 km/h wheel speed emits on average level/100 chips per effects update (whole part always, remainder by chance),
// never fewer than the original count and never more than 20. Default 800 (deliberately heavy per user request; dial down to 0 for the original look).
inline int WheelParticle_BoostedCount(int wheelKmh, bool contact, bool looseOrWet, unsigned random, int level)
{
    int base = WheelParticle_Count(wheelKmh, contact, looseOrWet, random);
    if (level <= 0 || !contact || !looseOrWet) return base;
    int64_t speed = wheelKmh;
    if (speed < 0) speed = -speed;
    if (speed <= 0xa0000) return base;
    if (level > 2000) level = 2000;
    int n = level / 100 + ((int)((random >> 8) % 100) < level % 100 ? 1 : 0);
    if (n > 20) n = 20;
    return n > base ? n : base;
}
// Chip flood guard (aggregate pool protection): live wheel-chip particles
// (types 0x10..0x17) may not exceed 60% of the pool, so a dirt wall cannot
// starve the plume cap reserve, weather, skid smoke or other original
// effects; motes still evict first under the spawn policy.
inline bool WheelParticle_ChipBudgetOK(int liveChips, int capacity) {
    return liveChips < capacity * 3 / 5;
}
// Dirt cone / rooster tail (video.dirt_cone, percent, 0 = off, default 100, max 300). The ordinary chips fly nearly flat (lift under 0.15 unit
// per step), so a fast gravel corner showed scattered clods but no plume. These extra chips leave the contact patch in a cone: backwards
// against the wheel's travel, lifted in proportion to its speed, fanned sideways. Visual only, same contact/loose-ground gates as above.
// wheelSpeed is the wheel's horizontal speed in world units per step (16.16 as float, about 1.3 at 136 km/h).
inline int WheelCone_Count(int wheelKmh, bool contact, bool looseOrWet, unsigned random, int amountPercent)
{
    if (amountPercent <= 0 || !contact || !looseOrWet) return 0;
    int64_t speed = wheelKmh;
    if (speed < 0) speed = -speed;
    if (speed <= 0x140000) return 0;                 // under 20 km/h there is no tail
    if (amountPercent > 300) amountPercent = 300;
    int64_t over = speed - 0x140000;                 // ramps up to full over the next 80 km/h
    if (over > 0x500000) over = 0x500000;
    int scaled = (int)(amountPercent * (0x8000 + over * 0x18000 / 0x500000) / 0x10000); // 0.5x at 20 km/h, 2x at 100 km/h, per 100 percent
    int n = scaled / 100 + ((int)((random >> 8) % 100) < scaled % 100 ? 1 : 0);
    return n > 8 ? 8 : n;
}
// Launch velocity of one cone chip (world units per step). vx/vz is the wheel's horizontal velocity, u1..u3 are 0..1 random numbers.
// Returns false when the wheel is too slow to define a direction. Back = 0.30..0.55 of the speed, lift = 0.25..0.60 of the speed,
// sideways = up to +-0.25 of the speed, so the chips spread into a cone opening behind the wheel and rising above the ground.
inline bool WheelCone_Velocity(float vx, float vz, float u1, float u2, float u3, float *ox, float *oy, float *oz)
{
    float speed = std::sqrt(vx * vx + vz * vz);
    if (!(speed > 0.05f)) return false;
    float dx = vx / speed, dz = vz / speed;
    float back = -speed * (0.30f + 0.25f * u1);
    float side = speed * 0.25f * (2.f * u3 - 1.f);
    *ox = dx * back - dz * side;
    *oz = dz * back + dx * side;
    *oy = speed * (0.25f + 0.35f * u2);
    return true;
}
#endif
