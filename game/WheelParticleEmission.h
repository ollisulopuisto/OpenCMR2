#ifndef OPENCMR2_WHEEL_PARTICLE_EMISSION_H
#define OPENCMR2_WHEEL_PARTICLE_EMISSION_H
#include <stdint.h>
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
// Optional stronger debris (video.tire_debris, 0 = off, the default and the original count). Visual only, same contact and surface gates as above: a grounded
// wheel on a loose or wet surface above 10 km/h wheel speed emits on average level/100 chips per effects update (whole part always, remainder by chance),
// never fewer than the original count and never more than 4.
inline int WheelParticle_BoostedCount(int wheelKmh, bool contact, bool looseOrWet, unsigned random, int level)
{
    int base = WheelParticle_Count(wheelKmh, contact, looseOrWet, random);
    if (level <= 0 || !contact || !looseOrWet) return base;
    int64_t speed = wheelKmh;
    if (speed < 0) speed = -speed;
    if (speed <= 0xa0000) return base;
    if (level > 400) level = 400;
    int n = level / 100 + ((int)((random >> 8) % 100) < level % 100 ? 1 : 0);
    if (n > 4) n = 4;
    return n > base ? n : base;
}
#endif
