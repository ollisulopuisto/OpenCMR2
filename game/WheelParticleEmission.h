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
#endif
