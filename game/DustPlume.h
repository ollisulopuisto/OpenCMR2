#ifndef OPENCMR2_DUST_PLUME_H
#define OPENCMR2_DUST_PLUME_H
#include <cmath>
struct Texture;
#include "Particle.h"
// One extra particle template, using the existing400particle pool. Up to96
// cloud billboards; at least64pool slots stay free for original effects.
constexpr int DUST_PLUME_TYPE = 0x20;
constexpr int DUST_PLUME_MAX = 96;
constexpr int DUST_PLUME_LIFE = 150; // fixed particle simulation steps; wall time follows game stepping
inline void DustPlume_Step(Particle &p, const FixVector &wind) {
    // Fixed-step visual advection, deliberately independent of owner position.
    p.position.x += (wind.x/10-p.position.x)/32;
    p.position.z += (wind.z/10-p.position.z)/32;
    p.position.y += (0x500-p.position.y)/48;
    p.sourceVector.x += p.position.x;
    p.sourceVector.y += p.position.y;
    p.sourceVector.z += p.position.z;
    p.vector0x28 = p.sourceVector;
    int elapsed = DUST_PLUME_LIFE - (p.age >> 16);
    if (elapsed < 0) elapsed = 0;
    if (elapsed > DUST_PLUME_LIFE) elapsed = DUST_PLUME_LIFE;
    // Smooth fade-in avoids visible pop, fading tail retains the old bend.
    int fadeIn = elapsed < 12 ? elapsed : 12;
    int fadeOut = (DUST_PLUME_LIFE-elapsed);
    int alpha = 75 * fadeIn * fadeOut / (12*DUST_PLUME_LIFE);
    p.type0x52 = p.type0x56;
    p.type0x56 = (BYTE)alpha;
    p.size = 0x6000 + elapsed * 0x30000 / DUST_PLUME_LIFE;
}
inline bool DustPlume_Surface(bool dirt, bool gravel, bool water, bool snow, bool ice) {
    return (dirt || gravel) && !water && !snow && !ice;
}
// The original tiny wheel particles use some green-dominant country dirt
// palettes. Do not amplify that grass-like tint into a whole soil cloud.
inline void DustPlume_SoilTint(float &r,float &g,float &b) {
    if(g>r){float green=g;r=green*1.12f;if(r>1.f)r=1.f;g=green*.84f;}
}
inline bool DustPlume_HasBudget(int active, int clouds, int capacity) {
    return capacity > 64 && clouds < DUST_PLUME_MAX && active < capacity-64;
}
// Optional speed cue (video.speed_motes, default off): sparse, world-fixed specks beside the road
// that the car passes at speed. Visual only: uses the particle pool, never touches physics, the
// collision world, the road line or the global rand stream. Separate from the rain/weather switch.
constexpr int SPEED_MOTE_TYPE = 0x21;
constexpr int SPEED_MOTE_MAX = 64;
constexpr int SPEED_MOTE_LIFE = 48; // 25 Hz steps
inline void SpeedMote_Step(Particle &p) {
    p.position.x = p.position.y = p.position.z = 0; // world-fixed: it never follows the car or the camera
    p.vector0x28 = p.sourceVector;
    int elapsed = SPEED_MOTE_LIFE - (p.age >> 16);
    if (elapsed < 0) elapsed = 0;
    if (elapsed > SPEED_MOTE_LIFE) elapsed = SPEED_MOTE_LIFE;
    int fadeIn = elapsed < 6 ? elapsed : 6, fadeOut = SPEED_MOTE_LIFE - elapsed;
    if (fadeOut > 10) fadeOut = 10;
    p.type0x52 = p.type0x56;
    p.type0x56 = (BYTE)(150 * fadeIn * fadeOut / 60);
}
// The live-mote cap grows with video.speed_motes_amount above 100% (64 at 100%, 192 at 300%); below 100% it stays 64 and
// only the spawn rate drops. Before this, amount 300 saturated at 64 live motes and could not show more.
inline int SpeedMote_Cap(int amountPercent) {
    int a = amountPercent < 100 ? 100 : amountPercent > 300 ? 300 : amountPercent;
    return SPEED_MOTE_MAX * a / 100;
}
inline bool SpeedMote_HasBudget(int active, int motes, int capacity, int amountPercent = 100) {
    return capacity > 64 && motes < SpeedMote_Cap(amountPercent) && active < capacity - 64;
}
// video.speed_motes_size: quad size in percent of the normal 0.5 m speck (100 = unchanged), clamped 50..800.
inline int SpeedMote_SizePercent(int v) { return v < 50 ? 50 : v > 800 ? 800 : v; }
void SpeedMote_Update(Particle *, ParticleType *, SceneNode *);
void SpeedMote_Spawn(FixVector *world);
struct BillboardDef; struct SceneNode;
void SpeedMote_ApplyStreak(BillboardDef *def, SceneNode *camera);
void SpeedMote_SetRatio(float ratio);
// Streak parameters for one mote (pure maths, see tests/speed_mote_streak.cpp). right/up/depth are the mote's offset from
// the camera in the camera basis (world units). stretch is video.speed_motes_stretch (0 to 100), ratio the speed ratio 0..1.
// Returns false when the mote is left as a round dot (stretch off, behind the camera, or on the view axis).
// angle12 is the 12-bit billboard rotation about the view axis that points the long axis from the view axis to the mote.
inline bool SpeedMote_StreakParams(float right, float up, float depth, float ratio, int stretch,
                                   int *angle12, float *lengthMul, float *alphaMul) {
    if (stretch <= 0 || depth <= 0.f) return false;
    if (stretch > 100) stretch = 100;
    if (ratio < 0.f) ratio = 0.f;
    if (ratio > 1.f) ratio = 1.f;
    if (right * right + up * up < 1e-6f) return false;
    const float tau = 6.2831853f;
    float turns = std::atan2(up, right) / tau;
    if (turns < 0.f) turns += 1.f;
    *angle12 = (int)std::floor(turns * 4096.f + 0.5f) & 0xfff;
    *lengthMul = 1.f + (stretch / 100.f) * (1.f + 7.f * ratio);
    float f = (depth - 3.f) / 6.f; // gone inside 3 units, full beyond 9
    *alphaMul = f < 0.f ? 0.f : f > 1.f ? 1.f : f;
    return true;
}
// Diagnostics (log only): counts spawned / budget-skipped / drawn motes and logs one line about every 5 s while motes are on.
void SpeedMote_NoteSkippedGround(void);
void SpeedMote_LogStats(float speedRatio,int stepsElapsed);
void SpeedMotes_Step(int step16_16);
void DustPlume_AdvanceWorld(int step16_16);
void DustPlume_Update(Particle *, ParticleType *, SceneNode *);
void DustPlume_Emit(FixVector *world, FixVector *velocity, BYTE *colour, BYTE shade, BYTE viewMask);
#endif
