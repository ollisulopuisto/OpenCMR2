#ifndef OPENCMR2_AIR_MOTE_H
#define OPENCMR2_AIR_MOTE_H
#include <cmath>
struct Texture;
#include "Particle.h"
// Optional floating dust motes (video.air_motes, default off): pale specks drifting
// slowly in the air around the viewed car, visible at any speed including a standstill.
// This is an atmosphere effect, deliberately unlike the removed speed motes (a speed cue
// whose look the user rejected): these hang in the air and drift instead of streaming
// past. Visual only: uses the existing 400-particle pool, never touches physics, the
// collision world, the road line or the global rand stream (own LCG in the caller).
constexpr int AIR_MOTE_TYPE = 0x22;
constexpr int AIR_MOTE_BASE = 48;    // live cap at amount 100
constexpr int AIR_MOTE_LIFE = 150;   // 25 Hz steps = 6 s
constexpr int AIR_MOTE_MAX_AMOUNT = 500;
// Peak alpha: raised 130 -> 190 after the first native look found the specks too faint
// against bright fog. Second native look ("smaller and more plentiful, like small
// bugs"): size 0x3800 -> 0x2000 and the amount ceiling 300 -> 500 (cap 144 -> 240).
// Third native look ("much smaller"): size 0x2000 -> 0x1000 (see StageTiming); count,
// cap and alpha stay - the density, not the speck size, carries the effect.
constexpr int AIR_MOTE_ALPHA_MAX = 190;
// video.air_motes_amount (0..500): spawn rate and, above 100, the live cap grow with it.
inline int AirMote_Cap(int amountPercent) {
    int a = amountPercent < 0 ? 0 : amountPercent > AIR_MOTE_MAX_AMOUNT ? AIR_MOTE_MAX_AMOUNT : amountPercent;
    return AIR_MOTE_BASE * a / 100;
}
// Same pool discipline as the dust plume and speed motes: the last 64 slots stay
// original-only. The cap above the base count applies while the pool is less than
// three-quarters full (raised from half for the "more plentiful" revision: any other
// spawn may still evict a mote first (see Particle_Spawn), so a fuller mote share can
// never starve original particles; priority is unchanged).
inline bool AirMote_HasBudget(int active, int motes, int capacity, int amountPercent) {
    if (!(capacity > 64 && active < capacity - 64)) return false;
    if (motes < AIR_MOTE_BASE) return true;
    return motes < AirMote_Cap(amountPercent) && active < capacity * 3 / 4;
}
// Slow drift: p.position is the per-step velocity (16.16 fixed), eased toward a gentle
// ambient drift (a fraction of the stage wind) plus a slow sink. Alpha fades in and out.
inline void AirMote_Step(Particle &p, const FixVector &wind) {
    p.position.x += (wind.x / 16 - p.position.x) / 48;
    p.position.z += (wind.z / 16 - p.position.z) / 48;
    p.position.y += (-0x300 - p.position.y) / 48;   // sinks about 0.3 world units per second
    p.sourceVector.x += p.position.x;
    p.sourceVector.y += p.position.y;
    p.sourceVector.z += p.position.z;
    p.vector0x28 = p.sourceVector;
    int elapsed = AIR_MOTE_LIFE - (p.age >> 16);
    if (elapsed < 0) elapsed = 0;
    if (elapsed > AIR_MOTE_LIFE) elapsed = AIR_MOTE_LIFE;
    int fadeIn = elapsed < 30 ? elapsed : 30;
    int fadeOut = AIR_MOTE_LIFE - elapsed;
    if (fadeOut > 50) fadeOut = 50;
    p.type0x52 = p.type0x56;
    p.type0x56 = (BYTE)(AIR_MOTE_ALPHA_MAX * fadeIn * fadeOut / (30 * 50));
}
// Spawn placement in a ring around a centre point (the viewed car): distance 3..24 world
// units, area-uniform angle and radius, 0.3..4.0 units above the ground. u1..u3 in 0..1.
// Pure maths, covered by tests/air_mote_spawn.cpp.
inline void AirMote_SpawnOffset(float u1, float u2, float u3, float *dx, float *dz, float *height) {
    float angle = 6.2831853f * u1;
    float distance = 3.f + 21.f * std::sqrt(u2 < 0.f ? 0.f : u2 > 1.f ? 1.f : u2);
    *dx = std::cos(angle) * distance;
    *dz = std::sin(angle) * distance;
    *height = 0.3f + 3.7f * (u3 < 0.f ? 0.f : u3 > 1.f ? 1.f : u3);
}
// Spawn rate in particles per 25 Hz step at amount 100: 8/s keeps the 48-strong cap
// populated against the 6 s life without a visible burst.
inline float AirMote_SpawnRate(int amountPercent) {
    int a = amountPercent < 0 ? 0 : amountPercent > AIR_MOTE_MAX_AMOUNT ? AIR_MOTE_MAX_AMOUNT : amountPercent;
    return 0.32f * (a / 100.f);
}
void AirMote_Update(Particle *, ParticleType *, SceneNode *);
void AirMote_Spawn(FixVector *world);
void AirMotes_Step(int step16_16);
void AirMote_LogStats(int stepsElapsed);
void AirMote_GetStats(int *live, unsigned *drawn);
#endif
