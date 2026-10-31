#ifndef OPENCMR2_PARTICLE_AMOUNT_H
#define OPENCMR2_PARTICLE_AMOUNT_H
// Shared amount-dial maths for the original car-effect emitters (wheel spray,
// exhaust smoke). The dials overdrive emission deliberately - the user asked
// for overdone particle FX first with per-effect dials to tone down - while
// every emitter stays on the fixed 25 Hz effects step (frame-rate
// independent) and inside the existing bounded particle pool.

inline int ParticleAmount_Clamp(int amountPercent)
{
    return amountPercent < 0 ? 0 : amountPercent > 300 ? 300 : amountPercent;
}

// Wheel spray runs on per-wheel countdowns of a few steps (original intervals
// are 2..10). Higher amount = shorter interval = more spray; 0 disables.
// Intervals stay in the original char range (1..127).
inline int ParticleAmount_ScaledInterval(int interval, int amountPercent)
{
    int a = ParticleAmount_Clamp(amountPercent);
    if (a < 10) a = 10;   // below 10% spray is effectively off; keep the maths in range
    int v = (int)((long)interval * 100 / a);
    return v < 1 ? 1 : v > 127 ? 127 : v;
}

// Exhaust smoke emits this many puffs per qualifying exhaust per step:
// the original single puff at 100%, none at 0, at most 3 at 300%.
inline int ParticleAmount_PuffCount(int amountPercent)
{
    return ParticleAmount_Clamp(amountPercent) / 100;
}
#endif
