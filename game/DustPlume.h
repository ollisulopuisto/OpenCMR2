#ifndef OPENCMR2_DUST_PLUME_H
#define OPENCMR2_DUST_PLUME_H
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
void DustPlume_AdvanceWorld(int step16_16);
void DustPlume_Update(Particle *, ParticleType *, SceneNode *);
void DustPlume_Emit(FixVector *world, FixVector *velocity, BYTE *colour, BYTE shade, BYTE viewMask);
#endif
